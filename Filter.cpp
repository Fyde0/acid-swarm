#include "Filter.hpp"

Filter::FilterCoeffs Filter::coeffTable_[Filter::coeffQSteps_]
                                        [Filter::coeffFreqSteps_];

void Filter::Init(float sr) {
  sr_ = sr;
  freqIndex_ = 0.5f;
  addFreqIndex_ = 0.0f;
  qIndex_ = 0.2f;
  out_ = 0.0f;
  s1_ = 0.0f;
  s2_ = 0.0f;
  lp3_ = 0.0f;
  InitLookupTable();
}

float Filter::Process(float in) {
  FilterCoeffs coeffs = GetNearestCoeffs(freqIndex_ + addFreqIndex_, qIndex_);

  out_ = in * coeffs.gainComp * 0.3f;
  float fb = tanhf(s1_ * 2.0f);

  float hp =
      (out_ - fb * coeffs.R - s2_) / (1.0f + coeffs.g * (coeffs.g + coeffs.R));
  float bp = coeffs.g * hp + s1_;
  float lp = coeffs.g * bp + s2_;

  s1_ = bp;
  s2_ = lp;

  float g3 = coeffs.fc / sr_;
  lp3_ += g3 * (lp - lp3_);

  out_ = lp3_;

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

      float gainComp = 1.0f - 0.6f * q;
      float fc = freq * (1.0f - 0.2f * q);
      float g = tanf(M_PI * fc / sr_);
      float R = 1.0f - q;

      coeffTable_[qIndex][freqIndex].gainComp = gainComp;
      coeffTable_[qIndex][freqIndex].fc = fc;
      coeffTable_[qIndex][freqIndex].g = g;
      coeffTable_[qIndex][freqIndex].R = R;
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