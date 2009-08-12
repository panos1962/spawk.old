///////////////////////////////////////////////////////////////////////

// Macro `pop_server' only counts down the server counters.

#define pop_server() n_server = cur_server--

///////////////////////////////////////////////////////////////////////

// Function `cleanup' is called just before the program exits.
// The main reason to call some atexit function is to report
// for unclaimed data; usually this fact denotes some kind of
// bug in the program script.

static void
cleanup(void)
{
	// We don't want error messages when browsing awk's output
	// with a pager and quit, or head some lines etc.
	if (errno == EPIPE) {
		cur_server = -1;
		n_server = 0;
		goto MYSQL_END;
	}
		
	while (cur_server >= 0) {
		if (busy[cur_server])
			warning("unclaimed data for server %d",
				n_server);

		pop_server();
	}

MYSQL_END:
	mysql_library_end();
}

///////////////////////////////////////////////////////////////////////

// We need some kind of signal handler in order to avoid broken pipe
// situations.

static RETSIGTYPE sigcatch(int sig)
{
	errno = EPIPE;
	exit(0);
}

///////////////////////////////////////////////////////////////////////

// Function `init' will be called the first time we need to connect
// to the database pushing a server.

static void
init(void)
{
	int i;

#ifdef SPAWK_DEBUG
	fprintf(stderr, "%sINITIATING SPAWK LIBRARY\n", debug_string);
	fflush(stderr);
#endif
	set_progname(0);
	MY_INIT(myname);
	atexit(cleanup);
	if (mysql_library_init(0, NULL, NULL))
		fatal("init: mysql_library_init: failed");

	signal(SIGPIPE, sigcatch);
	for (i = 0; i < SPAWK_MAX_SERVER; i++) {
		conn[i] = NULL;
		query[i] = NULL;
		cursor[i] = NULL;
		busy[i] = 0;
		res_set[i] = NULL;
		field_count[i] = 0;
		data_index[i] = NULL;
		affected[i] = 0;
	}

	// Remember `affected' array has one extra element?
	affected[SPAWK_MAX_SERVER] = 0;
	load_spawkinfo();

	// `cur_server' was initially set to -2. This was used
	// as a flag in order not to run `init' for a second time.
	// Correct `cur_server' values is `n_server' minus one:
	cur_server = -1;
}

///////////////////////////////////////////////////////////////////////

// Never call `init' function directly. We prefer to call `SPAWK_INIT'
// macro which checks the `cur_server' counter before calling `init'
// in order to ensure that `init' will be called no more than one
// time only.

#define SPAWK_INIT() { \
	if (cur_server < -1) \
		init(); \
}
