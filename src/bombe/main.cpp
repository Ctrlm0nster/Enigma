#include "bombe/DecipherInput.hpp"

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <stop_token>
#include <string_view>
#include <unordered_map>

namespace {

using Options = std::unordered_map<std::string_view, std::string_view>;

volatile std::sig_atomic_t interrupted = 0;
void interrupt(int) { interrupted = 1; }

void usage(std::ostream &out) {
  out << "EnigmaBombe decipher --ciphertext TEXT --start ABC --rings 01,01,01 "
         "[--plugs AZ,BY]\n"
         "EnigmaBombe search --ciphertext TEXT --crib TEXT --rings 01,01,01\n"
         "                   [--offset N] [--plugs AZ,BY | --known-leads "
         "AZ,BY]\n"
         "Search uses rotors I-II-III and Reflector B. Offsets start at 1.\n"
         "Omit --offset to scan all alignments, or use --plugs '' for a known "
         "unplugged board.\n"
         "Use --ciphertext - to read from stdin. Ctrl+C cancels a search.\n";
}

Options parseOptions(int argc, char **argv, std::string_view mode) {
  Options result;
  for (int i = 2; i < argc; i += 2) {
    const std::string_view name(argv[i]);
    const bool common =
        name == "--ciphertext" || name == "--rings" || name == "--plugs";
    const bool decipher = mode == "decipher" && name == "--start";
    const bool search =
        mode == "search" &&
        (name == "--crib" || name == "--offset" || name == "--known-leads");
    if (!(common || decipher || search) || i + 1 == argc ||
        !result.emplace(name, argv[i + 1]).second) {
      throw std::invalid_argument("Invalid, repeated, or incomplete option " +
                                  std::string(name));
    }
  }
  return result;
}

std::string_view value(const Options &options, std::string_view key,
                       bool required = false) {
  const auto option = options.find(key);
  if (option != options.end()) {
    return option->second;
  }
  if (required) {
    throw std::invalid_argument("Missing " + std::string(key));
  }
  return {};
}

std::string readCiphertext(std::string_view option) {
  if (option != "-") {
    return std::string(option);
  }
  std::string text;
  char character;
  while (std::cin.get(character)) {
    if (const auto letter = enigma::normalizeLetter(character); letter != 0) {
      if (text.size() == enigma::messageLimit) {
        throw std::invalid_argument("Ciphertext exceeds 240 letters");
      }
      text.push_back(letter);
    }
  }
  return text;
}

int runSearch(const Options &options, std::string ciphertext,
              std::string_view rings) {
  const bool full = options.contains("--plugs");
  const bool partial = options.contains("--known-leads");
  if (full && partial) {
    throw std::invalid_argument("Use either --plugs or --known-leads");
  }
  auto mode = bombe::PlugboardMode::Unknown;
  if (full) {
    mode = bombe::PlugboardMode::Known;
  } else if (partial) {
    mode = bombe::PlugboardMode::Partial;
  }
  const auto request = bombe::parseSearch(
      std::move(ciphertext), std::string(value(options, "--crib", true)), rings,
      value(options, "--offset"), mode,
      value(options, full ? "--plugs" : "--known-leads"));
  std::stop_source stop;
  std::signal(SIGINT, interrupt);
  const auto result =
      bombe::search(request, {}, stop.get_token(), [&](const auto &) {
        if (interrupted) {
          stop.request_stop();
        }
      });
  const auto &progress = result.progress;
  std::cout << bombe::statusText(result.status) << '\n'
            << progress.viableAlignments << '/' << progress.alignments
            << " viable alignments, " << progress.settingsChecked
            << " settings tested, " << progress.matches << " crib matches\n";
  for (const auto &match : result.candidates) {
    std::cout << "\nStart " << bombe::formatStarts(match.settings)
              << " | Rings " << bombe::formatRings(match.settings)
              << " | Crib position " << match.cribOffset + 1 << '\n';
    const auto leads = bombe::formatLeads(match.settings.plugPairs);
    std::cout << "Leads " << (leads.empty() ? "none" : leads) << '\n';
    if (!match.unresolvedSockets.empty()) {
      std::cout << "Assumed unplugged for this preview "
                << match.unresolvedSockets << '\n';
    }
    std::cout << match.plaintext << '\n';
  }
  return result.status == bombe::SearchStatus::Cancelled ? 130 : EXIT_SUCCESS;
}

} // namespace

int main(int argc, char **argv) {
  if (argc == 1 || std::string_view(argv[1]) == "--help") {
    usage(std::cout);
    return EXIT_SUCCESS;
  }
  try {
    const std::string_view mode(argv[1]);
    if (mode != "decipher" && mode != "search") {
      throw std::invalid_argument("Choose decipher or search");
    }
    const auto options = parseOptions(argc, argv, mode);
    const auto rings = value(options, "--rings", true);
    auto ciphertext = readCiphertext(value(options, "--ciphertext", true));
    if (mode == "search") {
      return runSearch(options, std::move(ciphertext), rings);
    }
    const auto settings = bombe::parseSettings(
        value(options, "--start", true), rings, value(options, "--plugs"));
    std::cout << bombe::decipher(ciphertext, settings) << '\n';
    return EXIT_SUCCESS;
  } catch (const std::exception &error) {
    std::cerr << "EnigmaBombe: " << error.what() << '\n';
    return EXIT_FAILURE;
  }
}
