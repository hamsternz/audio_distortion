# audio_distortion
Measure the THD+Noise for your default audio device with a 1kHz tone.

Creates a PPM image of the captured spectrum.

NOTE - only meausures the LEFT channel of the DEFAULT ALSA audio device.

WARNING: IF YOU SEND THE OUTPUT OF THIS PROGRAM TO YOUR SPEAKERS OR HEADPHONES YOU MIGHT DAMGAGE EITHER YOUR SPEAKERS OUR YOUR EARS

## Prerequeists

You will need to have the ALSA dev libraries installed.

## Using this program

1. Build with "make".

2. Attach a cable between line out to line in. Use a stereo cable.

3. Run ../audio_distortion <master volume %> <capture volume %>

4. On my system I get the best numbers from "./audio_destortion 90 9".

NOTE: Something is up with the capture volume - the driver reports different max and min values than it seems to use. This may also be a bug. 

5. Numbers will be displayed, and "graph.ppm" will be written.

## Optimizing the result

If you have very high THD numbers (> 1%) you are either overdriving the output or input. Try reducing the capture volume, and if that doesn't help, also try reducing the master volume. A reasonable sound card should easily get well below 0.1%
