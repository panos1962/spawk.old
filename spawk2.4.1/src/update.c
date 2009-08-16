///////////////////////////////////////////////////////////////////////

// When writting in the query log file (debugging on) it's a good idea
// to indent log queries according to the server's depth. Thus, queries
// of server 0 have no indent, queries of server 1 have one-tab indent,
// queries of server 2 have tow-tabs indent etc. The following macro
// does the indenting.

#define debug_level() { \
	int i; \
 \
	for (i = 1; i < n_server; i++) \
		putc('\t', debug); \
}

///////////////////////////////////////////////////////////////////////

// Function `exec_query' runs active query for the current server.
// Three arguments are passed to this function: the function's name,
// a debugging message, and a stop flag; if stop flag is non zero
// and the query fails, then the program exits. If stop flag is
// zero, then the function returns zero for query success, or else
// returns the MySQL errno code.

static unsigned int
exec_query(char *func, char *msg, int stop)
{
	char *p;
	unsigned int err;

	if (cur_server < 0)
		fatal("%s: no active server", func);

	// First check for empty query; empty or "white" queries
	// are not acceptable from MySQL server.
	for (p = query[cur_server]; p < cursor[cur_server]; p++) {
		switch (*p) {
		case ' ':	// space
		case '\t':	// tab
		case '\n':	// newline
		case '\r':	// carriage return
		case '\f':	// form feed
			continue;
		default:
			goto NOT_EMPTY_QUERY;
		}
	}

	// Query was empty or "white"; no normal character found.
	// Reset cursor and return.
	cursor[cur_server] = query[cur_server];
	return(0);
			
NOT_EMPTY_QUERY:
	// End current query and reset the query cursor for future use.
	*cursor[cur_server] = '\0';
	cursor[cur_server] = query[cur_server];
#ifdef SPAWK_DEBUG
	fprintf(stderr, "%sEXECUTING %s QUERY FOR SERVER %d:\n"
		"%s\n*** End of Query ***\n", debug_string, msg,
		n_server, p);
	fflush(stderr);
#endif
	if (debug != NULL) {
		register char *s;

		putc('\n', debug);
		debug_level();
		fprintf(debug, "%sQUERY FOR SERVER %d\n",
			comment_string, n_server);
		debug_level();
		for (s = p; *s; s++)
			putc(*s, debug);

		putc(';', debug);
		putc('\n', debug);
		fflush(debug);
	}

	affected[cur_server] = 0;
	if (!mysql_query(conn[cur_server], p)) {
		if (mysql_errno(conn[cur_server]))
			fatal("%s: mysql_errno vs mysql_query "
				"(%d -- %s)", func,
				mysql_errno(conn[cur_server]),
				mysql_error(conn[cur_server]));

		return(0);
	}

	if (!(err = mysql_errno(conn[cur_server])))
		fatal("%s: mysql_query vs mysql_errno", func);

	if (stop)
		fatal("%s: SQL error (%d -- %s)", func, err,
			mysql_error(conn[cur_server]));

	return(err);
}

///////////////////////////////////////////////////////////////////////

// do_spawk_update --- execute update query for current server

// This function executes the current query which shall not return
// results; inserts, updates and deletes must be executed via
// `spawk_update'. We can pass as first argument to this function
// the last part of the query (or a complete query). We can also
// pass a second argument which is a number to be used as program's
// exit status in case of failure; if we don't pass a second
// argument, then the program doesn't exit in case of query failure.

// USAGE
// spawk_update([query [, status]])

// RETURNS
// Zero on success, else MySQL errno (number).

static NODE *
do_spawk_update(NODE *tree)
{
	int argc;	// arguments' count
	int err;
	int stat;
	int eoe = 0;	// exit on error

	SPAWK_INIT();
	if (((argc = get_curfunc_arg_count()) > 2) && do_lint)
		lintwarn("update: must be called with "
			"at most two arguments");

	switch (argc) {
	case 2:
		stat = (int)force_number(get_scalar_argument
			(tree, 1, FALSE));
		eoe = 1;
	case 1:
		add_query(tree);
	case 0:
		break;
	default:
		fatal("update: invalid # of arguments");
	}

	affected[cur_server] = 
		((err = exec_query("update", "UPDATE", 0)) ? 0 :
		mysql_affected_rows(conn[cur_server]));
	if (err && eoe) {
		warning("update: SQL failed (%d -- %s)", err,
			mysql_error(conn[cur_server]));
		exit(stat);
	}

	pop_server();
	set_value(tmp_number((AWKNUM) err));
	return(tmp_number((AWKNUM) 0));
}
