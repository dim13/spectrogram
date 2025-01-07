# Spectrogram

Visualisation hack for OpenBSD (sndio) and Linux (alsa) playback.

![spectrogram](spectrogram.png)

## OpenBSD

Install required packages:

```
pkg_add fftw3
```

Build:

```
make -f Makefile.bsd-wrapper
```

`sndiod` must be started in monitoring mode (see the [Multimedia: Recording a Monitor Mix of All Audio Playback](https://www.openbsd.org/faq/faq13.html#recordmon) section of the FAQ:)

```
rcctl set sndiod flags -s default -m play,mon -s mon
rcctl restart sndiod
```

## Linux

Build packages: `sudo apt install libfftw3-dev libasound2-dev libx11-dev`

Audo loopback device: `sudo modprobe snd-aloop`
