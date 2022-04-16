# Daisy polysynth Demo

Combined a few examples from the ElectroSmith [DaisyExamples](https://github.com/electro-smith/DaisyExamples)
repo to make a polyphonic synth with a nice Costello reverb, just for tinkering.

To build, you need the libdaisy and DaisySP repos.

Then you can pass them to make like so:
```
LIBDAISY_DIR=path/to/libdaisy DAISYSP_DIR=path/to/DaisySP make
```

Without defining those, the makefile assumes the DaisyExamples repo is a sibling to this one.
If this is the case, you should just be able to run `make` after building those.

So far I've just tried this stuff out on macOS Big Sur (Intel). Make sure you have
set up your [Daisy dependencies](https://github.com/electro-smith/DaisyWiki/wiki/1b.-Installing-the-Toolchain-on-Mac#homebrew). (Cross compiler install + built Daisy repos.)

From there, deploy to the board as you usually would:
```
# using USB (after entering bootloader mode)
make program-dfu
# using JTAG/SWD adaptor (like STLink)
make program
```

TODO: Setup libdaisy and DaisySP repos as submodules and adjust the makefile
so a `make` just does whatcha need.