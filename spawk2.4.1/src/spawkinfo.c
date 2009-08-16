///////////////////////////////////////////////////////////////////////

// `load_spawkinfo' function is being used to read MySQL configuration
// files (`/etc/my.cnf' and `~/.my.cnf'). The parameters will concern
// in [client] and [spawk] groups as well as in any other group named
// by the name specified via `spawk_program' call. Note that the
// configuration files are being read only the first time we call
// `spawk_program' function or another SPAWK function (except
// `spawk_debug'). The parameters that concern the given program are
// being held in `SPAWKINFO' array just as environment variables are
// being held in `ENVIRON' array. Program name may be specified using
// `SPAWKINFO' "program" indexed element instead of calling the
// `spawk_program' function.

static void
load_spawkinfo(void)
{
	int argc;
	char **argv = NULL;
	int i;
	NODE **aptr;
	char *p;
	char *q;
	int l;
	char *db = NULL;
	char *groups[2];
	NODE conf;
	NODE *cf;
	int verbose = 0;

	// Program must have a valid name at this point.
	if (myname == NULL)
		fatal("spawkinfo: null `myname'");

#ifdef SPAWK_DEBUG
	fprintf(stderr, "%sLOADING `SPAWKINFO' FOR `%s'\n",
		debug_string, myname);
#endif

	// If a "no-defaults" indexed element exists in `SPAWKINFO',
	// then no configuration file is been read.
	if (in_array(spawkinfo, tmp_string("no-defaults", 11)) != NULL)
		return;

	// Initialize arguments' vector with program's name
	// and no other arguments.
	if ((argv = calloc(2, sizeof(char *))) == NULL)
		fatal("spawkinfo: cannot allocate arguments' vector");

	if ((argv[0] = malloc(strlen(myname) + 1)) == NULL)
		fatal("spawkinfo: cannot allocate argument's buffer");

	strcpy(argv[0], myname);
	argv[argc = 1] = NULL;

/*** ARNOLD ***/
	// We'll need an array to check if an item is inserted
	// now for the first time or was inserted earlier in
	// that same procedure. We construct that array (named
	// `conf') by hand.
	conf.type = Node_var_array;
	conf.var_array = NULL;
	conf.table_size = 0;
	conf.array_size = 0;
	conf.flags = 0;

	// It's time to read MySQL configuration files (`/etc/my.cnf'
	// and `~/.my.cnf'). The order is as follows:
	//
	//	[client]	After all this (awk) process is a
	//			client process.
	//
	//	[spawk]		All awk processes using SPAWK module
	//			belong to this group created just for
	//			that purpose.
	//
	// After reading the above groups' parameters we scan the
	// configuration files one more time to check for parameters
	// related to the given program (`myname'). Idiosyncrasies
	// of `load_defaults' dictates reverse ordering.
	groups[1] = NULL;
	groups[0] = (char *)myname;
	load_defaults("my", (const char **)groups, &argc, (&argv));
	groups[0] = "spawk";
	load_defaults("my", (const char **)groups, &argc, (&argv));
	groups[0] = "client";
	load_defaults("my", (const char **)groups, &argc, (&argv));

	// If a "print_defaults" configuration variable is defined,
	// then the variables will be printed to the standard error
	// when read and set.
	verbose = (in_array(spawkinfo,
		tmp_string("print-defaults", 14)) != NULL);

	for (i = 1; i < argc; i++) {
		if (argv[i] == NULL)
			fatal("spawkinfo: argument[%d] is null", i);

#ifdef SPAWK_DEBUG
		fprintf(stderr, "\t%sARGV[%d]: %s\n",
			debug_string, i, argv[i]);
#endif
		if (memcmp(argv[i], "--", 2))
			fatal("spawkinfo: %s: argument[%d] not in "
				"`--' format", argv[i], i);

		for (l = 0, p = (q = (argv[i] + 2)); *p != '\0'; p++) {
			if (*p == '=')
				break;
		}

		if ((l = p - q) <= 0)
			continue;

		if (*p == '=')
			p++;

		// We use `conf' array to hold now inserted
		// `SPAWKINFO' items. We do that in order not
		// to alter items already in `SPAWKINFO' array
		// before `load_spawkinfo' called. So, the tactics
		// is as follows: for each configuration parameter
		// insert it into `SPAWKINFO' only if that parameter
		// is not already there (the index of course), or
		// was inserted earlier in that same procedure.
		cf = NULL;
/*** ARNOLD ***/
		if ((in_array(spawkinfo, tmp_string(q, l)) != NULL) &&
			((cf = in_array(&conf,
			tmp_string(q, l))) == NULL))
			continue;

		// If not marked in `conf' mark it now.
		if (cf == NULL)
			assoc_lookup(&conf, tmp_string(q, l), FALSE);

		if (in_array(spawkinfo, tmp_string(q, l)) != NULL)
			do_delete(spawkinfo, tmp_string(q, l));

		aptr = assoc_lookup(spawkinfo, tmp_string(q, l), FALSE);
		*aptr = make_string(p, strlen(p));
		if (verbose)
			fprintf(stderr, "%.*s = \"%s\"\n", l, q, p);
	}

	assoc_clear(&conf);
	free_defaults(argv);
}
