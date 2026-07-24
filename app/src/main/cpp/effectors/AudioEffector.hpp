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
  virtual void process(const float* inputBuffer, float* outputBuffer,
                       int numSamples) = 0;

  /**
   * @brief Sets the sample rate for the audio effector.
   *
   * This method can be overridden by derived classes that require sample rate
   * information. The default implementation does nothing.
   *
   * @param sampleRate The sample rate in Hz.
   *
   */
  virtual void setSampleRate(float sampleRate) = 0;

  /**
   * @brief Enables or disables the audio effector.
   *
   * @param enabled True to enable, false to disable.
   */
  virtual void setEnabled(bool enabled) = 0;

  /**
   * @brief Checks if the audio effector is enabled.
   *
   * @return True if enabled, false otherwise.
   */
  virtual bool isEnabled() const = 0;
};

#endif  // AUDIO_EFFECTOR_HPP
