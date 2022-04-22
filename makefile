CC       = gcc --std=gnu99
SRC      = csvr.c parser.c utils.c string_st.c
OBJ      = ${SRC:.c=.o}
OBJDEBUG = ${SRC:.c=debug.o}
LIBS     = -lncurses
CFLAGS   = -Wall 
CDFLAGS  = -Wall -g 
LDFLAGS = ${LIBS}

STRINGSTDIR = ../string_st
STRINGSTSRC = ${STRINGSTDIR}/string_st.c ${STRINGSTDIR}/string_st.h

all: csvr

debug: csvrDeb

string: ${STRINGSTSRC}
	cp  ${STRINGSTSRC} .

%.o: %.c makefile
	${CC} -c ${CFLAGS} $<

%debug.o: %.c makefile
	${CC} -c ${CDFLAGS} $< -o $@ 

csvr: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

csvrDeb: ${OBJDEBUG}
	${CC} -o $@ ${OBJDEBUG} -g ${LDFLAGS}

clean:
	rm -f csvr csvrDeb ${OBJ} ${OBJDEBUG}
