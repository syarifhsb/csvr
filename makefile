CC       = gcc --std=gnu99
SRC      = csvr.c parser.c
OBJ      = ${SRC:.c=.o}
OBJDEBUG = ${SRC:.c=debug.o}
LIBS     = -lncurses
CFLAGS   = -Wall 
CDFLAGS  = -Wall -g 
LDFLAGS = ${LIBS}

all: csvr

debug: csvrDeb

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
