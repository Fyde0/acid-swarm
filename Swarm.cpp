#include "Envelope.hpp"
#include "FieldWrap.hpp"
#include "Filter.hpp"
#include "Oscillator.hpp"
#include "daisy_field.h"
#include <string>

using namespace daisy;

#define MAIN_DELAY 5 // ms, main loop iteration time (separate from audio)
#define DISPLAY_UPDATE_DELAY 10 // update display every x main iterations

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
          // midi note to hertz
          // osc.SetFreq(440.0f * pow(2.0f, (note - 69.0f) / 12.0f));
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
    filter1.AddFreq(env2Out);
    filter2.AddFreq(env2Out);
    osc.Process(&out1, &out2);
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
  cpuLoad.Reset();

  // main loop iterations
  uint8_t mainCount = 0;
  //
  std::string uiLabels[8] = {"Trns", "EnvA", "EnvD", "FltF",
                             "FltQ", "FEnA", "FEnD", "FEnS"};
  std::string uiValues[8] = {"", "", "", "", "", "", "", ""};

  // y position of text rows on screen
  uint8_t row1 = 0;
  uint8_t row2 = 11;
  uint8_t row3 = 19;
  uint8_t row4 = 30;
  uint8_t row5 = 38;
  // uint8_t row6 = 48;
  // uint8_t row7 = 56;
  // offset so the columns are centered
  uint8_t screenOffset = 6;

  while (1) {

    ++mainCount;

    hw.ProcessAllControls();

    for (size_t i = 0; i < 8; i++) {
      if (hw.DidKnobChange(i)) {
        switch (i) {
        case 0:
          // knob 1, transpose
          transpose = static_cast<int>(hw.ScaleKnob(i, -24.9f, 24.9f));
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
          // knob 4, filter frequency
          filter1.SetFreq(hw.ScaleKnob(i, 0.0f, 1.1f));
          filter2.SetFreq(hw.ScaleKnob(i, 0.0f, 1.1f));
          break;
        case 4:
          // knob 5, filter q
          filter1.SetQ(hw.ScaleKnob(i, 0.0f, 1.1f));
          filter2.SetQ(hw.ScaleKnob(i, 0.0f, 1.1f));
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
          env2.SetScale(hw.ScaleKnob(i, 0.0f, 1.1f));
          break;
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
        hw.PrintToScreen(uiLabels[i].c_str(), xPos, yPosLabel);
        hw.PrintToScreen(uiValues[i].c_str(), xPos, yPosValue);
      }

      hw.UpdateDisplay();
    }

    // slow down the main thread
    System::Delay(MAIN_DELAY);
  }
}