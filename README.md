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
The knob controls are listed in order on the display, the detune control is SW1 + Knob 1.

The filter envelope's attack and decay can be controlled from MIDI CC 14 and 15.
## Development
If you use VSCode you can install the clangd extension and run
```bash
make clean; bear -- make
```
to generate the `compile_commands.json` file.

Add `--query-driver=/path/to/gcc-arm-none-eabi-10-2020-q4-major/bin/arm-none-eabi-g++` to the clangd arguments in the extensions's settings.
