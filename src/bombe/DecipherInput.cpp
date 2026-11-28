#include "bombe/DecipherInput.hpp"

#include "Charactermap.hpp"

#include <algorithm>
#include <charconv>
#include <memory>
#include <span>
#include <stdexcept>
#include <system_error>

namespace bombe {
namespace {

constexpr std::string_view challengeVersion = "ENIGMA-CHALLENGE/1";
constexpr std::size_t challengeLimit = 1024;
constexpr std::array rotorTokens{
    std::pair{RotorScope::Fixed, "FIXED"},
    std::pair{RotorScope::FirstThree, "I-III"},
    std::pair{RotorScope::FirstFive, "I-V"}};
constexpr std::array reflectorTokens{
    std::pair{ReflectorScope::B, "B"}, std::pair{ReflectorScope::C, "C"},
    std::pair{ReflectorScope::Both, "BOTH"}};
constexpr std::array boardTokens{
    std::pair{PlugboardMode::Unknown, "UNKNOWN"},
    std::pair{PlugboardMode::Partial, "PARTIAL"},
    std::pair{PlugboardMode::Known, "KNOWN"}};

template <typename Value, std::size_t Size>
std::string_view tokenFor(Value value,
                          const std::array<std::pair<Value, const char *>, Size> &tokens) {
  for (const auto &[option, name] : tokens) {
    if (value == option) {
      return name;
    }
  }
  throw std::invalid_argument("Invalid challenge search option");
}

template <typename Value, std::size_t Size>
Value optionFor(std::string_view token,
                const std::array<std::pair<Value, const char *>, Size> &tokens) {
  for (const auto &[option, name] : tokens) {
    if (token == name) {
      return option;
    }
  }
  throw std::invalid_argument("Invalid challenge search option");
}

std::string_view readLine(std::string_view &text) {
  const auto end = text.find('\n');
  auto line = text.substr(0, end);
  text.remove_prefix(end == std::string_view::npos ? text.size() : end + 1);
  if (line.ends_with('\r')) {
    line.remove_suffix(1);
  }
  return line;
}

std::string_view readField(std::string_view &text, std::string_view prefix) {
  auto line = readLine(text);
  if (!line.starts_with(prefix)) {
    throw std::invalid_argument("Missing or unexpected challenge field");
  }
  line.remove_prefix(prefix.size());
  return line;
}

void validateChallenge(const SearchRequest &request) {
  const auto asciiLetters = [](std::string_view value) {
    return !value.empty() && value.size() <= enigma::messageLimit &&
           std::ranges::all_of(value, [](char ch) { return ch >= 'A' && ch <= 'Z'; });
  };
  if (!asciiLetters(request.ciphertext) || !asciiLetters(request.crib)) {
    throw std::invalid_argument("Challenge text and crib need 1–240 letters A–Z");
  }
  if (request.crib.size() >= request.ciphertext.size()) {
    throw std::invalid_argument("Choose a crib shorter than the message");
  }
  if (request.cribOffset &&
      *request.cribOffset > request.ciphertext.size() - request.crib.size()) {
    throw std::invalid_argument("Crib position is outside the ciphertext");
  }
  if (request.plugboard == PlugboardMode::Unknown && !request.knownLeads.empty()) {
    throw std::invalid_argument("Choose Known leads or Known board to share pairs");
  }
  enigma::MachineSettings key;
  std::ranges::copy(request.rings, key.rings.begin());
  key.plugPairs = request.knownLeads;
  enigma::makeMachine(key);
}

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
  const auto end = std::to_address(token.end());
  const auto [last, error] =
      std::from_chars(std::to_address(token.begin()), end, value);
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

std::string encodeChallenge(const SearchRequest &request) {
  validateChallenge(request);
  std::string payload(challengeVersion);
  payload += "\nCiphertext=" + request.ciphertext;
  payload += "\nCrib=" + request.crib;
  payload += "\nRings=" + formatRings(request.rings);
  payload += "\nPosition=";
  payload += request.cribOffset ? std::to_string(*request.cribOffset + 1) : "AUTO";
  payload += "\nRotors=";
  payload += tokenFor(request.rotors, rotorTokens);
  payload += "\nReflector=";
  payload += tokenFor(request.reflectors, reflectorTokens);
  payload += "\nPlugboard=";
  payload += tokenFor(request.plugboard, boardTokens);
  payload += "\nPairs=" + formatLeads(request.knownLeads);
  return payload;
}

SearchRequest decodeChallenge(std::string_view payload) {
  if (payload.size() > challengeLimit) {
    throw std::invalid_argument("Challenge text is too large");
  }
  if (readLine(payload) != challengeVersion) {
    throw std::invalid_argument("Clipboard needs an ENIGMA-CHALLENGE/1 puzzle");
  }
  const auto ciphertext = readField(payload, "Ciphertext=");
  const auto crib = readField(payload, "Crib=");
  const auto rings = readField(payload, "Rings=");
  const auto position = readField(payload, "Position=");
  const auto rotors = optionFor(readField(payload, "Rotors="), rotorTokens);
  const auto reflectors = optionFor(readField(payload, "Reflector="), reflectorTokens);
  const auto board = optionFor(readField(payload, "Plugboard="), boardTokens);
  const auto leads = readField(payload, "Pairs=");
  if (!payload.empty() || position.empty()) {
    throw std::invalid_argument("Unexpected challenge data");
  }
  auto request = parseSearch(std::string(ciphertext), std::string(crib), rings,
                             position == "AUTO" ? std::string_view{} : position,
                             board, leads);
  request.rotors = rotors;
  request.reflectors = reflectors;
  validateChallenge(request);
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
  return formatRings(std::span(settings.rings).first(settings.rotorCount));
}

std::string formatRings(std::span<const int> rings) {
  std::string text;
  for (const int offset : rings) {
    if (!text.empty()) {
      text += ' ';
    }
    const int ring = offset + 1;
    if (ring < 10) {
      text += '0';
    }
    text += std::to_string(ring);
  }
  return text;
}

std::string formatProfile(const enigma::MachineSettings &settings) {
  std::string text = "Rotors";
  for (std::size_t i = 0; i < settings.rotorCount; ++i) {
    const auto model = static_cast<std::size_t>(settings.rotorTypes[i]);
    text += ' ';
    text += enigma::rotorNames[model < enigma::rotorNames.size() ? model : i];
  }
  const auto reflector = static_cast<std::size_t>(settings.reflectorIndex);
  text += "  ·  ";
  text += enigma::reflectorNames[reflector < enigma::reflectorNames.size()
                                    ? reflector : 1];
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
