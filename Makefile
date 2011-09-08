# $Id$

PROG=	spectrogram
SRCS=	spectrogram.c fifo.c fft.c hsv2rgb.c
HEADERS=fifo.h fft.h hsv2rgb.h
CFLAGS+=-I/usr/local/include \
	-I/usr/local/include/SDL \
	-D_GNU_SOURCE=1 \
	-D_REENTRANT \
	-I/usr/X11R6/include \
	-DXTHREADS
LDADD+=	-L/usr/local/lib \
	-lSDL \
	-pthread \
	-L/usr/X11R6/lib \
	-R/usr/X11R6/lib
LDADD+=	-lsndio -lfftw3 -lm
#LDADD+=	-liconv -lusbhid -lX11 -lxcb -lXau -lXdmcp
DEBUG+=	-Wall
#DEBUG+=-ggdb -g -pg
NOMAN=

.include <bsd.prog.mk>
