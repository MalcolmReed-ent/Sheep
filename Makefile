# sheep - simple mpd client
# See LICENSE file for copyright and license details.

include config.mk

SRC = sheep.c
OBJ = ${SRC:.c=.o}

all: options sheep

options:
	@echo sheep build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk

sheep: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

debug: CFLAGS += ${DEBUG_CFLAGS}
debug: LDFLAGS += ${DEBUG_LDFLAGS}
debug: sheep

release: CFLAGS += ${RELEASE_CFLAGS}
release: LDFLAGS += ${RELEASE_LDFLAGS}
release: sheep

clean:
	rm -f sheep ${OBJ} sheep-${VERSION}.tar.gz

dist: clean
	mkdir -p sheep-${VERSION}
	cp -R LICENSE Makefile README.md config.mk config.h sheep.1 \
		sheep.c sheep-${VERSION}
	tar -cf sheep-${VERSION}.tar sheep-${VERSION}
	gzip sheep-${VERSION}.tar
	rm -rf sheep-${VERSION}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f sheep ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/sheep
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" < sheep.1 > ${DESTDIR}${MANPREFIX}/man1/sheep.1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/sheep.1

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/sheep
	rm -f ${DESTDIR}${MANPREFIX}/man1/sheep.1

.PHONY: all options clean dist install uninstall debug release
