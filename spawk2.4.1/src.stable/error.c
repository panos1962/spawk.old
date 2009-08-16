///////////////////////////////////////////////////////////////////////

// do_spawk_errno --- returns last SQL error code

// USAGE
// spawk_errno()

// RETURNS
// The error code (number).

static NODE *
do_spawk_errno(NODE *tree)
{
	int argc;

	SPAWK_INIT();
	if (((argc = get_curfunc_arg_count()) != 0) && do_lint)
		lintwarn("errno: must be called with no arguments");

	switch (argc) {
	case 0:
		break;
	default:
		fatal("errno: invalid # of arguments");
	}

	set_value(tmp_number((AWKNUM) (conn[n_server] == NULL ?
		0 : mysql_errno(conn[n_server]))));
	return(tmp_number((AWKNUM) 0));
}

///////////////////////////////////////////////////////////////////////

// do_spawk_error --- returns last SQL error message

// USAGE
// spawk_error()

// RETURNS
// The error message (string).

static NODE *
do_spawk_error(NODE *tree)
{
	int argc;

	SPAWK_INIT();
	if (((argc = get_curfunc_arg_count()) != 0) && do_lint)
		lintwarn("error: must be called with no arguments");

	switch (argc) {
	case 0:
		break;
	default:
		fatal("error: invalid # of arguments");
	}

	if (conn[n_server] == NULL)
		set_value(tmp_string(empty_string, 0));
	else
		set_value(tmp_string((char *)
			mysql_error(conn[n_server]),
			strlen(mysql_error(conn[n_server]))));

	return(tmp_number((AWKNUM) 0));
}
