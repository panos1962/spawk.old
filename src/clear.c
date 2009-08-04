///////////////////////////////////////////////////////////////////////

// do_spawk_clear --- clear data and release current server

// USAGE
// spawk_clear([all])

// RETURNS
// Current server (number).

static NODE *
do_spawk_clear(NODE *tree)
{
	int argc;

	SPAWK_INIT();
	if (((argc = get_curfunc_arg_count()) > 1) && do_lint)
		lintwarn("clear: cannot be called with arguments");

	if (argc > 0) {
		while (release_server(debug))
			;
	}
	else
		(void)release_server(debug);

	set_value(tmp_number((AWKNUM) n_server));
	return(tmp_number((AWKNUM) 0));
}
