#pragma once

class Envelope {
public:
  Envelope() {}
  ~Envelope() {}

  void Init(float sr) {
    sr_ = sr;
    stageTime_ = 0.0f;
    stageTimeInc_ = 1.0f / sr_; // samples in one second
    stage_ = 0;
    attack_ = 0.1f; // seconds
    decay_ = 1.0f;  // seconds
    curve_ = 0.0f;
    scale_ = 1.0f;
    out_ = 0.0f;
  }

  // 0.001sec to 10sec
  void SetAttack(float attack) {
    attack_ = (attack < 0.001f) ? 0.001f : (attack > 5.0f ? 5.0f : attack);
  }

  void SetDecay(float decay) {
    decay_ = (decay < 0.001f) ? 0.001f : (decay > 5.0f ? 5.0f : decay);
  }

  void SetScale(float scale) {
    scale_ = (scale < 0.0f) ? 0.0f : (scale > 1.0f ? 1.0f : scale);
  }

  void Trigger() {
    if (out_ == 0.0f) {
      stageTime_ = 0.0f;
    } else {
      // retriggers, to avoid click
      // TODO make this an option, filter should not retrigger
      stageTime_ = out_ * attack_;
    }
    stage_ = 1;
  }

  float Process() {
    // attack
    if (stage_ == 1) {
      stageTime_ += stageTimeInc_;
      out_ = stageTime_ / attack_;
      // TODO apply curve here
      // end of attack, go to decay
      if (out_ >= 1.0f) {
        stageTime_ = 0.0f;
        stage_ = 2;
      }
    }

    if (stage_ == 2) {
      stageTime_ += stageTimeInc_;
      out_ = stageTime_ / decay_;
      out_ = 1.0f - out_;
      // TODO apply curve here
      // end of decay, stop
      if (out_ <= 0.0001f) {
        out_ = 0.0f;
        stage_ = 0;
      }
    }
    return out_ * scale_;
  }

  float GetAttack() { return attack_; }
  float GetDecay() { return decay_; }
  float GetScale() { return scale_; }

private:
  // Stage: OFF 0, ATTACK 1, DECAY 2
  float sr_, stageTime_, stageTimeInc_, stage_, attack_, decay_, curve_, scale_,
      out_;
};