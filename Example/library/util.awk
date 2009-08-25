# This is an incomplete awk program file. There is no detail section,
# but rather a set of "library" utility functions are defined to be
# used in other awk programs.

# Function `witer' accepts a writer's primary key (`wr_key') and
# returns the writer's name (`wr_name'). If the passed writer's
# key is invalid or the writer is not found in the database, then
# an error message is printed and an empty string is returned.

function writer(wk,			data) {
	if (wk !~ /^[ \t][0-9][0-9]*[ \t]$/) {
		print SPAWKINFO["program"] ": " wk \
			": invalid `wr_key' value" >"/dev/stderr"
		return(data)
	}

	spawk_select("SELECT `wr_name` FROM `writer` " \
		"WHERE `wr_key` = " wk)
	if (!spawk_first(data))
		print SPAWKINFO["program"] ": " wk \
			": writer not found" >"/dev/stderr"

	return(data[1])
}

# Function `title' accepts a book's primary key (`bo_key') and
# returns the book's title (`bo_title'). If the passed book's
# key is invalid or the book is not found in the database, then
# an error message is printed and an empty string is returned.

function title(bk,			data) {
	if (bk !~ /^[ \t][0-9][0-9]*[ \t]$/) {
		print SPAWKINFO["program"] ": " bk \
			": invalid `bo_key' value" >"/dev/stderr"
		return(data)
	}

	spawk_select("SELECT `bo_title` FROM `book` " \
		"WHERE `bo_key` = " bk)
	if (!spawk_first(data))
		print SPAWKINFO["program"] ": " bk \
			": book not found" >"/dev/stderr"

	return(data[1])
}

# Function `author' accepts a book's primary key (`bo_key') and
# selects the book's related authors. If the passed book's key
# is invalid, then an error message is printed and a non zero
# value is returned and no results are available, else the
# select query is executed and zero is returned; you must
# fetch the results via repeated "spawk_data" calls, or
# by calling "spawk_first", "spawk_last" or "spawk_clear"
# functions.

function author(bk) {
	if (bk !~ /^[ \t][0-9][0-9]*[ \t]$/) {
		print SPAWKINFO["program"] ": " bk \
			": invalid `bo_key' value" >"/dev/stderr"
		return(1)
	}

	spawk_select("SELECT `au_writer` FROM `author` " \
		"WHERE `au_book` = " bk)
	return(0)
}

# Function `subject' accepts a book's primary key (`bo_key') and
# selects the book's related subjects in alphabetical order. If
# the passed book's key is invalid, then an error message is printed,
# a non zero value is returned and no results are available, else the
# select query is executed and zero is returned; you must
# fetch the results via repeated "spawk_data" calls, or
# by calling "spawk_first", "spawk_last" or "spawk_clear"
# functions.

function subject(bk) {
	if (bk !~ /^[ \t][0-9][0-9]*[ \t]$/) {
		print SPAWKINFO["program"] ": " bk \
			": invalid `bo_key' value" >"/dev/stderr"
		return(1)
	}

	spawk_select("SELECT `su_subject` FROM `subject` " \
		"WHERE `su_book` = " bk " ORDER BY su_subject")
	return(0)
}
