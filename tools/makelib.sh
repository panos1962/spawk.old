#!/bin/bash
#
# USAGE(1)
# ========
# makelib.sh target library
# e.g.		makelib.sh /usr/lib/libspawk.so mysqlclient
# or		makelib.sh /usr/lib/libspawk_r.so mysqlclient_r
#
# USAGE(2)
# ========
# makelib.sh

mklib() {
	# It's more likely to find MySQL shared libraries in
	# `/usr/lib' or `/usr/lib/mysql' directories. So seek
	# the correct directory and link the SPAWK module.
	ok=
	for ldir in /usr/lib /usr/lib/mysql
	do
		if [ -f ${ldir}/lib${2}.so ]; then
			ok=yes
			break
		fi
	done

	if [ -z "${ok}" ]; then
		echo "${0}: lib${2}.so not found under /usr/lib: \
${1}: not created" >&2
		return 1
	fi

	echo "Linking \`${1}' with \`${2}' library..."
	ld --strip-all -o ${1} -shared spawk.o \
-L ${ldir} -l ${2} || {
		echo "${0}: lib${2}: failed to link"
		return 1
	}

	selinuxenabled 2>/dev/null && chcon -t textrel_shlib_t ${1}
	return 0
}


case $# in
0)
	mklib /usr/lib/libspawk.so mysqlclient
	mklib /usr/lib/libspawk_r.so mysqlclient_r
	;;
2)
	mklib $*
	;;
*)
	echo "usage: ${0}
       ${0} target library" >&2
	exit 1
esac
