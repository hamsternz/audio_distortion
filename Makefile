audio_distortion : audio_distortion.c image.h
	gcc -o audio_distortion audio_distortion.c image.c -Wall -pedantic -O4 -lasound -lm -g
