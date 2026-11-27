#include "bombe/DecipherInput.hpp"

#include "Charactermap.hpp"

#include <charconv>
#include <span>
#include <stdexcept>
#include <system_error>

namespace bombe {
namespace {

std::string_view nextToken(std::string_view &text,
                           std::string_view delimiters) {
  const auto first = text.find_first_not_of(delimiters);
  if (first == std::string_view::npos) {
    text = {};
    return {};
  }
  text.remove_prefix(first);
  const auto end = text.find_first_of(delimiters);
  const auto token = text.substr(0, end);
  text.remove_prefix(token.size());
  return token;
}

std::size_t positiveNumber(std::string_view token, std::string_view label) {
  std::size_t value = 0;
  const auto end = token.data() + token.size();
  const auto [last, error] = std::from_chars(token.data(), end, value);
  if (error != std::errc{} || last != end || value == 0) {
    throw std::invalid_argument(std::string(label) +
                                " must be a positive number");
  }
  return value;
}

void readRings(std::string_view text, std::span<int> rings) {
  for (auto &ring : rings) {
    const auto token = nextToken(text, ", \t\r\n");
    if (token.empty()) {
      throw std::invalid_argument("Enter one ring setting per rotor");
    }
    const auto value = positiveNumber(token, "Ring setting");
    if (value > 26) {
      throw std::invalid_argument("Ring settings must be between 01 and 26");
    }
    ring = static_cast<int>(value - 1);
  }
  if (!nextToken(text, ", \t\r\n").empty()) {
    throw std::invalid_argument("Enter one ring setting per rotor");
  }
}

std::vector<enigma::PlugPair> readLeads(std::string_view text) {
  std::vector<enigma::PlugPair> pairs;
  auto token = nextToken(text, ", \t\r\n");
  while (!token.empty()) {
    if (token.size() != 2 || !enigma::normalizeLetter(token[0]) ||
        !enigma::normalizeLetter(token[1])) {
      throw std::invalid_argument("Enter plugboard pairs like AZ BY");
    }
    if (pairs.size() == enigma::plugLimit) {
      throw std::invalid_argument("The plugboard supports up to 10 leads");
    }
    pairs.emplace_back(charToIndex(enigma::normalizeLetter(token[0])),
                       charToIndex(enigma::normalizeLetter(token[1])));
    token = nextToken(text, ", \t\r\n");
  }
  return pairs;
}

} // namespace

enigma::MachineSettings parseSettings(std::string_view starts,
                                      std::string_view rings,
                                      std::string_view leads) {
  if (starts.size() < enigma::minimumRotorCount ||
      starts.size() > enigma::maximumRotorCount) {
    throw std::invalid_argument(
        "Enter three to five starting letters, such as AAA");
  }
  enigma::MachineSettings settings;
  settings.rotorCount = starts.size();
  for (std::size_t i = 0; i < settings.rotorCount; ++i) {
    const auto letter = enigma::normalizeLetter(starts[i]);
    if (!letter) {
      throw std::invalid_argument("Starting positions must use A–Z");
    }
    settings.starts[i] = charToIndex(letter);
  }
  readRings(rings, std::span(settings.rings).first(settings.rotorCount));
  settings.plugPairs = readLeads(leads);
  return settings;
}

SearchRequest parseSearch(std::string ciphertext, std::string crib,
                          std::string_view rings, std::string_view position,
                          PlugboardMode mode, std::string_view leads) {
  SearchRequest request;
  request.ciphertext = std::move(ciphertext);
  request.crib = std::move(crib);
  readRings(rings, request.rings);
  if (!position.empty()) {
    request.cribOffset = positiveNumber(position, "Crib position") - 1;
  }
  request.plugboard = mode;
  request.knownLeads = readLeads(leads);
  return request;
}

std::string formatStarts(const enigma::MachineSettings &settings) {
  std::string text;
  for (std::size_t i = 0; i < settings.rotorCount; ++i) {
    text.push_back(indexToChar(settings.starts[i]));
  }
  return text;
}

std::string formatRings(const enigma::MachineSettings &settings) {
  std::string text;
  for (std::size_t i = 0; i < settings.rotorCount; ++i) {
    if (!text.empty()) {
      text += ' ';
    }
    const int ring = settings.rings[i] + 1;
    if (ring < 10) {
      text += '0';
    }
    text += std::to_string(ring);
  }
  return text;
}

std::string formatLeads(const std::vector<enigma::PlugPair> &pairs) {
  std::string text;
  for (const auto &[first, second] : pairs) {
    if (!text.empty()) {
      text += ' ';
    }
    text += indexToChar(first);
    text += indexToChar(second);
  }
  return text;
}

std::string_view statusText(SearchStatus status) {
  switch (status) {
  case SearchStatus::Complete:
    return "Search complete";
  case SearchStatus::Cancelled:
    return "Search cancelled";
  case SearchStatus::WorkLimit:
    return "Search work limit reached";
  case SearchStatus::ResultLimit:
    return "Result limit reached";
  }
  return {};
}

} // namespace bombe
