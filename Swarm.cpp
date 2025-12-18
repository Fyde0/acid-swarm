#include "Envelope.hpp"
#include "FieldWrap.hpp"
#include "Filter.hpp"
#include "Oscillator.hpp"
#include "daisy_field.h"
#include "hid/midi_parser.h"
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

// audio
float out1, out2;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {

  cpuLoad.OnBlockStart();

  for (size_t i = 0; i < size; i++) {

    hw.ListenMidi();
    while (hw.MidiHasEvents()) {
      MidiEvent m = hw.PopMidiEvent();

      switch (m.type)
      case NoteOn: {
        uint8_t note = m.data[0];
        uint8_t velocity = m.data[1];
        if (velocity > 0) {
          env1.Trigger();
          // midi note to hertz
          osc.SetFreq(440.0f * pow(2.0f, (note - 69.0f) / 12.0f));
        }
        break;
      }
    }

    float env1Out = env1.Process();
    osc.SetAmp(env1Out);
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
  osc.SetMode(Oscillator::MODE_SAW);
  filter1.Init(samplerate);
  filter2.Init(samplerate);
  env1.Init(samplerate);
  cpuLoad.Init(samplerate, blocksize);

  // main loop iterations
  uint8_t mainCount = 0;
  // y position of text rows on screen
  uint8_t row1 = 0;
  //   uint8_t row2 = 11;
  //   uint8_t row3 = 19;
  //   uint8_t row4 = 30;
  //   uint8_t row5 = 38;
  //   uint8_t row6 = 48;
  //   uint8_t row7 = 56;

  while (1) {

    ++mainCount;

    // hw.ProcessAllControls();

    // update display every x iterations
    if (mainCount % DISPLAY_UPDATE_DELAY == 0) {

      hw.ClearDisplay();

      std::string cpuStr =
          "CPU:" +
          std::to_string(static_cast<int>(cpuLoad.GetAvgCpuLoad() * 100)) +
          "% ";
      hw.PrintToScreen(cpuStr.c_str(), 86, row1);

      std::string testStr = std::to_string(mainCount);
      hw.PrintToScreen(testStr.c_str(), 0, row1);

      hw.UpdateDisplay();
    }

    // slow down the main thread
    System::Delay(MAIN_DELAY);
  }
}