progname=`basename ${0}`
case "${progname}" in
makesrc.sh)
	cat="cat"
	post="cat"
	;;
makeprn.sh)
	cat="expand"
	post="sed 's;^;  ;'"
	;;
*)
	echo "${progname}: invalid calling name" >&2
	exit 2
esac

errs=
line="////////////////////////////////\
///////////////////////////////////////"

echo "${line}
//
// $(date)
//
// Panos I. Papadopoulos (C) 2009-
// 
// The sources of SPAWK concatenated in a single roll.
// Sources marked \`stable', e.g. \`src.stable/global.c',
// are more stable than others.
//
${line}" | eval ${post}

for file in `cat lib/source.list`
do
	if [ -f "src/${file}" ]; then
		file="src/${file}"
		if [ -f "src.stable/${file}" ]; then
			echo "$0: ${file}: multiple source" >&2
			errs=yes
		fi
	elif [ -f "src.stable/${file}" ]; then
		file="src.stable/${file}"
	else
		echo "$0: ${file}: file not found" >&2
		errs=yes
	fi

	echo "
${line}
//
// SOURCE: ${file}
//
${line}
"
	${cat} "${file}"
done | eval ${post}

if [ -n "${errs}" ]; then
	echo "$0: fininshed with problems" >&2
	exit 2
fi
