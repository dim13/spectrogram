# $Id$

LDLIBS	= -lfftw3 -lX11 -lasound -lm

spectrogram: spectrogram.o fft.o cms.o aux.o widget.o alsa.o

clean:
	rm -f spectrogram *.o
