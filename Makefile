# $Id$

PROG=	spectrogram
SRCS=	spectrogram.c sio.c fft.c hsv2rgb.c
HEADERS=fft.h hsv2rgb.h
LIBS=	fftw3 x11
PCCF!=	pkg-config --cflags ${LIBS}
PCLA!=	pkg-config --libs ${LIBS}
CFLAGS+=${PCCF}
LDADD+=	${PCLA} -lsndio
DEBUG+=	-Wall -ggdb
NOMAN=

.include <bsd.prog.mk>
