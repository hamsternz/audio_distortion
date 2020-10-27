# audio_distortion
A basic utility to measure the THD+Noise for your default audio device with a 1kHz tone. By default it generates and cptures 12000 samples @ 48kS/s (0.25s)

Creates a PPM image of the captured spectrum.

![Example](https://github.com/hamsternz/audio_distortion/blob/main/example.png)

NOTE - only meausures the **LEFT** channel of the DEFAULT ALSA audio device.

> **WARNING**: IF YOU SEND THE OUTPUT OF THIS PROGRAM TO YOUR SPEAKERS OR HEADPHONES YOU
> MIGHT DAMAGE EITHER YOUR SPEAKERS AND/OR YOUR EARS

## Prerequisites

You will need to have the ALSA dev libraries installed.

## Using this program

1. Build with "make".

2. Attach a cable between line out to line in. Use a stereo cable.

3. Run ../audio_distortion

NOTE: Something is up with the capture volume - the driver reports different max an
d min values than it seems to use. This may also be a bug. 

5. Numbers will be displayed, and "graph.ppm" will be written.

Look in main() to change the test frequency

## Optimizing the result for best numbers

If you have very high THD numbers (> 1%) you are either overdriving the output or input.
Try reducing the capture volume, and if that doesn't help, also try reducing the master
volume. A reasonable sound card should easily get well below 0.1%

## Example output:

    $ /audio_distortion 95 9
    Audio playback device opened successfully.
    Audio device parameters have been set successfully.
    Init: Buffer size = 16384 frames.
    Init: Significant bits for linear samples = 16
    Audio device has been prepared for use.
    Audio playback device opened successfully.
    Audio capture device parameters have been set successfully.
    Init: Buffer size = 16384 frames.
    Init: Significant bits for linear samples = 16
    Audio capture device has been prepared for use.
    Setting playback volume to 62259 in range 65536 0
    Setting capture level to 5898 in range 65536 0
    Audio devices has been uninitialized.
    
    Analysing captured data...
    
    signal =   23365.16
    dc     =      -0.05
    thd+n  =       1.15  (  0.005%)
    s:n    =      86.13 dB
