# $Id$

PROG=	spectrogram
SRCS=	spectrogram.c fft.c hsv2rgb.c
HEADERS=fifo.h fft.h hsv2rgb.h
LIBS=	sdl fftw3
PCCF!=	pkg-config --cflags ${LIBS}
PCLA!=	pkg-config --libs ${LIBS}
CFLAGS+=${PCCF}
LDADD+=	${PCLA} -lsndio
DEBUG+=	-Wall -ggdb
NOMAN=

.include <bsd.prog.mk>
