# $Id$

PROG=	spectrogram
SRCS=	spectrogram.c fft.c hsv2rgb.c
HEADERS=fft.h hsv2rgb.h
LIBS=	sdl SDL_gfx fftw3
PCCF!=	pkg-config --cflags ${LIBS}
PCLA!=	pkg-config --libs ${LIBS}
CFLAGS+=${PCCF}
LDADD+=	${PCLA} -lsndio
DEBUG+=	-Wall
NOMAN=

.include <bsd.prog.mk>
