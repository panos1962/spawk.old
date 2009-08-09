#!/bin/bash

# This shell script takes two arguments: The first argument is the
# target library to construct whereas the second argument is the
# full pathname of the MySQL client shared library
# (libmysqlclient[_r].so).
#
# After linking the target library, selinux is checked and if it's
# turned on, then the target library is enabled via `chcon'.
#
# We can pass one argument if we only want selinux process for
# an existing library (install process).

if [ \( $# -lt 1 \) -o \( $# -gt 2 \) ]; then
	echo "usage: ${0} awk_extension [MySQL_library]" >&2
	exit 1
fi

if [ $# -eq 2 ]; then
	errs=
	for i in "${2}" spawk.o
	do
		if [ ! -f "${2}" ]; then
			echo "${0}: {2} not found" >&2
			errs=yes
		fi
	done
	[ -n "${errs}" ] && exit 2

	echo "Linking \`${1}'
		with \`${2}' library..."
	ld --strip-all -o "${1}" -shared spawk.o "${2}" || {
		echo "${0}: ${1}: link failed" >&2
		exit 1
	}
fi

if selinuxenabled 2>/dev/null; then
	chcon -t textrel_shlib_t "${1}" || exit 2
fi
exit 0
