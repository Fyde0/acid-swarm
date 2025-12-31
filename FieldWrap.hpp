#pragma once

#include <daisy_field.h>

using namespace daisy;

class FieldWrap {
public:
  FieldWrap() {}

  void Init(AudioHandle::AudioCallback cb) {
    field_.Init();
    field_.SetAudioBlockSize(16); // not too small
    field_.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_96KHZ);
    field_.StartAdc();
    field_.StartAudio(cb);
    // zero LEDs
    field_.led_driver.SwapBuffersAndTransmit();
    // set all knobs as changed so the values are all read at boot
    for (size_t i = 0; i < 8; i++) {
      knobChanged_[i] = true;
    }
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

  // switches

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
      // log scale (don't send 0 or lower to this, log doesn't like it)
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

  /**
   * MIDI
   */

  void InitMidi() { field_.midi.StartReceive(); }
  void ListenMidi() { field_.midi.Listen(); }
  bool MidiHasEvents() { return field_.midi.HasEvents(); }
  MidiEvent PopMidiEvent() { return field_.midi.PopEvent(); }

  // getter for passthrough
  daisy::DaisyField &Field() { return field_; }

private:
  DaisyField field_;

  // knobs
  const float knobTolerance_ = 0.001f;
  // the knobs are not actually from 0 to 1
  // getting the real values for each knob is annoying
  // so I picked safe values
  const float minKnob_ = 0.001f;
  const float maxKnob_ = 0.967f;
  float knobValues_[8];
  bool knobChanged_[8];
};