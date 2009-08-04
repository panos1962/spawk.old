# The following program scans the `file` table in the `spawk` database
# and prints statistics. The scan can be narrowed by the means of the
# (global) variables `from' and `to'. These dates can be specified in
# the command line using -v from=date or -v to=date, e.g.
#
#	awk -v from=20090101 -v to=20090531 -f test12.awk
#
# will gather and print statistics for the interval 01/01/2009 through
# 05/31/2009.

BEGIN {
	CONVFMT = "%.0f"
	extension("libspawk.so", "dlload")
	SPAWKINFO["database"] = "spawk"
	SPAWKINFO["null"] = "NULL"
	spawk_update("USE `spawk`", 2)	# check `spawk' database
	scan_files()
	statistics()	# print statistics for the last user/date
	exit(0)
}

# Function `scan_files' scans the `file' table from `from' until
# `to' dates inclusive. If either one of the date `from' and `to'
# have not specified, then we act logically. While scaning `file'
# data we collect data and print statistics when either the date
# or the user changes. That's why the `file' rows are scanned
# in an ordered fashion.

function scan_files(				data, null, uid) {
	spawk_query("SELECT `user`, `date`, `type`, `size` " \
		"FROM `file` WHERE 1 = 1")

	if (from)
		spawk_query(" AND `date` >= " from)

	if (to)
		spawk_query(" AND `date` <= " to)

	spawk_select(" ORDER BY `user`, `date`")
	while (spawk_data(data, null)) {
		# If a new user arises, then print statistics for
		# the previous user. Local variable `uid' keeps track
		# of the "current" user whereas global variable
		# `date' keeps track of the "current" date.
		if (data[1] != uid) {
			statistics()
			user = login(uid = data[1])
			if (!total)
				print user

			date = ""
		}

		if (data[2] != date) {
			statistics()
			date = data[2]
			if (!total)
				print "\t" date
		}

		# Global variable `total' acts as a flag for
		# printing or not analytical data. If the variable
		# is set, then only the statistics are printed.
		if (!total)
			print "\t\t" data[3], data[4]

		
		# Global array `count' holds the statistics data.
		# The array is indexed on file types. Index "" is
		# used as a flag to denote that there are data to
		# print.
		count[""]
		count[data[3]]++

		# Check the fourth column for nullness. If not null,
		# then the `file' has some logical size. Add it to
		# the total size for the current user and date.
		if (!(4 in null))
			fsize += data[4]
	}
}

# Function `statistics' is straigt forward. It's called whenever
# a user or date changes. It's also called at the end for not to
# loose the last data.

function statistics(					i) {
	if (!("" in count))
		return

	# Remember `count[""]' is used as a flag to denote that
	# there exist data to print.
	delete count[""]
	if (!total)
		printf "\t"

	print "Statistics for `" user "' "  date
	for (i in count)
		print "\t" i "(s)", count[i]

	print "\tTotal file size: " fsize
	if (count["file"])
		print "\tBytes per file: " fsize / count["file"]

	delete count
	fsize = 0
}

# Function `login' is a tool function that accepts a user-id and
# returns the corresponding login name.
#
# Later we'll replace that function with a "library" function in
# `spawk.awk' awk script. This script doesn't contain a complete
# stand-alone program but rather contains a collection of awk
# functions to be used by other programs, e.g.
#
#	awk -f spawk.awk -f test12.awk

function login(uid,			data) {
	spawk_select("SELECT `login` FROM `user` " \
		"WHERE `uid` = " uid)
	spawk_first(data)
	return(data[1])
}
