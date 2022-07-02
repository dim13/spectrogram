# $Id$

PROG	= spectrogram
LIBS	= fftw3 x11 alsa
CFLAGS	= $(shell pkg-config --cflags ${LIBS})
LDLIBS	= $(shell pkg-config --libs ${LIBS}) -lm

${PROG}: spectrogram.o fft.o cms.o aux.o widget.o alsa.o

clean:
	rm -f *.o ${PROG}
