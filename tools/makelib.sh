#!/bin/bash

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

	selinuxenabled 2>/dev/null && chcon -t textrel_shlib_t ${1}
	return 0
}

if [ $# -eq 2 ]; then
	makelib "${1}" "${2}"
	exit $?
fi

if [ $# -ne 0 ]; then
	echo "usage: ${0} [awk_library MySQL_library]" >&2
	exit 1
fi

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
