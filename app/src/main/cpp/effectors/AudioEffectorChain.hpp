#ifndef AUDIO_EFFECTOR_CHAIN_HPP
#define AUDIO_EFFECTOR_CHAIN_HPP

#include <memory>
#include <vector>

#include "AudioEffector.hpp"

/**
 * @brief A chain of audio effectors that processes audio in sequence.
 *
 * This class allows multiple AudioEffector instances to be applied in order,
 * where the output of one effector becomes the input to the next.
 */
class AudioEffectorChain : public AudioEffector {
 public:
  void process(const float* inputBuffer, float* outputBuffer,
               int numSamples) override;
  void setSampleRate(float sampleRate) override;
  void setEnabled(bool enabled) override;
  bool isEnabled() const override;
  void addEffector(std::shared_ptr<AudioEffector> effector);

  void clearEffectors();

 private:
  std::vector<std::shared_ptr<AudioEffector>> mEffectors;
};

#endif  // AUDIO_EFFECTOR_CHAIN_HPP