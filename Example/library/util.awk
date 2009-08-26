# This is NOT a complete awk program file. There is no detail section,
# nor any pattern matching sections, but rather a set of "library"
# utility functions are defined to be used in other awk programs.

# Function `witer' accepts a writer's primary key (`wr_key') and
# returns the writer's name (`wr_name'). If the passed writer's
# key is invalid or the writer is not found in the database, then
# an error message is printed and an empty string is returned.

function writer(wk,			data) {
	if (wk !~ /^[ \t]*[0-9][0-9]*[ \t]*$/) {
		print SPAWKINFO["program"] ": " wk \
			": invalid `wr_key' value" >"/dev/stderr"
		return(data[1])
	}

	spawk_select("SELECT `wr_name` FROM `library`.`writer` " \
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
	if (bk !~ /^[ \t]*[0-9][0-9]*[ \t]*$/) {
		print SPAWKINFO["program"] ": " bk \
			": invalid `bo_key' value" >"/dev/stderr"
		return(data[1])
	}

	spawk_select("SELECT `bo_title` FROM `library`.`book` " \
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
	if (bk !~ /^[ \t]*[0-9][0-9]*[ \t]*$/) {
		print SPAWKINFO["program"] ": " bk \
			": invalid `bo_key' value" >"/dev/stderr"
		return(1)
	}

	spawk_select("SELECT `au_writer` FROM `library`.`author` " \
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
	if (bk !~ /^[ \t]*[0-9][0-9]*[ \t]*$/) {
		print SPAWKINFO["program"] ": " bk \
			": invalid `bo_key' value" >"/dev/stderr"
		return(1)
	}

	spawk_select("SELECT `su_subject` FROM `library`.`subject` " \
		"WHERE `su_book` = " bk " ORDER BY su_subject")
	return(0)
}

# Function `get_copy_data' accepts a copy's primary key (`co_key') as
# first argument and an array as a second argument (optional). It
# returns the copy's data in the array (indices "book" and "location").
# If the passed copy's key is invalid or if the copy is not found,
# the function returns non zero, else zero is returned. We can
# pass a third non empty, non zero parameter to get an extra item
# in the returned array; that item will be indexed "title" and
# it's value will be the book's title.

function get_copy_data(ck, data, get_title) {
	delete data
	if (ck !~ /^[ \t]*[0-9][0-9]*[ \t]*$/) {
		print SPAWKINFO["program"] ": " ck \
			": invalid `co_key' value" >"/dev/stderr"
		return(1)
	}

	spawk_select("SELECT `co_book`, `co_location` " \
		"FROM `library`.`copy` WHERE `co_key` = " \
		ck, "book,location")
	if (!spawk_first(data)) {
		print SPAWKINFO["program"] ": " ck \
			": copy not found" >"/dev/stderr"
		return(1)
	}

	if (get_title)
		data["title"] = title(data["book"])

	return(0)
}

# Function `reader' accepts a reader's primary key (`rd_key') and
# returns the reader's name (`rd_name'). If the passed reader's
# key is invalid or the reader is not found in the database, then
# an error message is printed and an empty string is returned.

function reader(rk,			data) {
	if (rk !~ /^[ \t]*[0-9][0-9]*[ \t]*$/) {
		print SPAWKINFO["program"] ": " wk \
			": invalid `rd_key' value" >"/dev/stderr"
		return(data[1])
	}

	spawk_select("SELECT `rd_name` FROM `library`.`reader` " \
		"WHERE `rd_key` = " rk)
	if (!spawk_first(data))
		print SPAWKINFO["program"] ": " rk \
			": reader not found" >"/dev/stderr"

	return(data[1])
}
