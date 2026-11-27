#include "AudioEngine.hpp"

#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <mutex>
#include <numbers>
#include <queue>
#include <thread>
#include <vector>

#if defined(ENIGMA_HAS_ALSA)
#include <alsa/asoundlib.h>

extern "C" void silentAlsaErrorHandler(const char *, int, const char *, int, const char *, ...) {}
#endif

namespace workbench {
namespace {

constexpr unsigned int sampleRate = 44100;

std::vector<int16_t> synthesizeKeyClick() {
  // ~36ms duration of typewriter mechanical strike
  constexpr std::size_t sampleCount = static_cast<std::size_t>(sampleRate * 0.036);
  std::vector<int16_t> samples(sampleCount, 0);
  constexpr float twoPi = 2.0F * std::numbers::pi_v<float>;

  // PRNG state for transient noise
  uint32_t rng = 0x89ABCDEF;
  const auto randFloat = [&rng]() -> float {
    rng ^= rng << 13U;
    rng ^= rng >> 17U;
    rng ^= rng << 5U;
    return static_cast<float>(rng & 0xFFFFU) / 32768.0F - 1.0F;
  };

  for (std::size_t i = 0; i < sampleCount; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(sampleRate);
    // Component 1: Initial sharp impact transient (0-4ms)
    const float impactEnv = std::exp(-t / 0.0035F);
    const float impactNoise = randFloat() * impactEnv;

    // Component 2: Metallic typebar ping (1850 Hz and 2420 Hz)
    const float pingEnv = std::exp(-t / 0.009F);
    const float ping1 = std::sin(twoPi * 1850.0F * t);
    const float ping2 = std::sin(twoPi * 2420.0F * t) * 0.6F;
    const float ping = (ping1 + ping2) * pingEnv;

    // Component 3: Typewriter key lever bottom-out thump (280 Hz)
    const float thumpEnv = std::exp(-t / 0.022F);
    const float thump = std::sin(twoPi * 280.0F * t) * thumpEnv * 0.8F;

    const float mixed = (impactNoise * 0.45F + ping * 0.35F + thump * 0.40F);
    const float clamped = std::clamp(mixed, -1.0F, 1.0F);
    samples[i] = static_cast<int16_t>(clamped * 24000.0F);
  }
  return samples;
}

std::vector<int16_t> synthesizeRotorStep() {
  // ~48ms duration of pawl ratchet turnover
  constexpr std::size_t sampleCount = static_cast<std::size_t>(sampleRate * 0.048);
  std::vector<int16_t> samples(sampleCount, 0);
  constexpr float twoPi = 2.0F * std::numbers::pi_v<float>;

  uint32_t rng = 0x12345678;
  const auto randFloat = [&rng]() -> float {
    rng ^= rng << 13U;
    rng ^= rng >> 17U;
    rng ^= rng << 5U;
    return static_cast<float>(rng & 0xFFFFU) / 32768.0F - 1.0F;
  };

  // Secondary latch click delay (~16ms)
  constexpr float latchDelay = 0.016F;

  for (std::size_t i = 0; i < sampleCount; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(sampleRate);
    
    // First pawl strike on bronze tooth
    const float pawlEnv = std::exp(-t / 0.005F);
    const float pawlTone = std::sin(twoPi * 1150.0F * t) + randFloat() * 0.35F;
    const float pawl = pawlTone * pawlEnv;

    // Secondary spring-loaded detent click falling into notch
    float latch = 0.0F;
    if (t >= latchDelay) {
      const float dt = t - latchDelay;
      const float latchEnv = std::exp(-dt / 0.007F);
      latch = (std::sin(twoPi * 1580.0F * dt) + randFloat() * 0.4F) * latchEnv * 0.85F;
    }

    // Heavy rotor body resonance (140 Hz)
    const float wheelBody = std::sin(twoPi * 140.0F * t) * std::exp(-t / 0.035F) * 0.5F;

    const float mixed = pawl * 0.50F + latch * 0.45F + wheelBody * 0.35F;
    const float clamped = std::clamp(mixed, -1.0F, 1.0F);
    samples[i] = static_cast<int16_t>(clamped * 22000.0F);
  }
  return samples;
}

std::vector<int16_t> synthesizePlugSnap() {
  // ~28ms duration of banana plug insertion contact
  constexpr std::size_t sampleCount = static_cast<std::size_t>(sampleRate * 0.028);
  std::vector<int16_t> samples(sampleCount, 0);
  constexpr float twoPi = 2.0F * std::numbers::pi_v<float>;

  uint32_t rng = 0xCAFEBABE;
  const auto randFloat = [&rng]() -> float {
    rng ^= rng << 13U;
    rng ^= rng >> 17U;
    rng ^= rng << 5U;
    return static_cast<float>(rng & 0xFFFFU) / 32768.0F - 1.0F;
  };

  for (std::size_t i = 0; i < sampleCount; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(sampleRate);
    // Friction slide on contacts
    const float slideEnv = std::exp(-t / 0.008F);
    const float friction = randFloat() * slideEnv * 0.5F;

    // Contact spring snap
    const float snapEnv = std::exp(-t / 0.004F);
    const float snapTone = std::sin(twoPi * 2180.0F * t) * snapEnv;

    const float mixed = (friction * 0.5F + snapTone * 0.5F);
    const float clamped = std::clamp(mixed, -1.0F, 1.0F);
    samples[i] = static_cast<int16_t>(clamped * 20000.0F);
  }
  return samples;
}

} // namespace

struct AudioEngine::Impl {
  std::atomic<bool> muted{false};
  std::atomic<bool> available{false};
  std::atomic<bool> running{false};

  std::vector<int16_t> keyWave;
  std::vector<int16_t> rotorWave;
  std::vector<int16_t> plugWave;

#if defined(ENIGMA_HAS_ALSA)
  snd_pcm_t *pcmHandle{nullptr};
#endif

  std::mutex queueMutex;
  std::condition_variable queueCv;
  std::queue<SoundEffect> pendingEffects;
  std::jthread worker;

  Impl() {
    keyWave = synthesizeKeyClick();
    rotorWave = synthesizeRotorStep();
    plugWave = synthesizePlugSnap();
  }

  ~Impl() {
    stop();
  }

  void stop() {
    running = false;
    queueCv.notify_all();
    if (worker.joinable()) {
      worker.request_stop();
      worker.join();
    }
#if defined(ENIGMA_HAS_ALSA)
    if (pcmHandle) {
      snd_pcm_close(pcmHandle);
      pcmHandle = nullptr;
    }
    snd_config_update_free_global();
#endif
    available = false;
  }
};

AudioEngine::AudioEngine() : impl_(std::make_unique<Impl>()) {}

AudioEngine::~AudioEngine() = default;

AudioEngine::AudioEngine(AudioEngine &&) noexcept = default;
AudioEngine &AudioEngine::operator=(AudioEngine &&) noexcept = default;

bool AudioEngine::init() {
#if defined(ENIGMA_HAS_ALSA)
  // Silence ALSA error messages in stderr so missing soundcards do not clutter logs
  snd_lib_error_set_handler(silentAlsaErrorHandler);

  snd_pcm_t *handle = nullptr;
  // Non-blocking open to prevent hanging on occupied devices
  const int err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
  if (err < 0 || !handle) {
    snd_config_update_free_global();
    impl_->available = false;
    return false;
  }

  // Configure mono 16-bit 44.1kHz with 40ms buffer
  if (snd_pcm_set_params(handle, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED,
                         1, sampleRate, 1, 40000) < 0) {
    snd_pcm_close(handle);
    snd_config_update_free_global();
    impl_->available = false;
    return false;
  }

  impl_->pcmHandle = handle;
  impl_->available = true;
  impl_->running = true;

  impl_->worker = std::jthread([this](std::stop_token stopToken) {
    std::vector<int16_t> mixBuffer(sampleRate / 10, 0); // 100ms chunk

    while (!stopToken.stop_requested() && impl_->running) {
      std::vector<SoundEffect> active;
      {
        std::unique_lock lock(impl_->queueMutex);
        impl_->queueCv.wait_for(lock, std::chrono::milliseconds(20), [&] {
          return !impl_->pendingEffects.empty() || stopToken.stop_requested();
        });
        while (!impl_->pendingEffects.empty()) {
          active.push_back(impl_->pendingEffects.front());
          impl_->pendingEffects.pop();
        }
      }

      if (stopToken.stop_requested()) {
        break;
      }

      if (active.empty() || impl_->muted.load()) {
        continue;
      }

      // Mix active sound effects
      for (auto sfx : active) {
        const std::vector<int16_t> *wave = nullptr;
        switch (sfx) {
          case SoundEffect::KeyClick: wave = &impl_->keyWave; break;
          case SoundEffect::RotorStep: wave = &impl_->rotorWave; break;
          case SoundEffect::PlugSnap: wave = &impl_->plugWave; break;
        }
        if (!wave || wave->empty()) {
          continue;
        }

        snd_pcm_sframes_t frames = static_cast<snd_pcm_sframes_t>(wave->size());
        const int16_t *data = wave->data();
        while (frames > 0 && !stopToken.stop_requested()) {
          snd_pcm_sframes_t written = snd_pcm_writei(impl_->pcmHandle, data, static_cast<snd_pcm_uframes_t>(frames));
          if (written == -EAGAIN) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
          }
          if (written < 0) {
            snd_pcm_recover(impl_->pcmHandle, static_cast<int>(written), 0);
            break;
          }
          frames -= written;
          data += written;
        }
      }
    }
  });

  return true;
#else
  impl_->available = false;
  return false;
#endif
}

void AudioEngine::shutdown() {
  if (impl_) {
    impl_->stop();
  }
}

void AudioEngine::play(SoundEffect sfx) {
  if (!impl_ || !impl_->available.load() || impl_->muted.load()) {
    return;
  }
  {
    std::lock_guard lock(impl_->queueMutex);
    impl_->pendingEffects.push(sfx);
  }
  impl_->queueCv.notify_one();
}

void AudioEngine::setMuted(bool muted) {
  if (impl_) {
    impl_->muted = muted;
  }
}

bool AudioEngine::isMuted() const {
  return impl_ ? impl_->muted.load() : true;
}

bool AudioEngine::isAvailable() const {
  return impl_ ? impl_->available.load() : false;
}

} // namespace workbench
