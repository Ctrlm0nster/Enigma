/**
 * @file Session.hpp
 * @brief Interactive workbench state management and machine replay.
 *
 * Manages user text input, ciphertext tape, rotor initial positions,
 * ring offsets, plugboard wire selections, and full message recomputation.
 */

#pragma once

#include "EnigmaMachine.hpp"

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace workbench {

inline constexpr std::array rotorNames{"I", "II", "III", "IV", "V"};
inline constexpr std::size_t minimumRotorCount = 3;
inline constexpr std::size_t maximumRotorCount = rotorNames.size();
inline constexpr std::size_t messageLimit = 240;  ///< Maximum characters on the message tape.

/**
 * @enum PlugAction
 * @brief Outcome of a user plugboard socket interaction.
 */
enum class PlugAction {
  Selected,     ///< First letter of a pair selected; waiting for second socket.
  Cancelled,    ///< Selected letter deselected.
  Removed,      ///< Existing connected lead disconnected.
  Connected,    ///< New pair successfully patched.
  LimitReached  ///< Maximum 10 lead pairs already connected.
};

/**
 * @class Session
 * @brief Encapsulates an interactive user session.
 *
 * Keeps track of tape messages, user settings (starts and rings), plugboard pairs,
 * and maintains message consistency by replaying text whenever settings change.
 */
class Session {
public:
  /**
   * @brief Constructs a session with standard default configuration (Rotors I, II, III).
   */
  Session();

  /**
   * @brief Appends a letter to the tape and encrypts it through the machine.
   *
   * @param letter Uppercase ASCII character.
   * @return true if appended, false if message limit reached.
   */
  bool typeLetter(char letter);

  /**
   * @brief Pastes a block of text, extracting and encrypting valid letters up to message limit.
   *
   * @param text String view containing text to paste.
   * @return Number of letters successfully accepted.
   */
  std::size_t paste(std::string_view text);

  /**
   * @brief Removes the last typed character and rewinds the machine state.
   */
  void backspace();

  /**
   * @brief Clears current input and output messages without resetting machine settings.
   */
  void clearMessage();

  /**
   * @brief Resets all rotors, rings, and message state back to initial defaults.
   */
  void reset();

  /** Changes the rotor count and replays the tape. Inactive rotor settings are saved. */
  bool setRotorCount(std::size_t count);
  std::size_t rotorCount() const { return rotorCount_; }

  /**
   * @brief Adjusts a rotor's starting position or ring setting.
   *
   * Automatically triggers a message replay so existing ciphertext updates immediately.
   *
   * @param rotor Index of an active rotor.
   * @param startDelta Offset to add to starting position (-1 or +1).
   * @param ringDelta Offset to add to ring setting (-1 or +1).
   */
  void adjustRotor(std::size_t rotor, int startDelta, int ringDelta);

  /**
   * @brief Handles user click on a plugboard socket.
   *
   * @param letter 0-indexed letter index [0..25].
   * @return Resulting PlugAction status.
   */
  PlugAction selectPlug(int letter);

  /**
   * @brief Disconnects all active plugboard leads and replays the message.
   */
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
  std::size_t rotorCount_ = minimumRotorCount;
  std::array<int, maximumRotorCount> starts_{}; ///< Initial starting positions for rotors.
  std::array<int, maximumRotorCount> rings_{};  ///< Ringstellung offsets.
  std::vector<std::pair<int, int>> plugPairs_; ///< Active plugboard wire pairs.
  EnigmaMachine machine_;                      ///< Current live machine instance.
  std::string input_;                          ///< Plaintext tape.
  std::string output_;                         ///< Ciphertext tape.
  std::optional<SignalTrace> trace_;           ///< Signal trace for most recent keystroke.
  std::optional<int> selectedPlug_;            ///< Currently pending plug socket selection.

  EnigmaMachine makeMachine() const;
  void replay();
  void encryptLetter(char letter);
  void disconnectPlug(int letter);
};

} // namespace workbench
