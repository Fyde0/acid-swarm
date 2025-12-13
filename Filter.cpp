#include "Filter.hpp"

// I don't even know where to start commenting this, watch this:
// https://www.youtube.com/playlist?list=PLbqhA-NKGP6Afr_KbPUuy_yIBpPR4jzWo

Filter::FilterCoeffs Filter::coeffTable_[Filter::coeffQSteps_]
                                        [Filter::coeffFreqSteps_];

void Filter::Init(float sr) {
  sr_ = sr;
  freqIndex_ = 0.5f;
  addFreqIndex_ = 0.0f;
  qIndex_ = 0.2f;
  out_ = 0.0f;
  x[0] = x[1] = x[2] = 0.0;
  y[0] = y[1] = y[2] = 0.0;
  InitLookupTable();
}

float Filter::Process(float in) {
  FilterCoeffs coeffs = GetNearestCoeffs(freqIndex_ + addFreqIndex_, qIndex_);

  x[2] = x[1];
  x[1] = x[0];
  x[0] = in;

  y[2] = y[1];
  y[1] = y[0];
  y[0] = coeffs.a0 * x[0];
  y[0] += coeffs.a1 * x[1];
  y[0] += coeffs.a2 * x[2];
  y[0] -= coeffs.b1 * y[1];
  y[0] -= coeffs.b2 * y[2];

  out_ = y[0];

  return out_;
}

void Filter::SetFreq(float freqIndex) {
  freqIndex = (freqIndex < 0) ? 0 : (freqIndex > 1.0f ? 1.0f : freqIndex);
  freqIndex_ = freqIndex;
}

void Filter::AddFreq(float freqIndex) {
  // not clamping here because it already happens in GetNearestCoeffs
  addFreqIndex_ = freqIndex;
}

void Filter::SetQ(float qIndex) {
  qIndex = (qIndex < 0) ? 0 : (qIndex > 1.0f ? 1.0f : qIndex);
  qIndex_ = qIndex;
}

float Filter::GetFreq() {
  float freq = minFreq_ * powf(maxFreq_ / minFreq_, freqIndex_);
  return freq;
}
float Filter::GetQ() { return minQ_ + (maxQ_ - minQ_) * qIndex_; }

void Filter::InitLookupTable() {
  for (int qIndex = 0; qIndex < coeffQSteps_; ++qIndex) {
    float q = minQ_ + (maxQ_ - minQ_) * (float(qIndex) / (coeffQSteps_ - 1));

    for (int freqIndex = 0; freqIndex < coeffFreqSteps_; ++freqIndex) {
      float fT = float(freqIndex) / (coeffFreqSteps_ - 1);
      float freq = minFreq_ * powf(maxFreq_ / minFreq_, fT);

      float w0 = 2.0f * PI_F * (freq / sr_);
      float cosw0 = cos(w0);
      float alpha = sin(w0) / (2.0f * q);

      float b0 = 1.0f + alpha;
      float ib0 = 1.0f / b0;

      coeffTable_[qIndex][freqIndex].a0 = ((1.0f - cosw0) / 2.0f) * ib0;
      coeffTable_[qIndex][freqIndex].a1 = (1.0f - cosw0) * ib0;
      coeffTable_[qIndex][freqIndex].a2 = ((1.0f - cosw0) / 2.0f) * ib0;

      coeffTable_[qIndex][freqIndex].b1 = (-2.0f * cosw0) * ib0;
      coeffTable_[qIndex][freqIndex].b2 = (1.0f - alpha) * ib0;
    }
  }
}

Filter::FilterCoeffs Filter::GetNearestCoeffs(float freq, float q) {
  // clamp is necessary because envelope makes freq go above 1
  freq = (freq < 0) ? 0 : (freq > 1.0f ? 1.0f : freq);
  // scale to index
  // could interpolate here but I don't think it's necessary
  int fi = static_cast<int>(freq * (coeffFreqSteps_ - 1));
  int qi = static_cast<int>(q * (coeffQSteps_ - 1));
  return coeffTable_[qi][fi];
}