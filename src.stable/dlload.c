///////////////////////////////////////////////////////////////////////

// The `dlload' function is the dummy function to load causing the
// dynamic link of the SPAWK module as a side-effect. This is the
// standard way for GNU awk to load extension libraries.

// dlload --- load new builtins in this library

NODE *
dlload(tree, dl)
NODE *tree;
void *dl;
{
#ifdef SPAWK_DEBUG
	fprintf(stderr, "%sLOADING SPAWK EXTENSION\n", debug_string);
	fflush(stderr);
#endif

	// Use `spawkinfo' pointer as a flag. The first time this
	// function is called, `spawkinfo' is created or correlated
	// with the awk `SPAWKINFO' array.
	if (spawkinfo != NULL)
		return;

	// Check for `SPAWKINFO' array; if not there, create it
	// now. But what does "not there" means? How can the array
	// be there already? Simple: if the array `SPAWIKINFO'have
	// been used already in the calling awk script, then the
	// array is "already there". In that case we only correlate
	// that array with the `spawkinfo' pointer.
	if (((spawkinfo = variable(SPAWKINFO, 0,
		Node_var_array)) == NULL) ||
		(spawkinfo->type != Node_var_array))
		fatal("%s: not an array", SPAWKINFO);

	// Check `SPAWKINFO' for "program" indexed element and
	// if exists, then use it as the program's name.
	set_progname();
	make_builtin("spawk_debug", do_spawk_debug, 2);
	make_builtin("spawk_program", do_spawk_program, 1);
	make_builtin("spawk_query", do_spawk_query, 1);
	make_builtin("spawk_update", do_spawk_update, 2);
	make_builtin("spawk_select", do_spawk_select, 2);
	make_builtin("spawk_results", do_spawk_results, 1);
	make_builtin("spawk_data", do_spawk_data, 2);
	make_builtin("spawk_first", do_spawk_first, 2);
	make_builtin("spawk_last", do_spawk_last, 2);
	make_builtin("spawk_clear", do_spawk_clear, 1);
	make_builtin("spawk_affected", do_spawk_affected, 0);
	make_builtin("spawk_string", do_spawk_string, 1);
	make_builtin("spawk_errno", do_spawk_errno, 0);
	make_builtin("spawk_error", do_spawk_error, 0);
	make_builtin("spawk_server", do_spawk_server, 0);
	return tmp_number((AWKNUM) 0);
}
