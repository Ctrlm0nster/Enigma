/**
 * @file AudioEngine.hpp
 * @brief Thread-safe mechanical audio synthesizer for typewriter key clacks, rotor ratchets, and plug snaps.
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <memory>

namespace workbench {

enum class SoundEffect {
  KeyClick,   ///< Heavy mechanical typewriter keypress and spring snap.
  RotorStep,  ///< Metallic pawl ratchet turnover and rotor detent click.
  PlugSnap    ///< Banana plug contact friction and socket spring clamp.
};

class AudioEngine {
public:
  AudioEngine();
  ~AudioEngine();

  AudioEngine(const AudioEngine &) = delete;
  AudioEngine &operator=(const AudioEngine &) = delete;
  AudioEngine(AudioEngine &&) noexcept;
  AudioEngine &operator=(AudioEngine &&) noexcept;

  /** Initializes ALSA PCM output and synthesizes sound wave tables. Returns false if audio hardware is unavailable. */
  bool init();

  /** Shuts down the audio worker thread and closes the ALSA PCM device. */
  void shutdown();

  /** Queues a mechanical sound effect for immediate playback. Non-blocking and thread-safe. */
  void play(SoundEffect sfx);

  /** Toggles or sets the audio mute state. */
  void setMuted(bool muted);
  bool isMuted() const;

  /** Returns true if audio device was successfully opened and active. */
  bool isAvailable() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace workbench
