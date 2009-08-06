#!/bin/bash

BU_EMAIL="panos1962@gmail.com"

echo "Mailing tar backup to ${BU_EMAIL}..."
{
	echo "SPAWK Backup"
	date
} | mutt -s "SPAWK Commit" -a spawk.tar "${BU_EMAIL}"
