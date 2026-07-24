#include "AudioEffectorChain.hpp"

void AudioEffectorChain::process(const float* inputBuffer, float* outputBuffer,
                                 int numSamples) {
  const float* currentInput = inputBuffer;
  float* currentOutput = outputBuffer;
  for (auto& effector : mEffectors) {
    effector->process(currentInput, currentOutput, numSamples);
    currentInput =
        currentOutput;  // Output of this effector becomes input for the next
  }
}

void AudioEffectorChain::setSampleRate(float sampleRate) {
  for (auto& effector : mEffectors) {
    effector->setSampleRate(sampleRate);
  }
}

void AudioEffectorChain::addEffector(std::shared_ptr<AudioEffector> effector) {
  mEffectors.push_back(effector);
}

void AudioEffectorChain::clearEffectors() { mEffectors.clear(); }

void AudioEffectorChain::setEnabled(bool enabled) {
  for (auto& effector : mEffectors) {
    effector->setEnabled(enabled);
  }
}

bool AudioEffectorChain::isEnabled() const {
  for (auto& effector : mEffectors) {
    if (effector->isEnabled()) return true;
  }
  return false;
}