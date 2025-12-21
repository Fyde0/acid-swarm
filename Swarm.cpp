#include "Envelope.hpp"
#include "FieldWrap.hpp"
#include "Filter.hpp"
#include "Oscillator.hpp"
#include "daisy_field.h"
#include <string>

using namespace daisy;

// adding delay to the main while the block size is small (1 or 2)
// makes the controls sluggish
#define MAIN_DELAY 0 // ms, main loop iteration time (separate from audio)
#define DISPLAY_UPDATE_DELAY 100 // update display every x main iterations

FieldWrap hw;
CpuLoadMeter cpuLoad;

Oscillator osc;
Filter filter1;
Filter filter2;
Envelope env1;
Envelope env2;

// audio
float out1, out2;
// set by knob in main, used by midi note on
int transpose = 0;
//
bool switch1 = false;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {

  cpuLoad.OnBlockStart();

  for (size_t i = 0; i < size; i++) {

    hw.ListenMidi();
    while (hw.MidiHasEvents()) {
      MidiEvent m = hw.PopMidiEvent();

      switch (m.type) {

      case NoteOn: {
        uint8_t note = m.data[0];
        uint8_t velocity = m.data[1];
        if (velocity > 0) {
          env1.Trigger();
          env2.Trigger();
          note = note + transpose;
          osc.SetNote(note);
        }
        break;
      }
      case ControlChange: {
        uint8_t cc = m.data[0];    // CC number
        uint8_t value = m.data[1]; // CC value
        // CC 14 for filter envelope attack
        if (cc == 14) {
          float addAttack = (value / 127.0f) * 5.0f;
          env2.AddAttack((addAttack < 0.0f)
                             ? 0.0f
                             : (addAttack > 5.0f ? 5.0f : addAttack));
        }
        // CC 15 for filter envelope decay
        if (cc == 15) {
          float addDecay = (value / 127.0f) * 5.0f;
          env2.AddDecay(
              (addDecay < 0.0f) ? 0.0f : (addDecay > 5.0f ? 5.0f : addDecay));
        }
        break;
      }
      default:
        break;
      }
    }

    float env1Out = env1.Process();
    float env2Out = env2.Process();
    osc.SetAmp(env1Out);
    osc.Process(&out1, &out2);
    filter1.AddFreq(env2Out);
    filter2.AddFreq(env2Out);
    out1 = filter1.Process(out1 * 0.50f);
    out2 = filter2.Process(out2 * 0.50f);

    out[0][i] = out1;
    out[1][i] = out2;
  }

  cpuLoad.OnBlockEnd();
}

int main(void) {

  float samplerate;
  uint8_t blocksize;

  hw.Init(AudioCallback);
  hw.InitMidi();
  samplerate = hw.Field().AudioSampleRate();
  blocksize = hw.Field().AudioBlockSize();
  osc.Init(samplerate);
  filter1.Init(samplerate);
  filter2.Init(samplerate);
  env1.Init(samplerate);
  env2.Init(samplerate);
  cpuLoad.Init(samplerate, blocksize);

  // main loop iterations
  uint8_t mainCount = 0;
  //
  std::string uiLabels1[8] = {"Trns", "EnvA", "EnvD", "FltF",
                              "FltQ", "FEnA", "FEnD", "FEnS"};
  std::string uiLabels2[8] = {"Dtun", "", "", "", "", "", "", ""};
  std::string uiValues[8] = {"", "", "", "", "", "", "", ""};

  // y position of text rows on screen
  uint8_t row1 = 0;
  uint8_t row2 = 11;
  uint8_t row3 = 19;
  uint8_t row4 = 30;
  uint8_t row5 = 38;
  // uint8_t row6 = 48;
  uint8_t row7 = 56;
  // offset so the columns are centered
  uint8_t screenOffset = 6;

  while (1) {

    ++mainCount;

    hw.ProcessAllControls();

    switch1 = hw.SwitchPressed(1);

    for (size_t i = 0; i < 8; i++) {
      if (hw.DidKnobChange(i)) {
        if (!switch1) {
          switch (i) {
          case 0:
            // knob 1, transpose
            transpose = static_cast<int>(hw.ScaleKnob(i, -24.0f, 24.0f));
            break;
          case 1:
            // knob 2, env1 attack
            env1.SetAttack(hw.ScaleKnob(i, 0.001f, 5.1f, true));
            break;
          case 2:
            // knob 3, env1 decay
            env1.SetDecay(hw.ScaleKnob(i, 0.001f, 5.1f, true));
            break;
          case 3:
            // knob 4, filter frequency (index)
            filter1.SetFreq(hw.ScaleKnob(i, 0.0f, 1.0f));
            filter2.SetFreq(hw.ScaleKnob(i, 0.0f, 1.0f));
            break;
          case 4:
            // knob 5, filter q
            filter1.SetQ(hw.ScaleKnob(i, 0.0f, 1.0f));
            filter2.SetQ(hw.ScaleKnob(i, 0.0f, 1.0f));
            break;
          case 5:
            // knob 6, env2 attack
            env2.SetAttack(hw.ScaleKnob(i, 0.001f, 5.1f, true));
            break;
          case 6:
            // knob 7, env2 decay
            env2.SetDecay(hw.ScaleKnob(i, 0.001f, 5.1f, true));
            break;
          case 7:
            // knob 8, env2 scale
            env2.SetScale(hw.ScaleKnob(i, 0.0f, 1.0f));
            break;
          }
        }
        if (switch1) {
          switch (i) {
          case 0:
            // knob 1, detune
            osc.SetDetune(hw.ScaleKnob(i, 0.01f, 1.0f));
            break;
          }
        }
      }
    }

    // update display every x iterations
    if (mainCount % DISPLAY_UPDATE_DELAY == 0) {

      hw.ClearDisplay();

      std::string cpuAvgStr =
          "CPUAvg:" +
          std::to_string(static_cast<int>(cpuLoad.GetAvgCpuLoad() * 100)) +
          "% ";
      std::string cpuMaxStr =
          "CPUMax:" +
          std::to_string(static_cast<int>(cpuLoad.GetMaxCpuLoad() * 100)) +
          "% ";
      hw.PrintToScreen(cpuAvgStr.c_str(), 0, row1);
      hw.PrintToScreen(cpuMaxStr.c_str(), 68, row1);

      // floats cause problems so I multiply and cast to int
      if (!switch1) {
        uiValues[0] = std::to_string(transpose);
        uiValues[1] = std::to_string(static_cast<int>(env1.GetAttack() * 100));
        uiValues[2] = std::to_string(static_cast<int>(env1.GetDecay() * 100));
        float filtFreq = filter1.GetFreq();
        if (filtFreq < 10000.f) {
          uiValues[3] = std::to_string(static_cast<int>(filtFreq));
        } else {
          uiValues[3] = std::to_string(static_cast<int>(filtFreq / 1000));
          uiValues[3].append("k");
        }
        uiValues[4] = std::to_string(static_cast<int>(filter1.GetQ() * 100));

        uiValues[5] = std::to_string(static_cast<int>(env2.GetAttack() * 100));
        uiValues[6] = std::to_string(static_cast<int>(env2.GetDecay() * 100));
        uiValues[7] = std::to_string(static_cast<int>(env2.GetScale() * 100));
      } else {
        uiValues[0] = std::to_string(static_cast<int>(osc.GetDetune() * 100));
        uiValues[1] = "";
        uiValues[2] = "";
        uiValues[3] = "";
        uiValues[4] = "";
        uiValues[5] = "";
        uiValues[6] = "";
        uiValues[7] = "";
      }

      for (int i = 0; i < 8; i++) {
        uint8_t xPos = i * 30 + screenOffset; // + offset to center
        uint8_t yPosLabel = row2;
        uint8_t yPosValue = row3;
        // second row
        if (i > 3) {
          xPos = xPos - (4 * 30);
          yPosLabel = row4;
          yPosValue = row5;
        }
        if (!switch1) {
          hw.PrintToScreen(uiLabels1[i].c_str(), xPos, yPosLabel);
        } else {
          hw.PrintToScreen(uiLabels2[i].c_str(), xPos, yPosLabel);
        }
        hw.PrintToScreen(uiValues[i].c_str(), xPos, yPosValue);
      }

      if (switch1) {
        char sw1[4] = "SW1";
        hw.PrintToScreen(sw1, screenOffset, row7);
      }

      hw.UpdateDisplay();
    }

    // slow down the main thread
    System::Delay(MAIN_DELAY);
  }
}