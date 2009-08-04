#!/bin/bash
#
# USAGE(1)
# ========
# checkawk.sh directory
#

info="

Fix Info
--------
Please edit \`Makefile' and set \`AWKDIR' to the full
or realative pathname of gawk source directory."

[ $# -lt 1 ] && {
	echo "AWKDIR not specified${info}" >&2
	exit 2
}

[ $# -gt 1 ] && {
	echo "usage: ${0} directory" >&2
	exit 1
}

[ -d "${1}" ] || {
	echo "${1}: directory not found${info}" >&2
	exit 2
}

[ -f "${1}/awk.h" ] || {
	echo "${1}/awk.h: file not found${info}" >&2
	exit 2
}

cd "${1}"
