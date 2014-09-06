# $Id$

VERSION=2.0
PROG=	spectrogram
SRCS=	spectrogram.c sio.c fft.c cms.c Spectrogram.c
BINDIR=	/usr/local/bin
HEADERS=sio.h fft.h cms.h Spectrogram.h SpectrogramP.h
LIBS=	fftw3 x11 xaw7
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
	@tar zcf ${DIR}.tar.gz ${DIR}
	@rm -rf ${DIR}

.include <bsd.prog.mk>
