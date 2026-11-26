#pragma once

#include "EnigmaMachine.hpp"

#include <array>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace workbench {

inline constexpr std::size_t rotorCount = 3;
inline constexpr std::size_t messageLimit = 240;

enum class PlugAction { Selected, Cancelled, Removed, Connected, LimitReached };

class Session {
public:
  Session();

  bool typeLetter(char letter);
  void backspace();
  void clearMessage();
  void reset();
  void adjustRotor(std::size_t rotor, int startDelta, int ringDelta);
  PlugAction selectPlug(int letter);
  void clearPlugboard();

  const std::string &input() const { return input_; }
  const std::string &output() const { return output_; }
  const std::optional<SignalTrace> &trace() const { return trace_; }
  const auto &plugPairs() const { return plugPairs_; }
  std::optional<int> selectedPlug() const { return selectedPlug_; }
  bool isPlugConnected(int letter) const;
  int startAt(std::size_t rotor) const { return starts_[rotor]; }
  int ringAt(std::size_t rotor) const { return rings_[rotor]; }
  int positionAt(std::size_t rotor) const;

private:
  std::array<int, rotorCount> starts_{};
  std::array<int, rotorCount> rings_{};
  std::vector<std::pair<int, int>> plugPairs_;
  EnigmaMachine machine_;
  std::string input_;
  std::string output_;
  std::optional<SignalTrace> trace_;
  std::optional<int> selectedPlug_;

  EnigmaMachine makeMachine() const;
  void replay();
  void encryptLetter(char letter);
  void disconnectPlug(int letter);
};

} // namespace workbench
