# The following awk code can serve as awk utilities function library.
# You can include the file in your awk command line, e.g.
#
#	awk -f /usr/lib/spawk/spawk.awk -f panos1 data1 data2
#

BEGIN {
	extension("libspawk.so", "dlload")

	# Here follows an array that contains the error codes used
	# in the standard SPAWK utilities functions library. The
	# reason is just to make code readable and avoid MySQL
	# error codes.
	
	delete SPAWK_ERROR
	SPAWK_ERROR["FATAL_ERROR_EXIT_CODE"] = 2
	SPAWK_ERROR["UNKNOWN_DATABASE"] = 1049
}

# Function `spawk_database' sets the "current database", that is
# the database to connect for the future server connections.
# If not passed a database name, then the current database name will
# be returned.
#
# SYNOPSIS
# --------
# spawk_database([database])
#
# ARGUMENTS
# ---------
# database	It's the name of the database (string) to set as
#		current. If not passed, then the current database
#		is not changed.
#
# RETURN VALUE
# ------------
# The current database name (string) is returned.

function spawk_database(database,		data) {
	if (database) {
		if (spawk_update("USE " database))
			spawk_fatal(database ": cannot use database", \
				(spawk_errno() != \
				SPAWK_ERROR["UNKNOWN_DATABASE"]))

		SPAWKINFO["database"] = database
		return(database)
	}

	spawk_select("SELECT DATABASE()")
	spawk_first(data)
	return(data[1])
}

# Function `spawk_exec_select' forms and executes a select query. The
# projection list (select columns), the tables' list (optional), the
# WHERE clause (optional) and the ORDER BY clause (optional) of the
# query are passed as (string) arguments. After calling the function
# you must fetch the results via `spawk_data', `spawk_first' or
# `spawk_last' functions; you can also clear the unfetched results
# via `spawk_clear' function.
#
# If anything goes wrong and the query cannot be executed, then the
# program will be aborted.
#
# SYNOPSIS
# --------
# spawk_select(prjlist [, tlist [, where_clause [, order_by_clause]]])
#
# ARGUMENTS
# ---------
# prjlist		The projection list, that is the columns' (or
#			other expressions') list to be placed just after
#			the SELECT keyword in the query to be executed.
#			Any "white" characters embedded in that string
#			will be eliminated, so you can put spaces after
#			commas to let that string to be more readable.
#			The fields will be used later as indices to the
#			returned data array. If there are any "`"
#			(backquote) characters in the string, then those
#			characters will be not used in the indices.
#
# tlist			Optional. It's the table (or tables' list) from
#			where to get the columns in the projection list.
#			That is a comma separated list to be placed just
#			after the FROM keyword in the query to be
#			executed.
#
# where_clause		Optional. It's the WHERE clause of the query to
#			to be executed. It's a string that will be
#			placed just after the WHERE keyword.
#
# order_by_clause	Optional. It's the ORDER BY clause of the query
#			to be executed. That string will be placed just
#			after the ORDER BY keyword.
#
# RETURN VALUE
# ------------
# Function `spawk_exec_select' always returns zero.

function spawk_exec_select(prjlist, tlist, where_clause, \
	order_by_clause) {
	gsub("[ \t]?", "", prjlist)
	spawk_query("SELECT " prjlist)
	if (tlist)
		spawk_query(" FROM " tlist)

	if (where_clause)
		spawk_query(" WHERE " where_clause)

	if (order_by_clause)
		spawk_query(" ORDER BY " order_by_clause)

	if (spawk_update())
		spawk_fatal("", 1)

	gsub("`", "", prjlist)
	return(spawk_results(prjlist))
}

# Function `spawk_exec_insert' will execute an INSERT query. The
# table name, the columns' list and the columns' values are passed
# as (string) arguments. The function returns zero on success, or
# the MySQL error code if an error occurs.
#
# You can pass a fourth (non zero) argument to be used as fatal error
# indicator. In that case, if anything goes wrong the program exits
# after printing that parameter as an erro message.
#
# SYNOPSIS
# --------
# spawk_exec_insert(table, clist, vlist [, fatal])
#
# ARGUMENTS
# ---------
# table		The table name (string) for the inserted row.
#
# clist		A comma separated list of columns' names of the columns
#		that construct the row to be inserted.
#
# vlist		A comma separated list of the values of the columns'
#		to be inserted. That string must have the same form
#		as the string to give on mysql insert queries.
#
# RETURN VALUE
# ------------
# The `spawk_exec_insert' function returns zero on success. If the
# insert query fails, the the MySQL error code will be returned.
#
# EXAMPLES
# --------
# ...
# if (spawk_exec_insert(person, "pe_key, pe_lname, pe_fname", \
#	"3307, 'Papadopoulos', 'Panos'")) {
#	spawk_errmsg("cannot insert person", 1)
#	next
# }
# ...
#
# ...
# if (spawk_exec_insert(person, "pe_key, pe_lname, pe_fname", \
#	pekey ",'" pelname "','" pefname "'") {
#	spawk_errmsg(pekey ": cannot insert person", 1)
#	next
# }
# ...
#
# ...
# spawk_exec_insert(param, "pa_person, pa_kind, pa_date, pa_value", \
#	pekey "," pakind "," today() "," paval, \
#	pekey ": cannot insert parameter of type " pakind)
# ...
# In the above example, if insert fails, then the program will exit
# after printing a message like:
#
#	3307: cannot insert parameter of type 75

function spawk_exec_insert(table, clist, vlist, msg,		err) {
	if (!(err = spawk_update("INSERT INTO " table \
		" (" clist ") VALUES (" vlist ")")))
		return(0)

	if (msg)
		spawk_fatal(msg, 1)

	return(err)
}

function spawk_exec_update(table, set_list, where_clause) {
	spawk_query("UPDATE " table " SET " set_list)
	if (where_clause)
		spawk_query(" WHERE " where_clause)

	if (spawk_update())
		spawk_fatal("", 1)

	return(0)
}

function spawk_exec_delete(table, where_clause) {
	spawk_query("DELETE FROM " table)
	if (where_clause)
		spawk_query(" WHERE " where_clause)

	if (spawk_update())
		spawk_fatal("", 1)

	return(0)
}

function spawk_abort(err, msg) {
	if (!err)
		return(0)

	if (msg)
		msg = msg " "

	spawk_fatal(msg "(program aborted)")
}

function spawk_errmsg(msg, sqlerr,				nl) {
	printf SPAWKINFO["program"] ":" >"/dev/stderr"
	if (msg) {
		printf " " msg >"/dev/stderr"
		nl = 1
	}

	if (sqlerr && (sqlerr = spawk_errno())) {
		printf " (" sqlerr " -- " spawk_error() \
			")" >"/dev/stderr"
		nl = 1
	}

	if (nl) {
		print "" >"/dev/stderr"
		fflush("/dev/stderr")
	}

	return(1)
}

function spawk_fatal(msg, sqlerr) {
	if (!msg)
		msg = "fatal error"

	spawk_errmsg(msg, sqlerr)
	exit(SPAWK_ERROR["FATAL_ERROR_EXIT_CODE"])
}
