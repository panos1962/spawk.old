AWK_INCLUDE = /root/gawk-3.1.7
SQL_INCLUDE = /usr/include/mysql
PREFIX = /usr/lib/
LIB = libspawk.so
LIB_r = libspawk_r.so
OBJ = spawk.o
BU_LIST = README INSTALL NEWS configure tools/*.sh src.stable/*.c src/*.c lib/* bin/* Test/* Sample/* Example/*
TARBALL = spawk-2.4.1.tar.gz

.SUFFIXES:

all: Makefile $(LIB) $(LIB_r)

Makefile: lib/version
	@echo "Run \`sh configure' to refresh \`Makefile'"
	@exit 1

$(LIB): $(OBJ)
	@sh tools/makelib.sh $(LIB) /usr/lib/mysql/libmysqlclient.so

$(LIB_r): $(OBJ)
	@sh tools/makelib.sh $(LIB_r) /usr/lib/mysql/libmysqlclient_r.so

$(OBJ): spawk.c
	@sh tools/checkincl.sh $(AWK_INCLUDE) awk.h
	@sh tools/checkincl.sh $(SQL_INCLUDE) mysql.h
	@echo "Compiling \`spawk.c'..."
	@gcc -shared -g -c -O -I$(AWK_INCLUDE) -I$(SQL_INCLUDE) -D_SPAWK_DEBUG -DHAVE_CONFIG_H -DSPAWK_VERSION="\"2.4.1 (Alpha)\"" spawk.c
	@strip --strip-unneeded $(OBJ)

install: $(PREFIX)$(LIB) $(PREFIX)$(LIB_r)

$(PREFIX)$(LIB): $(OBJ)
	@mv $(LIB) $(PREFIX)$(LIB)
	@sh tools/makelib.sh $(PREFIX)$(LIB)

$(PREFIX)$(LIB_r): $(OBJ)
	@mv $(LIB_r) $(PREFIX)$(LIB_r)
	@sh tools/makelib.sh $(PREFIX)$(LIB_r)

test:
	@make
	@-LD_LIBRARY_PATH="." awk -f Test/test99.awk 2>error
	@[ -s error ] && cat error >&2; rm -f error

cleanup:
	@rm -f backup spawk.* $(TARBALL) $(LIB) $(LIB_r)

cleanall:
	@make cleanup
	@rm -f $(PREFIX)$(LIB) $(PREFIX)$(LIB_r)

backup: $(TARBALL)
	@sh tools/backup.sh $(TARBALL)

tar: $(TARBALL)

$(TARBALL): $(BU_LIST)
	@make all
	@tar -cf spawk.tar $(BU_LIST) >/dev/null
	@rm -rf spawk-2.4.1; mkdir spawk-2.4.1
	@(cd spawk-2.4.1; tar xf ../spawk.tar)
	@tar -czvf $(TARBALL) spawk-2.4.1 >spawk.lst
	@rm -rf spawk.tar spawk-2.4.1

spawk.c: src.stable/*.c src/*.c
	@sh tools/makesrc.sh >spawk.c
	@sh tools/makeprn.sh >spawk.txt

man:
	@groff -T ascii -man -rLL=6.5i -rLT=7.7i lib/spawk.man | less -is
