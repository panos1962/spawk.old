///////////////////////////////////////////////////////////////////////

// Debug files are iniated with the `spawk_debug' function. Internally
// we maintain the file pointer `debug' which is coupled with that
// file. If this pointer is null, then debugging is off, else the
// pointer is used to write messages to the debug file (where the
// `debug' pointer points).

static void
close_debug(void)
{
#ifdef SPAWK_DEBUG
	fprintf(stderr, "%sCLOSE DEBUG FILE\n", debug_string);
	fflush(stderr);
#endif

	fprintf(debug, "%sclosing debug file\n", comment_string);
	if (fclose(debug))
		fatal("close_debug: %s", strerror(errno));

	debug = NULL;
}

///////////////////////////////////////////////////////////////////////

// do_spawk_debug --- open/close debug file

// Function `spawk_debug' takes as its first argument the name of a
// file to be used as query log. This file can be used for debugging
// purposes as well as for any other reasons. Other information,
// except queries, are written to the debug file as SQL commnets.
//
// If there exists a second argument (even empty or zero), then the
// file is truncated, else the queries will be appended to the
// existing file. If there are no arguments at all, then the logging
// is turned off. We can, of course, turn it on again later, or change
// it to another file etc.

// EXAMPLE
// spawk_debug("panos1001", "create")
//
// Creates new debug file `panos1001'. This file will accept all
// queries sent to the SQL servers.

// EXAMPLE
// spawk_debug("panos1001")
//
// Appends to debug file `panos1001'. This file will accept all
// queries sent to the SQL servers.

// EXAMPLE
// spawk_debug()
//
// Closes current debug file.

// USAGE
// spawk_debug([file [, create]])

// RETURNS
// Zero (number).

static NODE *
do_spawk_debug(NODE *tree)
{
	int argc;
	NODE *arg;
	char *mode = "a";
	char *msg = NULL;

	if (((argc = get_curfunc_arg_count()) > 2) && do_lint)
		lintwarn("debug: must be called with at most "
			"two arguments");

	switch (argc) {
	case 2:
		mode = "w";
		msg = "SPAWK Debug Log File";
	case 1:
		if (debug != NULL)
			close_debug();

		force_string(arg = get_scalar_argument(tree, 0, FALSE));
		if (arg->stlen <= 0)
			fatal("debug: null debug file name");

#ifdef SPAWK_DEBUG
		fprintf(stderr, "%sOPEN DEBUG FILE `%s'\n",
			debug_string, arg->stptr);
		fflush(stderr);
#endif
		if ((debug = fopen(arg->stptr, mode)) == NULL)
			fatal("debug: %s: %s", arg->stptr,
				strerror(errno));

		free_temp(arg);
		// If the file is new, then write a header message.
		if (msg != NULL) {
			fprintf(debug, "%s%s", comment_string, msg);
			if (myname != NULL)
				fprintf(debug, " (%s)", myname);

			putc('\n', debug);
			fflush(debug);
		}
		break;
	case 0:
		if (debug == NULL)
			warning("debug: file not open");
		else
			close_debug();
		break;
	default:
		fatal("debug: invalid # of arguments");
	}

	set_value(tmp_number((AWKNUM) 0));
	return(tmp_number((AWKNUM) 0));
}
