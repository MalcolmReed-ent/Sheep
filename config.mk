# sheep version
VERSION = 0.1

# Customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

# includes and libs
INCS = -I/usr/include
LIBS = -lncurses -lmpdclient

# flags
CPPFLAGS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_POSIX_C_SOURCE=200809L -DVERSION=\"${VERSION}\"
CFLAGS   = -std=c99 -pedantic -Wall -Wextra -Werror -Wno-deprecated-declarations -Os -fstack-protector-strong -D_FORTIFY_SOURCE=2 ${INCS} ${CPPFLAGS}
LDFLAGS  = -Wl,-z,relro,-z,now ${LIBS}

# compiler and linker
CC = cc

# Debug build settings
DEBUG_CFLAGS = -g -O0 -DDEBUG
DEBUG_LDFLAGS =

# Release build settings
RELEASE_CFLAGS = -O2
RELEASE_LDFLAGS = -s

# Default build
all: release

# Debug build target
debug: CFLAGS += ${DEBUG_CFLAGS}
debug: LDFLAGS += ${DEBUG_LDFLAGS}
debug: sheep

# Release build target
release: CFLAGS += ${RELEASE_CFLAGS}
release: LDFLAGS += ${RELEASE_LDFLAGS}
release: sheep
