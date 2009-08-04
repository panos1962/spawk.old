///////////////////////////////////////////////////////////////////////

// do_spawk_affected --- number of rows affected by the last update

// That function forms an intreface to `mysql_affected_rows' MySQL
// C API function. By default, this function returns the number of
// rows actually changed, not the number of rows involved in the
// updates. If we want the number of rows involved in the updates,
// then we must create `SPAWKINFO["CFR"]' element before loading
// the SPAWK module.

// USAGE
// spawk_affected()

// RETURNS
// Number of rows affected by the last update (number).

static NODE *
do_spawk_affected(NODE *tree)
{
	NODE *arg;

	SPAWK_INIT();
	if ((get_curfunc_arg_count() > 0) && do_lint)
		lintwarn("affected: must be called without arguments");

	// The server executed the last (update) query is
	// now popped out, that's the reason of taking the
	// `n_server' indexed element instead of `cur_server'.
	set_value(tmp_number((AWKNUM) affected[n_server]));
	return(tmp_number((AWKNUM) 0));
}
