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
	spawk_results(prjlist)
	return(0)
}

function spawk_exec_insert(table, clist, vlist) {
	if (spawk_update("INSERT INTO " table " (" clist \
		") VALUES (" vlist ")"))
		spawk_fatal("", 1)

	return(0)
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
