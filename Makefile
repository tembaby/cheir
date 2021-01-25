# $Id: Makefile,v 1.7 2002/10/11 19:21:49 te Exp $
#

OBJS = mem.o lexer.o main.o stack.o extract.o getline.o mk.o
TARGET = cheir
DEBUG =
UCFLAGS = -Wno-format -D_MAKEFILE
INSTALLDIR = $$HOME/bin
TARBALL = cheir.tar.gz
PROGDIR = cheir
LEX_FILES = lex.l
LEX_FLAGS = -olexer.c
CLEAN_EXTRA = lexer.c
HDRDEP = cheir.h
DISTRIB_EXTERNAL_FILES = mk.exe.inc

.include <unix.prog.c.mk>
