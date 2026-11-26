#include "Session.hpp"

#include "Charactermap.hpp"

#include <algorithm>

namespace workbench {
namespace {

struct RotorSpec {
  const char *wiring;
  char notch;
};

// Rotor wirings and turnover window letters for types I through V.
// https://enigmamuseum.com/rotwirg.htm
constexpr std::array<RotorSpec, maximumRotorCount> rotors{{
    {"EKMFLGDQVZNTOWYHXUSPAIBRCJ", 'Q'},
    {"AJDKSIRUXBLHWTMCQGZNPYFVOE", 'E'},
    {"BDFHJLCPRTXVZNYEIWGAKMUSQO", 'V'},
    {"ESOVPZJAYQUIRHXLNFTGKDCMWB", 'J'},
    {"VZBRGITYUPSDNHLXAWMJQOFECK", 'Z'},
}};
constexpr auto reflectorB = "YRUHQSLDPXNGOKMIEBFZCWVJAT";
constexpr std::size_t plugLimit = 10;

bool usesSocket(const std::pair<int, int> &pair, int letter) {
  return pair.first == letter || pair.second == letter;
}

} // namespace

Session::Session()
    : plugPairs_{{charToIndex('A'), charToIndex('Z')},
                 {charToIndex('B'), charToIndex('Y')}},
      machine_(makeMachine()) {}

EnigmaMachine Session::makeMachine() const {
  std::vector<Rotor> wheels;
  wheels.reserve(rotorCount_);
  for (std::size_t i = 0; i < rotorCount_; ++i) {
    Rotor rotor(rotors[i].wiring, rotors[i].notch, rings_[i]);
    rotor.setPosition(starts_[i]);
    wheels.push_back(rotor);
  }
  Plugboard plugboard;
  for (const auto &[first, second] : plugPairs_) {
    plugboard.addWire(first, second);
  }
  return EnigmaMachine(plugboard, std::move(wheels), Reflector(reflectorB));
}

void Session::encryptLetter(char letter) {
  trace_ = machine_.processCharWithTrace(letter);
  output_.push_back(trace_->output);
}

void Session::replay() {
  machine_ = makeMachine();
  output_.clear();
  trace_.reset();
  for (char letter : input_) {
    encryptLetter(letter);
  }
}

bool Session::typeLetter(char letter) {
  if (input_.size() >= messageLimit) {
    return false;
  }
  input_.push_back(letter);
  encryptLetter(letter);
  return true;
}

std::size_t Session::paste(std::string_view text) {
  const auto remaining = messageLimit - input_.size();
  if (remaining == 0) {
    return 0;
  }

  // The view may refer to either tape, which typing can reallocate.
  std::string accepted;
  accepted.reserve(std::min(remaining, text.size()));
  for (char character : text) {
    if (character >= 'a' && character <= 'z') {
      character = static_cast<char>(character + ('A' - 'a'));
    }
    if (character >= 'A' && character <= 'Z') {
      accepted.push_back(character);
      if (accepted.size() == remaining) {
        break;
      }
    }
  }
  for (const char letter : accepted) {
    typeLetter(letter);
  }
  return accepted.size();
}

void Session::backspace() {
  if (!input_.empty()) {
    input_.pop_back();
    replay();
  }
}

void Session::clearMessage() {
  input_.clear();
  replay();
}

void Session::reset() { *this = Session{}; }

bool Session::setRotorCount(std::size_t count) {
  if (count < minimumRotorCount || count > maximumRotorCount ||
      count == rotorCount_) {
    return false;
  }
  rotorCount_ = count;
  replay();
  return true;
}

void Session::adjustRotor(std::size_t rotor, int startDelta, int ringDelta) {
  starts_[rotor] = (starts_[rotor] + startDelta + 26) % 26;
  rings_[rotor] = (rings_[rotor] + ringDelta + 26) % 26;
  replay();
}

int Session::positionAt(std::size_t rotor) const {
  return machine_.getRotors()[rotor].getPosition();
}

bool Session::isPlugConnected(int letter) const {
  return std::ranges::any_of(plugPairs_, [letter](const auto &pair) {
    return usesSocket(pair, letter);
  });
}

void Session::disconnectPlug(int letter) {
  std::erase_if(plugPairs_, [letter](const auto &pair) {
    return usesSocket(pair, letter);
  });
}

PlugAction Session::selectPlug(int letter) {
  if (!selectedPlug_) {
    if (isPlugConnected(letter)) {
      disconnectPlug(letter);
      replay();
      return PlugAction::Removed;
    }
    selectedPlug_ = letter;
    return PlugAction::Selected;
  }

  const int first = *selectedPlug_;
  selectedPlug_.reset();
  if (first == letter) {
    return PlugAction::Cancelled;
  }
  if (plugPairs_.size() >= plugLimit && !isPlugConnected(letter)) {
    return PlugAction::LimitReached;
  }

  disconnectPlug(letter);
  plugPairs_.emplace_back(first, letter);
  replay();
  return PlugAction::Connected;
}

void Session::clearPlugboard() {
  plugPairs_.clear();
  selectedPlug_.reset();
  replay();
}

} // namespace workbench
