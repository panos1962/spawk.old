///////////////////////////////////////////////////////////////////////

// Function `prepare_results' prepares next result set for use. That
// means preparing data to be fetched with `mysql_fetch_row' calls.
// It's very important to set the `busy' flag, meaning that the current
// server is returning results and cannot accept any new SQL requests
// until all the data have been returned (or discarded).
//
// For code density reasons, the function returns the number of
// columns in the result set as an awk return NODE, so this value
// can be used directly as other functions' return value.

static NODE *
prepare_results(char *func)
{
	int err;

	field_count[cur_server] = mysql_field_count(conn[cur_server]);
	if (((res_set[cur_server] =
		mysql_use_result(conn[cur_server])) == NULL) &&
		(err = mysql_errno(conn[cur_server])))
		fatal("%s: SQL error (%d -- %s)\n", func, err,
			mysql_error(conn[cur_server]));

	busy[cur_server] = 1;
	set_value(tmp_number((AWKNUM) field_count[cur_server]));
	return(tmp_number((AWKNUM) 0));
}

///////////////////////////////////////////////////////////////////////

static void
clear_results(const char *func)
{
	int err;

	if (conn[cur_server] == NULL)
		fatal("%s: no active server", func);

	// As said to the C API Reference Manual of MySQL, when
	// `mysql_use_result' is used (which is our case), the
	// `mysql_free_result' call automatically fetches and
	// discards all remainig rows.
	if (res_set[cur_server] != NULL) {
		mysql_free_result(res_set[cur_server]);
		res_set[cur_server] = NULL;
	}

	// `mysql_next_result' returns zero if there is another
	// statement in the queue which is not our case; this
	// call must return -1 which means no other statement
	// in the queue, everything is ok.
	if (!(err = mysql_next_result(conn[cur_server])))
		fatal("%s: multiple statements not allowed", func);

	if (err != -1)
		fatal("%s: mysql_next_result failed (%d -- %s) ", func,
			mysql_errno(conn[cur_server]),
			mysql_error(conn[cur_server]));
}

///////////////////////////////////////////////////////////////////////

// do_spawk_results --- prepare results for current server

// Function `do_spawk_results' is similar to `do_spawk_select' but
// operates on the data produced by the last query given with
// `do_spawk_update'. This way we can test the query and then fetch
// the results in contrary with `do_spawk_select' which exits if
// something is wrong in the query. We can pass an argument to
// `spawk_results' function. That argument will be used as index
// string just as the second argument passed in `spawk_select'.

// USAGE
// spawk_results([index])

// RETURNS
// Number of columns in result rows (number).

static NODE *
do_spawk_results(NODE *tree)
{
	int argc;	// arguments' count
	NODE *arg;

	SPAWK_INIT();
	if (((argc = get_curfunc_arg_count()) > 1) && do_lint)
		lintwarn("results: must be called with "
			"at most one argument");

	// After query execution, either select or update, we popped
	// down the server. Calling now the `spawk_results' function
	// must push up the server previously popped down. We call
	// `add_query' with no query just to push up the last server.
	add_query(NULL);
	switch (argc) {
	case 1:
		force_string(arg = get_scalar_argument(tree, 0, FALSE));
		(void)set_data_index(arg->stptr, arg->stlen);
		free_temp(arg);
		break;
	case 0:
		// Use numeric (default) indexing scheme for the
		// results and null array.
		(void)set_data_index(NULL, 0);
		break;
	default:
		fatal("results: invalid # of arguments");
	}

	return(prepare_results("results"));
}
