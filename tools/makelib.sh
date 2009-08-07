#!/bin/bash

# If given with one command line argument then that argument
# is considered an exisiting library filename and selinux
# care must be taken.
#
# Give two command line arguments, then the first argument is
# considered a library name to be linked.
#
# Given no arguments, then the two libraries (simple and
# threads-safe) will be constructed.

selinux() {
	if [ $# -ne 1 ]; then
		echo "${0}@selinux: internal error" >&2
		exit 1
	fi

	selinuxenabled 2>/dev/null && chcon -t textrel_shlib_t ${1}
	return 0
}

makelib() {
	if [ $# -ne 2 ]; then
		echo "${0}: internal error" >&2
		exit 1
	fi

	if [ ! -f "${2}" ]; then
		echo "${0}: {2} not found" >&2
		return 1
	fi

	echo "Linking \`${1}'
	with \`${2}' library..."
	ld --strip-all -o ${1} -shared spawk.o ${2} || {
		echo "${0}: ${2}: link failed" >&2
		return 1
	}

	selinux ${1}
	return 0
}

case $# in
1)
	selinux "${1}"
	exit $?
	;;
2)
	makelib "${1}" "${2}"
	exit $?
	;;
0)
	;;
*)
	echo "usage: ${0} [awk_library MySQL_library]" >&2
	exit 1
esac

errs=2
for i in "" "_r"
do
	mysql=$(find / -name libmysqlclient${i}.so \
-print -quit 2>/dev/null)
	if [ -z "${mysql}" ]; then
		echo "${0}: libmysqlclient${i}.so: \
MySQL library not found.
${0}: libspawk${i}.so: awk extension library not created!" >&2
	else
		makelib /usr/lib/libspawk${i}.so ${mysql} && errs=0
	fi
done
exit ${errs}
