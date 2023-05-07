# Makefile
# Build arecres
# By J. Stuart McMurray
# Created 20230505
# Last Modified 20230507

PROG=    arecres
SRCS!=   ls *.c
CFLAGS+= -Wall --pedantic -Wextra
NOMAN=   noman

.include <bsd.prog.mk>
