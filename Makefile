# $Id$

VERSION=2.0
PROG=	spectrogram
SRCS=	spectrogram.c sio.c fft.c hsv2rgb.c
BINDIR=	/usr/local/bin
HEADERS=sio.h fft.h hsv2rgb.h
LIBS=	fftw3 x11
PCCF!=	pkg-config --cflags ${LIBS}
PCLA!=	pkg-config --libs ${LIBS}
CFLAGS+=${PCCF}
LDADD+=	${PCLA} -lsndio
DEBUG+=	-Wall
NOMAN=
DIR=	${PROG}-${VERSION}

package:
	@mkdir ${DIR}
	@cp Makefile ${SRCS} ${HEADERS} ${DIR}
	@tar zcf ${DIR}.tgz ${DIR}
	@rm -rf ${DIR}

.include <bsd.prog.mk>
