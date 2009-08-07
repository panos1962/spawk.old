AWK_INCLUDE = __awk_include__
SQL_INCLUDE = __sql_include__
LIB = libspawk.so
LIB_r = libspawk_r.so
OBJ = spawk.o
BU_LIST = README configure tools/*.sh src.stable/*.c src/*.c \
	lib/* bin/* Test/* Sample/*

.SUFFIXES:

all: $(LIB) $(LIB_r)

$(LIB): $(OBJ)
	@sh tools/makelib.sh $(LIB) __sql_library__

$(LIB_r): $(OBJ)
	@sh tools/makelib.sh $(LIB_r) __sql_r_library__

$(OBJ): spawk.c
	@sh tools/checkincl.sh $(AWK_INCLUDE) awk.h
	@sh tools/checkincl.sh $(SQL_INCLUDE) mysql.h
	@echo "Compiling \`spawk.c'..."
	@gcc -shared -g -c -O -I$(AWK_INCLUDE) -I$(SQL_INCLUDE) \
		-D_SPAWK_DEBUG -DHAVE_CONFIG_H spawk.c
	@strip --strip-unneeded $(OBJ)

install: $(LIB) $(LIB_r)
	@cp $(LIB) $(LIB_r) /usr/lib/
	@sh tools/makelib.sh $(LIB)
	@sh tools/makelib.sh $(LIB_r)

test:
	@make
	@-LD_LIBRARY_PATH=".:/usr/lib" gawk -f Test/test99.awk 2>error
	@[ -s error ] && cat error >&2; rm -f error

cleanup:
	@rm -f backup spawk.* $(LIB) $(LIB_r) \
		/usr/lib/$(LIB) /usr/lib/$(LIB_r)

fresh:
	@make cleanup
	@make

backup: spawk.tar
	@sh tools/backup.sh

commit: spawk.tar
	@sh tools/commit.sh

spawk.tar: $(BU_LIST)
	@make all
	@tar -czvf spawk.tar $(BU_LIST) >spawk.lst

spawk.c: src.stable/*.c src/*.c
	@sh tools/makesrc.sh >spawk.c
	@sh tools/makeprn.sh >spawk.txt

man:
	@groff -T ascii -man -rLL=6.5i -rLT=7.7i \
		lib/spawk.man | less -is
