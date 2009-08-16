#!/bin/bash

BU_EMAIL="panos1962@gmail.com"

echo "Mailing tar backup to ${BU_EMAIL}..."
{
	echo "SPAWK Backup"
	date
} | mutt -s "SPAWK Backup" -a spawk"${1}".tar \
	-a libspawk.so -a libspawk_r.so \
	-a spawk.o -a tools/makelib.sh -a spawk.txt \
	-a Test/spawk.dd -a Test/test00.awk -a Test/test11.awk \
	-a Test/test12.awk "${BU_EMAIL}"
cp spawk.lst backup
