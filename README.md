# Daisy polysynth Demo


Combined a few examples from the ElectroSmith [DaisyExamples](https://github.com/electro-smith/DaisyExamples)
repo to make a polyphonic synth with a nice Costello reverb, just for tinkering.

This repo is an accompaniment for [this blog series](http://www.jackcampbellsounds.com/2022/04/30/anatomyofabaremetalsynth_part1.html) on building a simple bare metal synth using Daisy.

---
**NOTE**::
Builds will likely only work out of the box with Linux or Mac. Daisy documents that these scripts should build with
MINGW64 but I haven't tried that myself yet.
---


# Cloning

This repo includes two submodules, DaisySP and libDaisy. To get everything needed, make sure to clone using `--recurse-submodules` like so:

```
git clone --recurse-submodules git@github.com:jcampbellcodes/daisy_polysynth.git
```

# Building

To build, you need the libdaisy and DaisySP repos cloned from above. Then run the following to build those
as well as our synth code (called `pluck`):

```
make all
```

You can build just the synth with `make` or `make pluck` and just rebuild the Daisy libs with `make daisy_libs`.

`make clean` will clean out the pluck target and the Daisy targets. (Just deletes the build folders.)

So far I've just tried this stuff out on macOS Big Sur (Intel). Make sure you have
set up your [Daisy dependencies](https://github.com/electro-smith/DaisyWiki/wiki/1b.-Installing-the-Toolchain-on-Mac#homebrew). (Cross compiler install, most important, the makefile will build the Daisy dependencies for you.)

From there, deploy to the board as you usually would:
```
# using USB (after entering bootloader mode)
make program-dfu
# using JTAG/SWD adaptor (like STLink)
make program
```
