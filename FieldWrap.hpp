#pragma once

#include <daisy_field.h>

using namespace daisy;

class FieldWrap {
public:
  FieldWrap() {}

  void Init(AudioHandle::AudioCallback cb) {
    field_.Init();
    field_.SetAudioBlockSize(1);
    field_.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_96KHZ);
    field_.StartAdc();
    field_.StartAudio(cb);
    // zero LEDs
    field_.led_driver.SwapBuffersAndTransmit();
  }

  /**
   * DISPLAY
   */
  void ClearDisplay() { field_.display.Fill(false); }
  void UpdateDisplay() { field_.display.Update(); }

  /**
   * Prints a char* to screen
   *
   * @param text Text to print as char*
   * @param x X position as uint8_t
   * @param y Y position as uint8_t
   * @param color true = white on black
   */
  void PrintToScreen(const char *text, uint8_t x, uint8_t y,
                     bool color = true) {
    FixedCapStr<16> str(text);
    PrintFixedCapStrToScreen(str, x, y);
  }

  void PrintFixedCapStrToScreen(FixedCapStr<16> text, uint8_t x, uint8_t y,
                                bool color = true) {
    field_.display.SetCursor(x, y);
    field_.display.WriteString(text, Font_6x8, color);
  }

  /**
   * LEDs
   */

  void ToggleKeyLed(uint8_t i) {
    keyLedsStates_[i] = !keyLedsStates_[i];
    keysLedsChanged_ = true;
  }

  void BlinkKeyLed(uint8_t i) {
    ToggleKeyLed(i);
    keyLedsBlinking[i] = true;
    blinking_ = true;
  }

  /**
   * Processes and applies current status of LEDs
   *
   * @param stepTime current step time to calculate blinking time
   */
  void ProcessLeds(uint32_t stepTime) {
    if (stepTime >= blinkingTime_ and blinking_) {
      for (size_t i = 0; i < 16; i++) {
        if (keyLedsBlinking[i]) {
          ToggleKeyLed(i);
          keyLedsBlinking[i] = false;
        }
      }
      blinking_ = false;
    }
    // only apply changes if there were any
    if (keysLedsChanged_) {
      for (size_t i = 0; i < 16; i++) {
        field_.led_driver.SetLed(keyLeds_[i],
                                 static_cast<float>(keyLedsStates_[i]));
      }
      field_.led_driver.SwapBuffersAndTransmit();
      keysLedsChanged_ = false;
    }
  }

  /**
   * CONTROLS
   */

  void ProcessAllControls() {
    field_.ProcessAllControls();
    // process knobs, check for tolerance
    for (size_t i = 0; i < 8; i++) {
      float tempKnobValue = field_.knob[i].Process();
      if (tempKnobValue < 0.0f) {
        tempKnobValue = 0.0f;
      }
      if (fabsf(tempKnobValue - knobValues_[i]) > knobTolerance_) {
        knobValues_[i] = tempKnobValue;
        knobChanged_[i] = true;
        return;
      }
      knobChanged_[i] = false;
    }
  }

  // keys

  bool KeyboardRisingEdge(uint8_t i) {
    if (field_.KeyboardRisingEdge(i)) {
      currentTime_ = System::GetNow();
      if (currentTime_ - lastDebounceTime_ >= debounceDelay_) {
        lastDebounceTime_ = currentTime_;
        return true;
      }
    }
    return false;
  }

  char GetKeyGroup(uint8_t key) {
    if (key >= 8 and key <= 15) {
      return 'A';
    }
    return 'B';
  }

  // switches

  bool SwitchRisingEdge(uint8_t i) {
    if (i == 1) {
      return field_.GetSwitch(DaisyField::SW_1)->RisingEdge();
    } else {
      return field_.GetSwitch(DaisyField::SW_2)->RisingEdge();
    }
  }

  bool SwitchPressed(uint8_t i) {
    if (i == 1) {
      return field_.GetSwitch(DaisyField::SW_1)->Pressed();
    } else {
      return field_.GetSwitch(DaisyField::SW_2)->Pressed();
    }
  }

  // knobs

  float ScaleKnob(int i, float minOutput, float maxOutput, bool log = false) {

    float norm = (knobValues_[i] - minKnob_) / (maxKnob_ - minKnob_);
    norm = (norm < 0.0f) ? 0.0f : (norm > 1.0f ? 1.0f : norm);

    if (log) {
      // log scale
      float logMin = logf(minOutput);
      float logMax = logf(maxOutput);
      // the power shapes the curve, 1 = normal log scale
      return expf(logMin + powf(norm, 0.5) * (logMax - logMin));
    } else {
      // linear scale
      return norm * (maxOutput - minOutput) + minOutput;
    }
  }

  bool DidKnobChange(uint8_t i) { return knobChanged_[i]; }
  float GetKnobValue(uint8_t i) { return knobValues_[i]; }
  float GetKnobValueInHertz(uint8_t i) {
    // notes from 21 to 108 (A0 to C8)
    uint8_t note = static_cast<int>(ScaleKnob(i, 21, 108));
    // 440 * 2^((note - 69)/12)
    return 440.0f * pow(2.0, (static_cast<float>(note) - 69.0) / 12.0);
  }

  /**
   * MIDI
   */

  void InitMidi() { field_.midi.StartReceive(); }

  // calculate time between clock packets
  // delta = time between packet 0 and 24 (24ppqn)
  // bpm = 60000 / delta
  void ProcessMidiClock() {
    field_.midi.Listen();
    while (field_.midi.HasEvents()) {
      MidiEvent m = field_.midi.PopEvent();
      if (m.type == SystemRealTime) {
        // clock midi event
        if (m.srt_type == TimingClock) {
          // enable midi clock
          usingMidiClock = true;
          // current time to calculate delta
          lastMidiClockTime = System::GetNow();
          midiPacketCount++;
          // calculate delta after 24 packets (24ppqn)
          if (midiPacketCount >= 24) {
            uint32_t delta = lastMidiClockTime - prevMs;
            midiBpm = std::round(60000.0f / delta);
            prevMs = lastMidiClockTime;
            midiPacketCount = 0;
          }
        }
        if (m.srt_type == Start || m.srt_type == Continue) {
          midiPlaying = true;
        }
        if (m.srt_type == Stop) {
          midiPlaying = false;
        }
      }
    }
    if (usingMidiClock &&
        (System::GetNow() - lastMidiClockTime > midiTimeoutMs)) {
      usingMidiClock = false;
    }
  }

  bool UsingMidiClock() { return usingMidiClock; }
  uint16_t GetMidiClock() { return midiBpm; }
  bool MidiIsPlaying() { return midiPlaying; }

  // getter for passthrough
  daisy::DaisyField &Field() { return field_; }

private:
  DaisyField field_;

  /**
   * LEDs
   */

  size_t keyLeds_[16] = {
      DaisyField::LED_KEY_A1, DaisyField::LED_KEY_A2, DaisyField::LED_KEY_A3,
      DaisyField::LED_KEY_A4, DaisyField::LED_KEY_A5, DaisyField::LED_KEY_A6,
      DaisyField::LED_KEY_A7, DaisyField::LED_KEY_A8, DaisyField::LED_KEY_B1,
      DaisyField::LED_KEY_B2, DaisyField::LED_KEY_B3, DaisyField::LED_KEY_B4,
      DaisyField::LED_KEY_B5, DaisyField::LED_KEY_B6, DaisyField::LED_KEY_B7,
      DaisyField::LED_KEY_B8};

  bool keyLedsStates_[16];
  bool keyLedsBlinking[16];
  // this is so we apply changes only if there were any
  bool keysLedsChanged_ = false;
  // how long to blink LEDs for, in AudioCallback iterations
  uint8_t blinkingTime_ = 50;
  bool blinking_ = false;

  /**
   * CONTROLS
   */

  // keys
  uint32_t currentTime_;
  uint32_t lastDebounceTime_;
  uint8_t debounceDelay_ = 32;
  // knobs
  const float knobTolerance_ = 0.001f;
  const float minKnob_ = 0.000396f;
  const float maxKnob_ = 0.968734f;
  float knobValues_[8];
  bool knobChanged_[8];

  /**
   * MIDI
   */

  // for midi clock in
  bool usingMidiClock = false;
  uint16_t midiBpm = 0;
  bool midiPlaying = false;
  // when the last clock message was received
  uint32_t lastMidiClockTime = 0;
  // timeout to go back to internal clock
  uint32_t midiTimeoutMs = 500;
  // for calculating time between packets
  uint32_t prevMs = 0;
  // for packet cound (24ppqn)
  uint16_t midiPacketCount = 0;
};