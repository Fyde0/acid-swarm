#include "Filter.hpp"
#include <cfloat>
#include <cmath>

Filter::FilterCoeffs Filter::coeffTable_[Filter::coeffQSteps_]
                                        [Filter::coeffFreqSteps_];

void Filter::Init(float sr) {
  sr_ = sr;
  freqIndex_ = 0.5f;
  addFreqIndex_ = 0.0f;
  qIndex_ = 0.2f;
  out_ = 0.0f;

  // main filter
  y0_ = 0.0f;
  y1_ = 0.0f;
  y2_ = 0.0f;
  y3_ = 0.0f;
  y4_ = 0.0f;

  // feedback highpass
  y1hp_ = 0.0f;
  x1hp_ = 0.0f;
  // allpass
  y1ap_ = 0.0f;
  x1ap_ = 0.0f;
  // notch
  x1n_ = 0.0f;
  x2n_ = 0.0f;
  y1n_ = 0.0f;
  y2n_ = 0.0f;

  twoPiOverSampleRate = 2.0 * M_PI / sr_;

  // feedback highpass coefficients
  // it's always at 150Hz so we only need one set of coefficients
  float x = exp(-2.0 * M_PI * 150.0f * (1.0f / sr_));
  b0hp_ = 0.5 * (1 + x);
  b1hp_ = -0.5 * (1 + x);
  a1hp_ = x;

  // allpass coefficients
  // always at 14.008Hz
  float y = exp(-2.0 * M_PI * 14.008f * (1.0f / sr_));
  b0ap_ = 0.5 * (1 + x);
  b1ap_ = -0.5 * (1 + x);
  a1ap_ = y;

  // notch coefficients
  // frequency 7.5164Hz, bandwidth 4.7
  float w = 2 * M_PI * 7.5164 / sr_;
  float s, c;
  sinCos(w, &s, &c);
  double alpha = s * sinh(0.5 * log(2.0) * 4.7 * w / s);
  double scale = 1.0 / (1.0 + alpha);
  a1n_ = 2.0 * c * scale;
  a2n_ = (alpha - 1.0) * scale;
  b0n_ = 1.0 * scale;
  b1n_ = -2.0 * c * scale;
  b2n_ = 1.0 * scale;

  InitLookupTable();
}

float Filter::Process(float in) {

  FilterCoeffs coeffs = GetNearestCoeffs(freqIndex_ + addFreqIndex_, qIndex_);

  float tmp = in;

  // feedback highpass
  float hpin = coeffs.k * shape(y4_);
  y1hp_ = b0hp_ * hpin + b1hp_ * x1hp_ + a1hp_ * y1hp_ + FLT_MIN;
  x1hp_ = hpin;

  // main filter
  y0_ = -tmp - y1hp_;
  y1_ += 2 * coeffs.b0 * (y0_ - y1_ + y2_);
  y2_ += coeffs.b0 * (y1_ - 2 * y2_ + y3_);
  y3_ += coeffs.b0 * (y2_ - 2 * y3_ + y4_);
  y4_ += coeffs.b0 * (y3_ - 2 * y4_);
  tmp = 2 * coeffs.g * y4_;

  // allpass
  float apin = tmp;
  y1ap_ = b0ap_ * apin + b1ap_ * x1ap_ + a1ap_ * y1ap_ + FLT_MIN;
  x1ap_ = apin;
  tmp = y1ap_;

  // biquad notch
  float y = b0n_ * tmp + b1n_ * x1n_ + b2n_ * x2n_ + a1n_ * y1n_ + a2n_ * y2n_ +
            FLT_MIN;
  x2n_ = x1n_;
  x1n_ = tmp;
  y2n_ = y1n_;
  y1n_ = y;
  tmp = y;

  return tmp;
}

float Filter::shape(float x) {
  x = (x < -SQRT2) ? -SQRT2 : (x > SQRT2 ? SQRT2 : x);
  return x - r6_ * x * x * x;
}

void Filter::sinCos(float x, float *sinResult, float *cosResult) {
  *sinResult = sin(x);
  *cosResult = cos(x);
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

      float wc = twoPiOverSampleRate * freq;
      float wc2 = wc * wc;
      float r = (1.0 - exp(-3.0 * q)) / (1.0 - exp(-3.0));

      float pa12 = -1.341281325101042e-02;
      float pa11 = 8.168739417977708e-02;
      float pa10 = -2.365036766021623e-01;
      float pa09 = 4.439739664918068e-01;
      float pa08 = -6.297350825423579e-01;
      float pa07 = 7.529691648678890e-01;
      float pa06 = -8.249882473764324e-01;
      float pa05 = 8.736418933533319e-01;
      float pa04 = -9.164580250284832e-01;
      float pa03 = 9.583192455599817e-01;
      float pa02 = -9.999994950291231e-01;

      float tmp = wc2 * pa12 + pa11 * wc + pa10;
      tmp = wc2 * tmp + pa09 * wc + pa08;
      tmp = wc2 * tmp + pa07 * wc + pa06;
      tmp = wc2 * tmp + pa05 * wc + pa04;
      tmp = wc2 * tmp + pa03 * wc + pa02;

      float pr8 = -4.554677015609929e-05;
      float pr7 = -2.022131730719448e-05;
      float pr6 = 2.784706718370008e-03;
      float pr5 = 2.079921151733780e-03;
      float pr4 = -8.333236384240325e-02;
      float pr3 = -1.666668203490468e-01;
      float pr2 = 1.000000012124230e+00;
      float pr1 = 3.999999999650040e+00;
      float pr0 = 4.000000000000113e+00;
      tmp = wc2 * pr8 + pr7 * wc + pr6;
      tmp = wc2 * tmp + pr5 * wc + pr4;
      tmp = wc2 * tmp + pr3 * wc + pr2;
      tmp = wc2 * tmp + pr1 * wc + pr0;
      float k = r * tmp;
      float g = 1.0;

      float fx = wc * ONE_OVER_SQRT2 / (2 * M_PI);
      float b0 = (0.00045522346 + 6.1922189 * fx) /
                 (1.0 + 12.358354 * fx + 4.4156345 * (fx * fx));
      k = fx * (fx * (fx * (fx * (fx * (fx + 7198.6997) - 5837.7917) -
                            476.47308) +
                      614.95611) +
                213.87126) +
          16.998792;
      g = k * 0.058823529411764705882352941176471;
      g = (g - 1.0) * r + 1.0;
      g = (g * (1.0 + r));
      k = k * r;

      coeffTable_[qIndex][freqIndex].b0 = b0;
      coeffTable_[qIndex][freqIndex].k = k;
      coeffTable_[qIndex][freqIndex].g = g;
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
