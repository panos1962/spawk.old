#!/bin/bash

[ $# -lt 2 ] && {
	echo "${0}: internal error" >&2
	exit 1
}

if [ ! -d "${1}" ]; then
	echo "${0}: ${1}: directory not found" >&2
	exit 2
fi

dir="${1}"
shift 1
errs=
for i in $*
do
	if [ ! -f "${dir}/${i}" ]; then
		echo "${0}: ${dir}/${i}: file not found" >&2
		errs=yes
	fi
done
[ -z "${errs}" ]
