# The following program seeks and prints the delayed boroughed
# books, that is books that must have been returned at specified
# dates (default current date). The "br_date" column is the date
# of boroughing the book, "br_until" is the date when the book
# must be returned to the library and "br_return" is the date
# that the book is actually returned to the library.
#
# The program uses the following external variables, where external
# means variables to set in the command line:
#
#	date	Is the target date. If not set, then the
#		current date is assumed.
#
#	detail	If set to a non empty, non zero valued, then
#		the seeked "borough" records will be printed
#		in detail, that is reader's name and book's
#		tite will be printed. If not set, then raw
#		"borough" data will be printed (usually to
#		post format with other, most sophisticated
#		report processing programs).

BEGIN {
	extension("libspawk.so", "dlload")
	SPAWKINFO["program"] = "borough"
	SPAWKINFO["database"] = "library"

	# Now it's time to check/set the target date

	set_date()

	# Run the query to select and print the target
	# "borough" records and exit (no input will be
	# processed).

	borough()
	exit(0)
}

function borough(			data) {
	# Here follows the query to select the target "borough"
	# records. The only thing that can produce an error in
	# that query is a wrong "date" value. We use that simple
	# fact in order to run the query and in the same time
	# check the "date" value.

	if (spawk_update("SELECT `br_reader`, `br_copy`, " \
		"`br_date`, `br_until` FROM borough " \
		"WHERE br_return IS NULL AND br_until < " date)) {
		print date ": invalid date" >"/dev/stderr"
		exit(2)
	}

	# The query passed the "date" value check, so convert the
	# query from an "update" query to a "select" query and
	# read the results ("borough" records).

	spawk_results()
	while (spawk_data(data)) {
		# Based on the "detail" variable print the
		# selected "borough" data as raw data ("detail"
		# not set), or as detailed data ("detail" set).

		if (detail)
			print_borough(data)
		else
			print data[0]
	}
}

# The "print_borough" function accepts a "borough" returned data
# array and prints the reader's name, the book's title and the
# date that the book must have been returned to the library.

function print_borough(borough) {
	print "Reader: " reader(borough[1])
	print "Book..: " title(borough[2])
	print "Date..: " borough[3] " - " borough[4]
	print ""
}

# Function "reader" is a utility function that accepts a reader's
# primary key ("rd_key") and returns the reader's name ("rd_name").

function reader(rk,				data) {
	spawk_select("SELECT `rd_name` FROM `reader` " \
		"WHERE `rd_key` = " rk)
	spawk_first(data)
	return(data[1])
}

# Function "title" is a utility function that accepts a book copy's
# primary key ("co_key") and returns the related book's title
# ("bo_title").

function title(ck,				data) {
	spawk_select("SELECT `bo_title` FROM `book` " \
		"WHERE `bo_key` = (SELECT `co_book` FROM `copy` " \
		"WHERE `co_key` = " ck ")")
	spawk_first(data)
	return(data[1])
}

# Function "set_date" is a initialization function to check/set
# the target date. If the "date" (external) value has been set
# (in the command line), then that date will be used as the
# target date, else the current date is assumed.

function set_date() {
	if (!date) {
		spawk_select("SELECT DATE_FORMAT(CURDATE(), '%Y%m%d')")
		spawk_first(data)
		date = data[1]
	}
}
