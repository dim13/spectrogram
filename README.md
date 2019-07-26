# Spectrogram

Visualisation hack for OpenBSD (sndio) and Linux (alsa) playback.

## Notes

- OpenBSD: `sndiod` must be started in monitoring mode:
  `sndiod -m play,mon,midi`
- Linux: build with `pmake`

![spectrogram](spectrogram.png)
