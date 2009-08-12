// do_spawk_program --- set program's name (`myname', `my_progname')

// That function accepts a single argument meaned the program name.
// That name is being used as `myname' for gawk and as `my_progname'
// for MySQL. The given name is also being used for reading the
// MySQL configuration files (typically `/etc/my.cnf' and `~/.my.cnf').
// This same name is being used in error messages too.
//
// Note, that you can call this function many times (thus changing
// name of a given program), but the initialization will only take
// place once (during the first call). Calling the function with no
// arguments at all results in the original name of the program when
// it begun as a process (normally "gawk" or "awk").

// USAGE
// spawk_program([program])

// RETURNS
// Previous program name (string).

static NODE *
do_spawk_program(NODE *tree)
{
	int argc;
	NODE *arg;
	NODE **aptr;

	if (((argc = get_curfunc_arg_count()) > 1) && do_lint)
		lintwarn("program: must be called with at most "
			"one argument");

	// Check `SPAWKINFO' for "program" indexed element. If
	// found, then assume that element's value as the program's
	// name until now.
	set_progname(0);

	// The function will return the current program name.
	set_value(tmp_string((char *)myname, strlen(myname)));

	switch (argc) {
	case 1:
		force_string(arg = get_scalar_argument(tree, 0, FALSE));
		aptr = assoc_lookup(spawkinfo,
			tmp_string(program_string, PROGRAM_LEN), FALSE);
		*aptr = make_string((char *)(myname = arg->stptr),
			arg->stlen);
		free_temp(arg);
	case 0:
		break;
	default:
		fatal("program: invalid # of arguments");
	}

	my_progname = myname;
	SPAWK_INIT();
	return(tmp_number((AWKNUM) 0));
}
