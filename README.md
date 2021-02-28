# Daisy polysynth Demo

Combined a few examples from the ElectroSmith [DaisyExamples](https://github.com/electro-smith/DaisyExamples)
repo to make a polyphonic synth with a nice Costello reverb, just for tinkering.

To build, you need the libDaisy and DaisySP repos.

Then you can pass them to make like so:
```
LIBDAISY_DIR=path/to/libdaisy DAISYSP_DIR=path/to/DaisySP make
```

From there, deploy to the board as you usually would:
```
# using USB (after entering bootloader mode)
make program-dfu
# using JTAG/SWD adaptor (like STLink)
make program
```
