#pragma once

class Envelope {
public:
  Envelope() {}
  ~Envelope() {}

  enum Stage { OFF = 0, ATTACK, DECAY };

  void Init(float sr) {
    sr_ = sr;
    stageTime_ = 0.0f;
    stageTimeInc_ = 1.0f / sr_; // samples in one second
    stage_ = OFF;
    attack_ = 0.1f;    // seconds
    addAttack_ = 0.0f; // seconds
    decay_ = 1.0f;     // seconds
    addDecay_ = 0.0f;  // seconds
    curve_ = 2.0f;
    scale_ = 1.0f;
    out_ = 0.0f;
  }

  // 0.001sec to 10sec
  void SetAttack(float attack) {
    attack_ = (attack < 0.001f) ? 0.001f : (attack > 5.0f ? 5.0f : attack);
  }

  void AddAttack(float addAttack) {
    addAttack_ =
        (addAttack < 0.0f) ? 0.0f : (addAttack > 5.0f ? 5.0f : addAttack);
  }

  void SetDecay(float decay) {
    decay_ = (decay < 0.001f) ? 0.001f : (decay > 5.0f ? 5.0f : decay);
  }

  void AddDecay(float addDecay) {
    addDecay_ = (addDecay < 0.0f) ? 0.0f : (addDecay > 5.0f ? 5.0f : addDecay);
  }

  void SetScale(float scale) {
    scale_ = (scale < 0.0f) ? 0.0f : (scale > 1.0f ? 1.0f : scale);
  }

  void SetCurve(float curve) {
    curve_ = (curve < 1.0f) ? 1.0f : (curve > 4.0f ? 4.0f : curve);
  }

  void Trigger() {
    if (out_ == 0.0f) {
      stageTime_ = 0.0f;
    } else {
      // retriggers, to avoid click
      // TODO make this an option, filter should not retrigger
      stageTime_ = out_ * (attack_ + addAttack_);
    }
    stage_ = ATTACK;
  }

  void Release() {
    if (stage_ != OFF && stage_ != DECAY) {
      stageTime_ = 0.0f;
      stage_ = DECAY;
    }
  }

  float Process() {
    // attack
    if (stage_ == ATTACK) {
      stageTime_ += stageTimeInc_;
      out_ = stageTime_ / (attack_ + addAttack_);
      out_ = powf(out_, curve_);
      // end of attack, go to decay
      if (out_ >= 1.0f) {
        stageTime_ = 0.0f;
        stage_ = DECAY;
      }
    }

    if (stage_ == DECAY) {
      stageTime_ += stageTimeInc_;
      out_ = stageTime_ / (decay_ + addDecay_);
      out_ = 1.0f - out_;
      out_ = powf(out_, curve_);
      // end of decay, stop
      if (out_ <= 0.0001f) {
        out_ = 0.0f;
        stage_ = OFF;
      }
    }
    return out_ * scale_;
  }

  float GetAttack() { return attack_; }
  float GetDecay() { return decay_; }
  float GetScale() { return scale_; }
  float GetCurve() { return curve_; }

private:
  // Stage: OFF 0, ATTACK 1, DECAY 2
  Stage stage_;
  float sr_, stageTime_, stageTimeInc_, attack_, addAttack_, decay_, addDecay_,
      curve_, scale_, out_;
};