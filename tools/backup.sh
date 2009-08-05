#!/bin/bash

BU_EMAIL="panos1962@gmail.com"

echo "Mailing tar backup to ${BU_EMAIL}..."
{
	echo "SPAWK Backup"
	date
} | mutt -s "SPAWK Backup" -a SPAWK -a libspawk.so -a libspawk_r.so \
	-a spawk.o -a tools/makelib.sh -a spawk.txt -a TEST \
	-a Test/spawk.dd -a Test/test00.awk -a Test/test11.awk \
	-a Test/test12.awk "${BU_EMAIL}"
rm -f spawk.tar
cp spawk.lst BACKUP
