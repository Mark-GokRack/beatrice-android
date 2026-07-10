#ifndef AUDIO_EFFECTOR_HPP
#define AUDIO_EFFECTOR_HPP

/**
 * @brief Abstract base class for all audio effect processors.
 *
 * Defines the common interface for audio effect classes.
 * All audio effectors must implement the process() method.
 */
class AudioEffector {
 public:
  virtual ~AudioEffector() = default;

  /**
   * @brief Processes a buffer of audio samples.
   *
   * @param buffer Pointer to the audio buffer.
   * @param numSamples Number of samples in the buffer.
   */
  virtual void process(float* buffer, int numSamples) = 0;
};

#endif  // AUDIO_EFFECTOR_HPP
