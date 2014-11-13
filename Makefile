CC=gcc
OBJS=main.o log.o sock.o net.o util.o netutil.o table.o tap.o nvgre.o cmd.o config.o
SRCS=${OBJS:%.o=%.c}
DEPS=$(OBJS:%.o=%.d)
LDLIBS=-lpthread
DAEMON=nvgred
#DEBUG_FLAG=-g -DDEBUG
CFLAGS=-Wall -O2 -MMD
CONTROLER=nvconfig
CONTROLER_OBJS=nvconfig.o log.o sock.o util.o netutil.o
LDFLAGS=

OSTYPE=OS_$(shell uname -s | tr 'a-z' 'A-Z')
CFLAGS+=-D${OSTYPE}

ifeq (${OSTYPE}, OS_LINUX)
OS_DIST=$(shell head -1 /etc/issue|cut -d ' ' -f1)
endif

PREFIX=/usr/local/bin
SCRIPT_DIR=script
LSB_SCRIPT=nvgred
INIT_DIR=/etc/init.d
CONFIG_SRC=./conf/nvgre.conf
CONFIG_DST=/etc/nvgre.conf

.SUFFIXES: .c .o

.c.o:
	${CC} ${CFLAGS} ${DEBUG_FLAG} -c $< ${LDFLAGS}

.PHONY: all debug clean test install uninstall

all:${DAEMON} ${CONTROLER}
-include $(DEPS)

${DAEMON}:${OBJS}
	${CC} ${CFLAGS} ${DEBUG_FLAG} -o $@ $^ ${LDLIBS} ${LDFLAGS}

${CONTROLER}:${CONTROLER_OBJS}
	${CC} ${CFLAGS} ${DEBUG_FLAG} -o $@ $^ ${LDFLAGS}

debug:
	${MAKE} DEBUG_FLAG="-g -DDEBUG" OBJS="${OBJS}"

netdebug:
	${MAKE} DEBUG_FLAG="-g -DDEBUG" OBJS="${OBJS}"
#	@cd test && ${MAKE}

clean:
	@rm -f *.o ${DEPS} ${DAEMON} ${CONTROLER}
	@cd test && ${MAKE} -s clean

.PHONY: install-bin install-conf install-script

install-bin:all
	install -p -m 755 ${DAEMON} ${PREFIX}/${DAEMON}
	install -p -m 755 ${CONTROLER} ${PREFIX}/${CONTROLER}

install-script:
ifeq (${OS_DIST}, CentOS)
	install -p -m 755 ${SCRIPT_DIR}/${LSB_SCRIPT}.centos ${INIT_DIR}/${LSB_SCRIPT}
else
ifeq (${OS_DIST}, Debian)
	install -p -m 755 ${SCRIPT_DIR}/${LSB_SCRIPT}.debian ${INIT_DIR}/${LSB_SCRIPT}
else
ifeq (${OS_DIST}, Ubuntu)
	install -p -m 755 ${SCRIPT_DIR}/${LSB_SCRIPT}.debian ${INIT_DIR}/${LSB_SCRIPT}
else
	install -p -m 755 ${SCRIPT_DIR}/${LSB_SCRIPT} ${INIT_DIR}/${LSB_SCRIPT}
endif
endif
endif

install-conf:
	install -p -m 644 ${CONFIG_SRC} ${CONFIG_DST}

install: install-bin install-script install-conf

uninstall:
	@rm -f ${PREFIX}/${DAEMON}
	@rm -f ${PREFIX}/${CONTROLER}
	@rm -f ${INIT_DIR}/${LSB_SCRIPT}
