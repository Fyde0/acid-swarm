## Acid Swarm
Made for the Electrosmith Daisy Field.

7 saw waves with detune control, an acid filter based on the filter from [open303](https://github.com/maddanio/open303 "open303"), volume and filter envelopes.
## Build
```bash
git clone https://github.com/Fyde0/acid-swarm
cd acid-swarm
git clone --recurse-submodules https://github.com/electro-smith/libDaisy
cd libDaisy && make && cd ..
make
# flash with:
make program-dfu
```
## Use
The knob controls are visible on the display, here's a list:

Knobs:
1. Transpose (-24 to +24 semitones)
2. Envelope attack (0.001 to 5 seconds)
3. Envelope decay (0.001 to 5 seconds)
4. Filter frequency (20Hz to 20kHz)
5. Filter resonance (0 to 0.95)
6. Filter envelope attack (0.001 to 5 seconds)
7. Filter envelope decay (0.001 to 5 seconds)
8. Filter envelope scale (0% to 100%)

Switch 1 + knobs:
1. Swarm detune (0 to -12 and 12 cents for the right most and left most oscillators)
2. Envelope curve (1 (linear) to 4)
3. Filter envelope curve (1 (linear) to 4)
4. Pitch slide time (0 to 2 seconds)

The filter envelope's attack and decay can be controlled from MIDI CC 14 and 15.
## Development
If you use VSCode you can install the clangd extension and run
```bash
make clean; bear -- make
```
to generate the `compile_commands.json` file.

Add `--query-driver=/path/to/gcc-arm-none-eabi-10-2020-q4-major/bin/arm-none-eabi-g++` to the clangd arguments in the extensions's settings.
