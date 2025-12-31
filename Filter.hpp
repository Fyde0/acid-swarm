#pragma once

#include <cmath>

#define SQRT2 1.4142135623730950488016887242097
#define ONE_OVER_SQRT2 0.70710678118654752440084436210485

class Filter {
public:
  Filter() {}
  ~Filter() {}

  // Call before using
  void Init(float sr);
  // Get next sample
  float Process(float in);

  // Set frequency index (0 to 1)
  void SetFreq(float freq);
  // Set Q index (0 to 1)
  void SetQ(float q);
  // Value to add to frequency (0 to 1), eg for envelope
  void AddFreq(float freq);

  float GetFreq();
  float GetQ();

private:
  const float minFreq_ = 200.0f;
  const float maxFreq_ = 20000.0f;
  const float minQ_ = 0.0f;
  const float maxQ_ = 0.95f;
  float sr_, freqIndex_, addFreqIndex_, qIndex_, out_;
  float twoPiOverSampleRate;

  float y0_, y1_, y2_, y3_, y4_;           // for main filter
  float y1hp_, x1hp_, b0hp_, b1hp_, a1hp_; // for feedback highpass
  float y1ap_, x1ap_, b0ap_, b1ap_, a1ap_; // for allpass
  float b0n_, b1n_, b2n_, x1n_, x2n_, y1n_, y2n_, a1n_, a2n_; // notch

  const float r6_ = 1.0 / 6.0;
  float shape(float x);

  // linear interpolation
  inline float lerp(float a, float b, float t);

  // filter coefficients lookup table
  // lookup table size
  static constexpr int coeffFreqSteps_ = 384;
  static constexpr int coeffQSteps_ = 64;
  // table struct
  struct FilterCoeffs {
    float b0, k, g;
  };
  // table
  static FilterCoeffs coeffTable_[coeffQSteps_][coeffFreqSteps_];
  // generate lookup table
  void InitLookupTable();
  // get coefficients from index
  FilterCoeffs GetInterpolatedCoeffs(float freqIndex, float qIndex);
};