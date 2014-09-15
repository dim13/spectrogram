# $Id$
# Debian user, please use `pmake'

VERSION=3.0
PROG=	spectrogram

SRCS=	spectrogram.c fft.c cms.c aux.c widget.c Sgraph.c
LIBS=	fftw3 x11 xaw7
BINDIR=	/usr/local/bin

UNAME!=	uname
.ifdef ${UNAME} == Linux
SRCS+=	alsa.c
LIBS+=	alsa
.else
SRCS+=	sio.c
LDADD+=	-lsndio
.endif

PCCF!=	pkg-config --cflags ${LIBS}
CFLAGS+=${PCCF}

PCLA!=	pkg-config --libs ${LIBS}
LDADD+=	${PCLA}

DEBUG+=	-Wall
NOMAN=
DIR=	${PROG}-${VERSION}

package:
	@mkdir ${DIR}
	@cp Makefile ${SRCS} ${HEADERS} ${DIR}
	@tar zcf ${DIR}.tar.gz ${DIR}
	@rm -rf ${DIR}

.include <bsd.prog.mk>
