#pragma once

#include <algorithm>

class Oscillator {
public:
  Oscillator() {}
  ~Oscillator() {}

  void Init(float sr) {
    sr_ = sr;
    amp_ = 0.5f;
    detune_ = 0.0f;
    std::fill(freqs_, freqs_ + 7, 440.0f);
    std::fill(phases_, phases_ + 7, 0.0f);
    calcDetuneRatio();
    calcPhaseIncs();
  }

  void SetNote(int n) {
    baseFreq_ = 440.0f * powf(2.0f, (n - 69.0f) / 12.0f);
    for (int i = 0; i < 7; i++) {
      freqs_[i] = baseFreq_ * detuneRatio_[i];
    }
    calcPhaseIncs();
  }

  void SetAmp(float a) { amp_ = a; }

  void SetDetune(float d) {
    detune_ = (d < 0.0f) ? 0.00f : (d > 1.0f ? 1.0f : d);
    calcDetuneRatio();
  }

  float GetDetune() { return detune_; }

  void Process(float *out1, float *out2) {

    *out1 = 0.0f;
    *out2 = 0.0f;

    for (int i = 0; i < 7; i++) {
      saws_[i] = (2.0f * phases_[i]) - 1.0f;
      saws_[i] -= polyBLEP(phases_[i], phaseIncs_[i]);
    }

    for (int i = 0; i < 7; i++) {
      *out1 += saws_[i] * (1.0f - pans_[i]) * 0.5f * norm_;
      *out2 += saws_[i] * (1.0f + pans_[i]) * 0.5f * norm_;
    }

    for (int i = 0; i < 7; i++) {
      phases_[i] += phaseIncs_[i];
      if (phases_[i] > 1.0f) {
        phases_[i] -= 1.0f;
      }
    }

    *out1 = *out1 * amp_;
    *out2 = *out2 * amp_;
  }

private:
  float sr_, amp_, baseFreq_;
  float norm_ = 1 / sqrt(7);
  float freqs_[7], phases_[7], phaseIncs_[7];
  float detune_;
  float detuneCents_[7] = {0, -3, 3, -7, 7, -12, 12};
  float detuneRatio_[7];
  float pans_[7] = {0, -0.33f, 0.33f, -0.66f, 0.66f, -1.0f, 1.0f};

  void calcPhaseIncs() {
    for (int i = 0; i < 7; i++) {
      phaseIncs_[i] = freqs_[i] * (1.0f / sr_);
    }
  }

  void calcDetuneRatio() {
    for (int i = 0; i < 7; i++) {
      detuneRatio_[i] = powf(2.0f, (detuneCents_[i] * detune_) / 1200.0f);
    }
  }

  float saws_[7];

  float t, dt;
  float polyBLEP(float phase, float phaseInc) {
    // t is usually divided by 2pi because
    // it usually goes from 0 to 2pi, but here it
    // goes from 0 to 1, I guess?
    // It doesn't work if I use 2pi
    t = phase;
    dt = phaseInc;
    // beginning of wave
    if (t < dt) {
      t /= dt;
      return t + t - t * t - 1.0f; // adds some sort of smoothing?
    } // don't really understand
    // end of wave
    else if (t > 1.0f - dt) {
      t = (t - 1.0f) / dt;
      return t * t + t + t + 1.0f;
    } else {
      return 0.0f;
    }
  }
};