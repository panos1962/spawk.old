///////////////////////////////////////////////////////////////////////

// Function `add_query' adds query statements to the current query.
// This function is called from `spawk_query', `spawk_select' and
// `spawk_update' functions. All these functions may be called with
// an argument which is a query section to be added to the current
// query (or a complete query). So, it's reasonable for `add_query'
// to take as argument the arguments list of these functions and
// process the first argument.

static void
add_query(NODE *tree)
{
	NODE *arg = NULL;

	// This is the critical point of the SPAWK concept.
	// If there isn't any active server yet, or if the
	// current server is busy serving data from some
	// previously given SQL request, then push a new
	// server (connection) to serve this new request.
	if ((cur_server < 0) || busy[cur_server])
		push_server();

	// Given a null argument, `add_query' has only the side
	// effect to push and make current a new server.
	if (tree == NULL)
		return;

	force_string(arg = get_scalar_argument(tree, 0, FALSE));
	if (arg->stlen <= 0)
		goto RETURN;

	if (((cursor[cur_server] - query[cur_server]) +
		arg->stlen) >= max_query_len)
		fatal("add_query: huge query (over %u characters)",
			max_query_len);


	// Add given query part to the current query. No need
	// for termination; this will take place later at a higher
	// level function.
	memcpy(cursor[cur_server], arg->stptr, arg->stlen);
	cursor[cur_server] += arg->stlen;

RETURN:
	free_temp(arg);
}

///////////////////////////////////////////////////////////////////////

// do_spawk_query --- add query for later execution to current server

// Function `do_spawk_query' is passed a string which forms part
// of a query (or forms a complete query) and passes this string
// to the current server. Actually, nothing is sent to the server,
// but the string is accumulated to the current server's query
// buffer (array `query') and the current server's cursor (pointer
// `cursor') is updated accordingly. That function is just an awk
// extension interface to `add_query' internal function.

// USAGE
// spawk_query(query)

// RETURNS
// Current server (number).

static NODE *
do_spawk_query(NODE *tree)
{
	int argc;

	SPAWK_INIT();
	if (((argc = get_curfunc_arg_count()) != 1) && do_lint)
		lintwarn("query: must be called with one "
			"argument");

	if (argc != 1)
		fatal("query: invalid # of arguments");

	add_query(tree);
	set_value(tmp_number((AWKNUM) n_server));
	return(tmp_number((AWKNUM) 0));
}
