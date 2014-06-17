CC=gcc
OBJ=nvgre.o
TARGET=nvgre
TARGETS=${TARGET}
LDFLAGS=

.SUFFIXES: .c .o

.c.o:
	${CC} ${CFLAGS} ${LDFLAGS} ${DEBUG_FLAG} -c $<

.PHONY: all clean

all:${TARGETS}

${TARGET}:${OBJ}
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $^ ${LDLIBS}

clean:
	@rm -f *.o ${TARGETS}
