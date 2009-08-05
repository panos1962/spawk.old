#!/bin/bash

if [ $# -ne 2 ]; then
	echo "${0}: internal error" >&2
	exit 1
fi

if [ ! -f "${2}" ]; then
	echo "${0}: {2} not found" >&2
	return 1
fi

echo "Linking \`${1}' with \`${2}' library..."
ld --strip-all -o ${1} -shared spawk.o ${2} || {
	echo "${0}: ${2}: link failed" >&2
	exit 1
}

selinuxenabled 2>/dev/null && chcon -t textrel_shlib_t ${1}
exit 0
