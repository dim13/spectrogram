# Spectrogram

Visualisation hack for OpenBSD (sndio) and Linux (alsa) playback.

![spectrogram](spectrogram.png)

## OpenBSD

`sndiod` must be started in monitoring mode: `sndiod -m play,mon,midi`

## Linux

Packages required:

- pkg-config
- libfftw3-dev
- libasound2-dev
- libx11-dev
