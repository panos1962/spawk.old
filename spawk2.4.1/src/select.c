///////////////////////////////////////////////////////////////////////

// do_spawk_select --- execute query and prepare results

// This function executes the current query (query given to the
// current server) and prepares the results for future coming data
// requests (`spawk_data', `spawk_first', `spawk_last'). Function
// takes two arguments (at most): the first argument is the last
// part of the query (may be empty, or a complete query), while the
// second argument is the data index string (comma separated string
// where each field is being an index to use in returned data arrays).
// If the index is missing, then the results are indexed as follows:
// 0 for the whole row (record), 1 for the first column (field), 2 for
// the second column etc.

// USAGE
// spawk_select([query[, index]])

// RETURNS
// Number of columns in result rows (number).

static NODE *
do_spawk_select(NODE *tree)
{
	int argc;	// arguments' count
	NODE *arg;

	SPAWK_INIT();
	if (((argc = get_curfunc_arg_count()) > 2) && do_lint)
		lintwarn("%s: must be called with "
			"at most two arguments", select_string);

	switch (argc) {
	case 2:
		add_query(tree);
		force_string(arg = get_scalar_argument(tree, 1, FALSE));
		(void)set_data_index(arg->stptr, arg->stlen);
		free_temp(arg);
		break;
	case 1:
		add_query(tree);
	case 0:
		(void)set_data_index(NULL, 0);
		break;
	default:
		fatal("%s: invalid # of arguments", select_string);
	}

	exec_query(select_string, "RESULTS", 1);
	return(prepare_results(select_string));
}
