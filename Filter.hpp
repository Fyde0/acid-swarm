#pragma once

#include "utilities.hpp"

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
  const float minFreq_ = 20.0f;
  const float maxFreq_ = 20000.0f;
  const float minQ_ = 0.2f;
  const float maxQ_ = 5.0f;
  float sr_, freqIndex_, addFreqIndex_, qIndex_, out_;

  float x[3]{};
  float y[3]{};

  float b0, ib0;
  float w0, cosw0, alpha;

  // filter coefficients lookup table
  // lookup table size
  static constexpr int coeffFreqSteps_ = 512;
  static constexpr int coeffQSteps_ = 32;
  // table struct
  struct FilterCoeffs {
    float a0, a1, a2;
    float b1, b2;
  };
  // table
  static FilterCoeffs coeffTable_[coeffQSteps_][coeffFreqSteps_];
  // generate lookup table
  void InitLookupTable();
  // get coefficients from index
  FilterCoeffs GetNearestCoeffs(float freqIndex, float qIndex);
};