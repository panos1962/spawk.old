AWKDIR = ../gawk
LIB = /usr/lib/libspawk.so
LIB_r = /usr/lib/libspawk_r.so
OBJ = spawk.o
BU_LIST = README Makefile tools/*.sh src.stable/*.c src/*.c \
	lib/* bin/* Test/* Sample/*

.SUFFIXES:

all: $(LIB) $(LIB_r)

$(LIB): $(OBJ)
	@sh tools/makelib.sh $(LIB) mysqlclient

$(LIB_r): $(OBJ)
	@sh tools/makelib.sh $(LIB_r) mysqlclient_r

$(OBJ): spawk.c
	@sh tools/checkawk.sh $(AWKDIR)
	@echo "Compiling \`spawk.c'..."
	@gcc -shared -g -c -O -I$(AWKDIR) -I/usr/include/mysql \
		-D_SPAWK_DEBUG -DHAVE_CONFIG_H spawk.c
	@strip --strip-unneeded $(OBJ)

test:
	@make
	@-gawk -f Test/test99.awk 2>error
	@[ -s error ] && cat error >&2; rm -f error

cleanup:
	@rm -f spawk.[co] spawk.lst spawk.txt \
		BACKUP SPAWK TEST

fresh:
	@make cleanup
	@make

BACKUP: SPAWK TEST
	@sh tools/backup.sh

SPAWK: $(BU_LIST)
	@make all
	@tar -czvf spawk.tar $(BU_LIST) >spawk.lst
	tar -cf SPAWK lib/install spawk.tar

spawk.c: src.stable/*.c src/*.c
	@sh tools/makesrc.sh >spawk.c
	@sh tools/makeprn.sh >spawk.txt

man:
	@groff -T ascii -man -rLL=6.5i -rLT=7.7i \
		lib/spawk.man | less -is

TEST: Test/*
	@(cd Test && tar -czvf ../TEST * >/dev/null)
