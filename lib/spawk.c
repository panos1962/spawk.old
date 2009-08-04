///////////////////////////////////////////////////////////////////////
//
// Wed Jul  8 15:15:06 EEST 2009
//
// Panos I. Papadopoulos (C) 2009-
// 
// The sources of SPAWK concatenated in a single roll.
// Sources marked `stable', e.g. `src.stable/global.c',
// are more stable than others.
//
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
//
// SOURCE: src.stable/global.c
//
///////////////////////////////////////////////////////////////////////

#include "my_global.h"
#include "my_sys.h"
#include "mysql.h"
#include "mysqld_error.h"

// The following symbolic constants have been defined in MySQL
// header files above. These symbolic constants are defined in
// gawk header files also. So, I undefine them now and proceed
// with the gawk (re)definitions. This must not be a problem
// assuming that MySQL macros used in this code (if any) don't
// use these symbolic constants.

#undef HAVE_LANGINFO_CODESET
#undef HAVE_MBRLEN
#undef HAVE_MBRTOWC
#undef PACKAGE
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#undef VERSION

#include "awk.h"

///////////////////////////////////////////////////////////////////////

#define SPAWK_MAX_SERVER 16			// max # of servers
static unsigned int max_query_len = 16380;	// max query length
static unsigned int max_row_len = 16380;	// max row length

///////////////////////////////////////////////////////////////////////

// Results are returned from SQL queries in arrays indexed as follows:
// 0 for the whole row (record), 1 for the first column (field), 2 for
// the second column etc. We can, of course, change this default
// behavior supplying a comma separated index string as `spawk_select's
// second argument. In that case the array is indexed with "" (empty
// string) for the whole record, and the indices supplied for the
// columns. If there are more columns than the supplied indices, then
// the extra columns are been discarded. The indices supplied in the
// way we just described are kept in separate `DATA_INDEX' chains for
// every server. Null root node for a specific server means number
// indexing (default) for this server.

typedef struct DATA_INDEX {
	char *idx;			// index string
	size_t len;			// index string's length
	struct DATA_INDEX *next;	// next column's index node
} DATA_INDEX;

///////////////////////////////////////////////////////////////////////

// The following variables are considered global in the code but the
// scope is that source file only (static).

// We use both `n_server' as a server counter and `cur_server' as
// the current server cursor in the server stack. These two
// variables must always differ by one, e.g. when `n_server' is 3,
// `cur_server' is 2, when `n_server' is 6, `cur_server' is 5 etc.
// However, this is not true at the begining; `cur_server' must be
// set to -1 to have things in order, but we set `cur_server' to -2
// initially to be used as a flag; actually this flag will be used
// later to initialize the SPAWK module. During this initialization
// the `cur_server' will be set to -1 which is the correct initial
// value for this cursor. Finally, the reason for using two variables
// instead of one is efficiency; sometimes it's better to use
// `n_server', other times it's more convenient to use `cur_server'.
// We only must take care always to increment or decrement both of
// these two variables.
static int n_server = 0;
static int cur_server = -2;

// The following arrays are various server attributes. We could, of
// course, have defined a structure to describe servers and use a single
// array of this structure data type, but we preferred to use separate
// arrays instead.
static MYSQL *conn[SPAWK_MAX_SERVER];
static char *query[SPAWK_MAX_SERVER];
static char *cursor[SPAWK_MAX_SERVER];
static int busy[SPAWK_MAX_SERVER];
static MYSQL_RES *res_set[SPAWK_MAX_SERVER];
static int field_count[SPAWK_MAX_SERVER];
static DATA_INDEX *data_index[SPAWK_MAX_SERVER];

// The following array may have one extra place in order to avoid
// extra checks. By the way, the `affected' server attribute denotes
// the number of rows affected by the last update. Because this
// attribute concerns the last popped server, having one extra place
// in this array, we can take the next server's attribute as the
// requested value without warring if this is the last server in the
// stack.
static my_ulonglong affected[SPAWK_MAX_SERVER + 1];

// Parameter values in MySQL configuration files (`/etc/my.cnf'
// and `~/.my.cnf') can be set for [client] and [spawk] groups. We
// can, of course, set other parameters as well concerning specific
// program names, e.g. [payroll], [bank] etc. These parameters
// will become part of the awk process after invoking `spawk_program'
// (setting the program's name), or calling the first spawk function
// (except `spawk_debug'). The parameters will be loaded in the awk
// `SPAWKINFO' array just like the environment variables are loaded
// in `ENVIRON' array. The `password_prompt' variable is being used
// to prompt for MySQL password, while `OFS' variable is being used
// as output column separator in SQL queries that return results.
// If not defined, then the default awk `OFS' is being used instead.
// A list of SPAWK specific parameters follows:
//
//	password_prompt	The prompt to be used when asking for
//			password to connect. If not specified,
//			then the default prompt is used.
//
//	OFS		Column separator for returned rows. If not
//			specified, then the "OFS" awk variable is
//			used.
//
//	null		A string to be used for null valued columns
//			when returning rows. If not specified, then
//			nothing is printed for null valued columns.
//
//	max_query_len	The maximum length for any given query to
//			be processed by SPAWK. Default is 16380
//			characaters.
//
//	max_row_len	The maximum length for any returned row.
//			Default is 16380 characaters.

static NODE *spawkinfo = NULL;

// The `debug' file pointer is being used to write messages in a
// debug file. Function `spawk_debug' will help in opening, closing
// or appending messages to a debug file.
static FILE *debug = NULL;

// The following variables are defined only to avoid constant string
// repetitions in the code.
static char *empty_string = "";
static char *comment_string = "-- ";
static char *ofs_string = "OFS";
#define OFS_LEN 3
static char *null_string = "null";
#define NULL_LEN 4
static char *SPAWKINFO = "SPAWKINFO";
static char *select_string = "select";
static char *program_string = "program";
#define PROGRAM_LEN 7
static char *debug_string = "[[DBG]] ";

///////////////////////////////////////////////////////////////////////
//
// SOURCE: src.stable/debug.c
//
///////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////
//
// SOURCE: src/spawkinfo.c
//
///////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////
//
// SOURCE: src/progname.c
//
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////

// Function `set_progname' seeks `SPAWKINFO' array for a "program"
// indexed element. If found such an element, then it's assumed as
// the program's name. If not, then the current program name is
// inserted in `SPAWKINFO' indexed "program".
//
// After calling `set_progname' it's guaranteed that the program's
// name and `SPAWKINFO's "program" indexed element coincide.

static void
set_progname(void)
{
	NODE *arg;

	// It's assumed that the program has a name at this time;
	// if not then some bizare situation happens.
	if (myname == NULL)
		fatal("set_progname: null `myname'");

	// If there is a `SPAWKINFO' element indexed "program", then
	// use it as the program's name.
	if ((arg = in_array(spawkinfo,
		tmp_string(program_string, PROGRAM_LEN))) != NULL)
		myname = arg->stptr;
	else {
		NODE **aptr;
		aptr = assoc_lookup(spawkinfo,
			tmp_string(program_string, PROGRAM_LEN), FALSE);
		*aptr = make_string((char *)myname, strlen(myname));
	}
}

///////////////////////////////////////////////////////////////////////
//
// SOURCE: src.stable/init.c
//
///////////////////////////////////////////////////////////////////////

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
	set_progname();
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

///////////////////////////////////////////////////////////////////////
//
// SOURCE: src.stable/program.c
//
///////////////////////////////////////////////////////////////////////

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
	set_progname();

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

///////////////////////////////////////////////////////////////////////
//
// SOURCE: src.stable/index.c
//
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////

// Function `di_alloc' allocates space for a DATA_INDEX node.
// Supplied argument is the index string pointer. That string
// need not be null terminated; string's length (`len' field)
// will be set later. The `next' field will also be set later.

static DATA_INDEX *
di_alloc(char *idx)
{
	DATA_INDEX *p;

	if ((p = (DATA_INDEX *)malloc(sizeof(DATA_INDEX))) == NULL)
		fatal("di_alloc: out of memory allocating "
			"DATA_INDEX node");

	p->idx = idx;
	return(p);
}

///////////////////////////////////////////////////////////////////////

// Function `set_data_index' is given a comma separated string
// and sets the result columns' indices for the current server.
// If given a null pointer instead of a comma separated string,
// it frees previous index chain for the current server and set
// it to null, meaning number indexing scheme (default).
//
// EXAMPLE
// Given the string "pekey,pename,peaddr,penum" and the corresponding
// length (25), there will be created 4 DATA_INDEX nodes. The whole
// string is being kept in malloced memory and the first (root) node
// will point to this buffer (idx pointer), while other nodes' pointers
// will point to the after comma positions of the same string. Thus,
// when freeing the chain, we only free the string indexed by the
// root node.

static DATA_INDEX *
set_data_index(char *s, size_t l)
{
	DATA_INDEX *p;
	DATA_INDEX *q;
	DATA_INDEX **cur;

	// First thing is to free current server's data index chain.
	if (data_index[cur_server] != NULL) {
		if (data_index[cur_server]->idx != NULL)
			free(data_index[cur_server]->idx);

		for (p = data_index[cur_server]; p != NULL; p = q) {
			q = p->next;
			free(p);
		}
	}

	if ((s == NULL) || (l <= 0))
		return(data_index[cur_server] = NULL);

	data_index[cur_server] = di_alloc(NULL);
	if ((data_index[cur_server]->idx = malloc(l)) == NULL)
		fatal("set_data_index: out of memory allocating "
			"given index string buffer");

	memcpy(data_index[cur_server]->idx, s, l);
	for (cur = data_index + cur_server,
		s = data_index[cur_server]->idx; l > 0; s++, l--) {
		if (*s == ',') {
			// We could terminate the string here:
			//*s = '\0';
			// but there isn't any need for this because
			// we prefer to work with the lengths:
			(*cur)->len = s - (*cur)->idx;
			(*cur)->next = di_alloc(s + 1);
			cur = &((*cur)->next);
		}
	}

	(*cur)->len = s - (*cur)->idx;
	(*cur)->next = NULL;

	// We do not check for double indices, e.g. "key,name,key,zip"
	// is not wrong syntactically, but, of course, is a semantics
	// error. Reason for not check: EFFICIENCY!

#ifdef SPAWK_DEBUG
	fprintf(stderr, "%sDATA INDEX FOR SERVER %d: %.*s (%d)",
		debug_string, n_server,
		data_index[cur_server]->len,
		data_index[cur_server]->idx,
		data_index[cur_server]->len);
	for (p = data_index[cur_server]->next; p != NULL; p = p->next)
		fprintf(stderr, "->%.*s (%d)", p->len, p->idx, p->len);
	putc('\n', stderr);
	fflush(stderr);
#endif
	return(data_index[cur_server]);
}

///////////////////////////////////////////////////////////////////////
//
// SOURCE: src.stable/spawkrc.c
//
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////

// For each new server pushed we can execute initialization commands
// from two files: `/etc/spawkrc' (system wide) and `~/.spawkrc' (per
// user). These commands will be executed by the new server before
// executing any other command. It's good not to produce any results
// from these commands but rather to set attributes, default database
// etc.

static char *spawkrc = NULL;

#define MAX_FILE_NAME 16400

static int
read_init_file(char *dir, char *fil)
{
	static char *rc = NULL;
	FILE *fp;
	int c;
	char fname[MAX_FILE_NAME + 30];

	sprintf(fname, "%.*s/%.16s", MAX_FILE_NAME, dir, fil);
	if ((fp = fopen(fname, "r")) == NULL) {
		if (errno == ENOENT)
			return(0);

		fatal("%s: %s\n", fname, strerror(errno));
	}

	if (spawkrc == NULL) {
		if ((spawkrc = malloc(max_query_len + 2)) == NULL)
			fatal("spawkrc: out of memory");

		rc = spawkrc;
	}

	while ((c = getc(fp)) != EOF) {
		if ((rc - spawkrc) >= max_query_len)
			fatal("spawkrc: huge initialization script");

		*rc++ = c;
	}

	fclose(fp);
	*rc = 0;
	return(0);
}

#undef MAX_FILE_NAME

///////////////////////////////////////////////////////////////////////
//
// SOURCE: src/server.c
//
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////

// The following macros are only for clarity and readability. They
// are used in configuration files' scanning.

#define setparstr(name, len, var) { \
	if ((aptr = in_array(spawkinfo, \
		tmp_string(name, len))) != NULL) { \
		force_string(aptr); \
		var = aptr->stptr; \
		free_temp(aptr); \
	} \
}

#define setparint(name, len, var) { \
	if ((aptr = in_array(spawkinfo, \
		tmp_string(name, len))) != NULL) { \
		force_string(aptr); \
		if ((aptr->stptr != NULL) && \
			isdigit(*(aptr->stptr))) \
			var = (unsigned int) atoi(aptr->stptr); \
		else \
			fatal("%s: invalid parameter value (%s)\n", \
				name, aptr->stptr); \
		free_temp(aptr); \
	} \
}

#define checkvar(var, val, name, fmt) { \
	if (var != val) \
		fprintf(stderr, "\t%s%s = >>" fmt "<<\n", \
			debug_string, name, var); \
}

#define checkmsg(msg) fprintf(stderr, "\t%s%s\n", debug_string, msg)

#define setparssl(name, len, var) { \
	if ((aptr = in_array(spawkinfo, \
		tmp_string(name, len))) != NULL) { \
		force_string(aptr); \
		var = aptr->stptr; \
		free_temp(aptr); \
		use_ssl = 1; \
	} \
}


///////////////////////////////////////////////////////////////////////

// Function `push_server' pushes a new server to serve the
// SQL requests. The server may be completely new, or may
// have been pushed again earlier; if this is the first time
// for the server to be pushed, we establish a connection
// for the server which will remain open even if the server
// is popped out later.

static int
push_server(void)
{
	int err = 0;
	NODE *aptr;
	static int readrc_nok = 1;
	static int conf_nok = 1;
	static char *host = NULL;
	static char *user = NULL;
	static char *password = NULL;
	static char *password_prompt = NULL;
	static char *database = NULL;
	static int password_not_asked_yet = 1;
	static unsigned int port = 0;
	static char *socket_name = NULL;
	static unsigned long flags = 0L;
	static int use_ssl = 0;
	static char *ssl_key = NULL;
	static char *ssl_cert = NULL;
	static char *ssl_ca = NULL;
	static char *ssl_capath = NULL;
	static char *ssl_cipher = NULL;
	static char *skip_ssl = NULL;
	static int ssl = -1;

#ifdef SPAWK_DEBUG
	fprintf(stderr, "%sPUSHING SERVER %d\n",
		debug_string, n_server + 1);
	fflush(stderr);
#endif
	if (n_server >= SPAWK_MAX_SERVER)
		fatal("push_server: maximum # of "
			"connections exceeded (%d)", n_server);

	cur_server = n_server++;
	if (conn[cur_server] != NULL)
		goto SERVER_EXISTS;

	// We are going to establish a new server, i.e. a new
	// database connection. All connections use the same set
	// of parameters, e.g. host name, user name, password,
	// socket name, port number etc. If this is the first
	// time to connect, we get the parameters' values from
	// the `SPAWKINFO' array and keep them in static variables,
	// else we use these values for the new connection.
	if (conf_nok) {
		setparstr("host", 4, host);
		setparstr("user", 5, user);
		setparstr("password", 8, password);
		setparstr("password_prompt", 15, password_prompt);
		setparint("port", 4, port);
		setparstr("socket", 6, socket_name);
		setparint("max_query_len", 13, max_query_len);
		setparint("max_row_len", 11, max_row_len);
		setparssl("ssl-key", 7, ssl_key);
		setparssl("ssl-cert", 8, ssl_cert);
		setparssl("ssl-ca", 6, ssl_ca);
		setparssl("ssl-capath", 10, ssl_capath);
		setparssl("ssl-cipher", 10, ssl_cipher);
		setparssl("skip-ssl", 8, skip_ssl);
		setparint("ssl", 3, ssl);

		// If there is a `SPAWKINFO["CFR"]' element, then
		// the affected number of rows will be considered as
		// the number of the rows invlolved in updates, not
		// only the number of rows actually changed. By default,
		// MySQL returns the number of rows actually changed
		// as the result of `mysql_affected_rows()' function
		// calls.
		if (in_array(spawkinfo, tmp_string("CFR", 3)) != NULL)
			flags |= CLIENT_FOUND_ROWS;

		if (ssl > 0)
			use_ssl = 1;

		if (skip_ssl != NULL)
			use_ssl = 0;

		conf_nok = 0;
	}

	// Database selection takes place in a per server basis. That
	// fact allows setting the database as `SPAWKINFO["database"]'
	// before pushing new servers.
	setparstr("database", 8, database);
#ifdef SPAWK_DEBUG
	fprintf(stderr, "%sINITIATING SERVER %d\n",
		debug_string, n_server);
	checkvar(host, NULL, "host", "%s");
	checkvar(user, NULL, "user", "%s");
	checkvar(password, NULL, "password", "%s");
	checkvar(database, NULL, "database", "%s");
	checkvar(port, 0, "port", "%u");
	checkvar(socket_name, NULL, "socket", "%s");
	checkmsg("SSL Begin");
	checkvar(ssl_key, NULL, "ssl-key", "%s");
	checkvar(ssl_cert, NULL, "ssl-cert", "%s");
	checkvar(ssl_ca, NULL, "ssl-ca", "%s");
	checkvar(ssl_capath, NULL, "ssl-capath", "%s");
	checkvar(ssl_cipher, NULL, "ssl-cipher", "%s");
	checkvar(skip_ssl, NULL, "skip_ssl", "%s");
	checkvar(ssl, 0, "ssl", "%d");
	checkvar(use_ssl, 0, "use_ssl", "%d");
	checkmsg("SSL End");
	fflush(stderr);
#endif

	// This is the first time for that server to be pushed.
	// We establish a database connection and initialize the
	// query buffer. We also set the autocommit to false for
	// the new connection.
	conn[cur_server] = mysql_init(NULL);

#ifdef HAVE_OPENSSL
	if (use_ssl) {
#ifdef SPAWK_DEBUG
		fprintf(stderr, "%sINITIATING SSL\n", debug_string);
#endif
		mysql_ssl_set(conn[cur_server], ssl_key, ssl_cert,
			ssl_ca, ssl_capath, ssl_cipher);
	}
#endif

	while (mysql_real_connect(conn[cur_server], host, user,
		password, database, port, socket_name, flags) == NULL) {
		if ((mysql_errno(conn[cur_server]) ==
			ER_ACCESS_DENIED_ERROR) &&
			password_not_asked_yet) {
			password = get_tty_password(password_prompt);
			password_not_asked_yet = 0;
			continue;
		}

		fatal("push_server: connection failed "
			"for server %d (%d -- %s)", n_server,
			mysql_errno(conn[cur_server]),
			mysql_error(conn[cur_server]));
	}

	// Allocate space for queries to be sent to the newly
	// created server. `cursor' pointer will allways point
	// just after the last character of the current query.
	if ((cursor[cur_server] = (query[cur_server] =
		malloc(max_query_len + 2))) == NULL)
		fatal("push_server: out of memory "
			"allocating query space");

	if (mysql_autocommit(conn[cur_server], FALSE))
		fatal("push_server: mysql_autocommit: (%d -- %s)",
			mysql_errno(conn[cur_server]),
			mysql_error(conn[cur_server]));

	// Every new server will execute the `/etc/spawkrc' and
	// `~/.spawkrc' commands files; these files contain SQL
	// commands (queries) such as "USE...", or "SET..." etc.
	// The purpose of these initialization files is to form
	// the same database environment for all new servers.
	// Multiple queries must be separated by ";" characters.
	if (readrc_nok)
		readrc_nok = read_init_file("/etc", "spawkrc") || \
			read_init_file(getenv("HOME"), ".spawkrc");

	if ((spawkrc != NULL) && (*spawkrc != '\0')) {
#ifdef SPAWK_DEBUG
		fprintf(stderr, "%sINITIALIZING SERVER %d\n",
			debug_string, n_server);
#endif
		// By default, SPAWK servers don't accept multiple
		// queries. Each query must be excuted separately.
		// That's is not the case of initialization files;
		// initialization files may contain multiple queries
		// separated by ";" characters.
		if (mysql_set_server_option(conn[cur_server],
			MYSQL_OPTION_MULTI_STATEMENTS_ON))
			fatal("set_server_option: failed for multi "
				"statements on");

		if (mysql_query(conn[cur_server], spawkrc))
			fatal("init: server %d failed: SQL error "
				"(%d -- %s)", cur_server,
				mysql_errno(conn[cur_server]),
				mysql_error(conn[cur_server]));

		// Multiple commands allowed in `spawkrc' initialization
		// files. Execute all commands in a roll. `spawkrc'
		// commands must not produce any results.
		while (!(err = mysql_next_result(conn[cur_server])))
			;

		if (err != -1)
			fatal("spawkrc: command failed (%d -- %s)\n",
				mysql_errno(conn[cur_server]),
				mysql_error(conn[cur_server]));

		err = 0;

		// Now it's time to reset the (new) server to single
		// query mode, i.e. the server will not accept multiple
		// queries from now on.
		if (mysql_set_server_option(conn[cur_server],
			MYSQL_OPTION_MULTI_STATEMENTS_OFF))
			fatal("set_server_option: failed for multi "
				"statements off");

	}

SERVER_EXISTS:
	// If everything is ok and the housekeeping doesn't leak,
	// then all the following checks must pass.
	if (cursor[cur_server] != query[cur_server]) {
		err++;
		warning("push_server: dirty cursor found "
			"pushing server %d", n_server);
		cursor[cur_server] = query[cur_server];
	}

	if (busy[cur_server]) {
		err++;
		warning("push_server: server %d found busy", n_server);
		busy[cur_server] = 0;
	}

	if (res_set[cur_server] != NULL) {
		err++;
		warning("push_server: server %d found with not "
			"null results' set pointer", n_server);
		mysql_free_result(res_set[cur_server]);
		res_set[cur_server] = NULL;
	}

	if (field_count[cur_server]) {
		err++;
		warning("push_server: server %d found with non "
			"zero fields' count", n_server);
		field_count[cur_server] = 0;
	}

	if (data_index[cur_server] != NULL) {
		err++;
		warning("push_server: server %d found with "
			"not null data index pointer");
		(void)set_data_index(NULL, 0);
	}

	if (err)
		exit(2);

	// Affected number of rows has been purposely left untouched
	// after previous call to the server now re-pushed.
	affected[cur_server] = 0;
	return(n_server);
}

#undef checkvar
#undef checkmsg
#undef setparint
#undef setparstr

///////////////////////////////////////////////////////////////////////

// do_spawk_server --- returns current server

// USAGE
// spawk_server()

// RETURNS
// Current server (number).

static NODE *
do_spawk_server(NODE *tree)
{
	int argc;

	SPAWK_INIT();
	if (((argc = get_curfunc_arg_count()) > 0) && do_lint)
		lintwarn("clear: cannot be called with arguments");

	set_value(tmp_number((AWKNUM) n_server));
	return(tmp_number((AWKNUM) 0));
}

///////////////////////////////////////////////////////////////////////
//
// SOURCE: src/query.c
//
///////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////
//
// SOURCE: src/update.c
//
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////

// When writting in the query log file (debugging on) it's a good idea
// to indent log queries according to the server's depth. Thus, queries
// of server 0 have no indent, queries of server 1 have one-tab indent,
// queries of server 2 have tow-tabs indent etc. The following macro
// does the indenting.

#define debug_level() { \
	int i; \
 \
	for (i = 1; i < n_server; i++) \
		putc('\t', debug); \
}

///////////////////////////////////////////////////////////////////////

// Function `exec_query' runs active query for the current server.
// Three arguments are passed to this function: the function's name,
// a debugging message, and a stop flag; if stop flag is non zero
// and the query fails, then the program exits. If stop flag is
// zero, then the function returns zero for query success, or else
// returns the MySQL errno code.

static unsigned int
exec_query(char *func, char *msg, int stop)
{
	char *p;
	unsigned int err;

	if (cur_server < 0)
		fatal("%s: no active server", func);

	// First check for empty query; empty or "white" queries
	// are not acceptable from MySQL server.
	for (p = query[cur_server]; p < cursor[cur_server]; p++) {
		switch (*p) {
		case ' ':	// space
		case '\t':	// tab
		case '\n':	// newline
		case '\r':	// carriage return
		case '\f':	// form feed
			continue;
		default:
			goto NOT_EMPTY_QUERY;
		}
	}

	// Query was empty or "white"; no normal character found.
	// Reset cursor and return.
	cursor[cur_server] = query[cur_server];
	return(0);
			
NOT_EMPTY_QUERY:
	// End current query and reset the query cursor for future use.
	*cursor[cur_server] = '\0';
	cursor[cur_server] = query[cur_server];
#ifdef SPAWK_DEBUG
	fprintf(stderr, "%sEXECUTING %s QUERY FOR SERVER %d:\n"
		"%s\n*** End of Query ***\n", debug_string, msg,
		n_server, p);
	fflush(stderr);
#endif
	if (debug != NULL) {
		register char *s;

		putc('\n', debug);
		debug_level();
		fprintf(debug, "%sQUERY FOR SERVER %d\n",
			comment_string, n_server);
		debug_level();
		for (s = p; *s; s++)
			putc(*s, debug);

		putc(';', debug);
		putc('\n', debug);
		fflush(debug);
	}

	affected[cur_server] = 0;
	if (!mysql_query(conn[cur_server], p)) {
		if (mysql_errno(conn[cur_server]))
			fatal("%s: mysql_errno vs mysql_query "
				"(%d -- %s)", func,
				mysql_errno(conn[cur_server]),
				mysql_error(conn[cur_server]));

		return(0);
	}

	if (!(err = mysql_errno(conn[cur_server])))
		fatal("%s: mysql_query vs mysql_errno", func);

	if (stop)
		fatal("%s: SQL error (%d -- %s)", func, err,
			mysql_error(conn[cur_server]));

	return(err);
}

///////////////////////////////////////////////////////////////////////

// do_spawk_update --- execute update query for current server

// This function executes the current query which shall not return
// results; inserts, updates and deletes must be executed via
// `spawk_update'. We can pass as first argument to this function
// the last part of the query (or a complete query). We can also
// pass a second argument which is a number to be used as program's
// exit status in case of failure; if we don't pass a second
// argument, then the program doesn't exit in case of query failure.

// USAGE
// spawk_update([query [, status]])

// RETURNS
// Zero on success, else MySQL errno (number).

static NODE *
do_spawk_update(NODE *tree)
{
	int argc;	// arguments' count
	int err;
	int stat;
	int eoe = 0;	// exit on error

	SPAWK_INIT();
	if (((argc = get_curfunc_arg_count()) > 2) && do_lint)
		lintwarn("update: must be called with "
			"at most two arguments");

	switch (argc) {
	case 2:
		stat = (int)force_number(get_scalar_argument
			(tree, 1, FALSE));
		eoe = 1;
	case 1:
		add_query(tree);
	case 0:
		break;
	default:
		fatal("update: invalid # of arguments");
	}

	affected[cur_server] = 
		((err = exec_query("update", "UPDATE", 0)) ? 0 :
		mysql_affected_rows(conn[cur_server]));
	if (err && eoe) {
		warning("update: SQL failed (%d -- %s)", err,
			mysql_error(conn[cur_server]));
		exit(stat);
	}

	pop_server();
	set_value(tmp_number((AWKNUM) err));
	return(tmp_number((AWKNUM) 0));
}

///////////////////////////////////////////////////////////////////////
//
// SOURCE: src/results.c
//
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////

// Function `prepare_results' prepares next result set for use. That
// means preparing data to be fetched with `mysql_fetch_row' calls.
// It's very important to set the `busy' flag, meaning that the current
// server is returning results and cannot accept any new SQL requests
// until all the data have been returned (or discarded).
//
// For code density reasons, the function returns the number of
// columns in the result set as an awk return NODE, so this value
// can be used directly as other functions' return value.

static NODE *
prepare_results(char *func)
{
	int err;

	field_count[cur_server] = mysql_field_count(conn[cur_server]);
	if (((res_set[cur_server] =
		mysql_use_result(conn[cur_server])) == NULL) &&
		(err = mysql_errno(conn[cur_server])))
		fatal("%s: SQL error (%d -- %s)\n", func, err,
			mysql_error(conn[cur_server]));

	busy[cur_server] = 1;
	set_value(tmp_number((AWKNUM) field_count[cur_server]));
	return(tmp_number((AWKNUM) 0));
}

///////////////////////////////////////////////////////////////////////

static void
clear_results(const char *func)
{
	int err;

	if (conn[cur_server] == NULL)
		fatal("%s: no active server", func);

	// As said to the C API Reference Manual of MySQL, when
	// `mysql_use_result' is used (which is our case), the
	// `mysql_free_result' call automatically fetches and
	// discards all remainig rows.
	if (res_set[cur_server] != NULL) {
		mysql_free_result(res_set[cur_server]);
		res_set[cur_server] = NULL;
	}

	// `mysql_next_result' returns zero if there is another
	// statement in the queue which is not our case; this
	// call must return -1 which means no other statement
	// in the queue, everything is ok.
	if (!(err = mysql_next_result(conn[cur_server])))
		fatal("%s: multiple statements not allowed", func);

	if (err != -1)
		fatal("%s: mysql_next_result failed (%d -- %s) ", func,
			mysql_errno(conn[cur_server]),
			mysql_error(conn[cur_server]));
}

///////////////////////////////////////////////////////////////////////

// do_spawk_results --- prepare results for current server

// Function `do_spawk_results' is similar to `do_spawk_select' but
// operates on the data produced by the last query given with
// `do_spawk_update'. This way we can test the query and then fetch
// the results in contrary with `do_spawk_select' which exits if
// something is wrong in the query. We can pass an argument to
// `spawk_results' function. That argument will be used as index
// string just as the second argument passed in `spawk_select'.

// USAGE
// spawk_results([index])

// RETURNS
// Number of columns in result rows (number).

static NODE *
do_spawk_results(NODE *tree)
{
	int argc;	// arguments' count
	NODE *arg;

	SPAWK_INIT();
	if (((argc = get_curfunc_arg_count()) > 1) && do_lint)
		lintwarn("results: must be called with "
			"at most one argument");

	// After query execution, either select or update, we popped
	// down the server. Calling now the `spawk_results' function
	// must push up the server previously popped down. We call
	// `add_query' with no query just to push up the last server.
	add_query(NULL);
	switch (argc) {
	case 1:
		force_string(arg = get_scalar_argument(tree, 0, FALSE));
		(void)set_data_index(arg->stptr, arg->stlen);
		free_temp(arg);
		break;
	case 0:
		// Use numeric (default) indexing scheme for the
		// results and null array.
		(void)set_data_index(NULL, 0);
		break;
	default:
		fatal("results: invalid # of arguments");
	}

	return(prepare_results("results"));
}

///////////////////////////////////////////////////////////////////////
//
// SOURCE: src/select.c
//
///////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////
//
// SOURCE: src/return.c
//
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////

// The variable `sepstr' serves as column delimiter in the returned
// data row. That string may be an arbitrary string and defaults to
// `SPAWKINFO["OFS"]' or "OFS". Variable `seplen' is the length of
// the `sepstr' string.

static char *sepstr = NULL;
static size_t seplen = 0;

// The variable `nullstr' serves as null string in the returned
// data row. That string may be an arbitrary string and defaults to
// `SPAWKINFO["null"]'. Variable `nulllen' is the length of the
// `nullstr' string.

static char *nullstr = NULL;
static size_t nulllen = 0;

// Before each time we fetch data from SQL servers, we reset the
// column delimiter to `SPAWKINFO["OFS"]' string. If this array
// element is undefined, then we use current awk's "OFS" instead.
// We also reset null string at this time.

static void separator(void)
{
	NODE *arg;

	if ((arg = in_array(spawkinfo,
		tmp_string(ofs_string, OFS_LEN))) != NULL) {
		if ((arg = force_string(arg)) == NULL)
			fatal("separator: %s[\"%s\"] undefined",
				SPAWKINFO, ofs_string);
	}
	else if ((arg = force_string(OFS_node->var_value)) == NULL)
		fatal("separator: OFS undefined");

	sepstr = arg->stptr;
	seplen = arg->stlen;

	if ((arg = in_array(spawkinfo,
		tmp_string(null_string, NULL_LEN))) != NULL) {
		if ((arg = force_string(arg)) == NULL)
			fatal("null: %s[\"%s\"] undefined",
				SPAWKINFO, null_string);

		nullstr = arg->stptr;
		nulllen = arg->stlen;
	}
	else {
		nullstr = empty_string;
		nulllen = 0;
	}
}

///////////////////////////////////////////////////////////////////////

// The following function exists only for clarity, readability and
// economy reasons.

static void huge_row(void)
{
	fatal("huge row returnd for server %d (exceeds %u characters)",
		n_server, max_row_len);
}

///////////////////////////////////////////////////////////////////////

// Function `construct_row' takes as argument a fetched data row
// and returns a NODE containing the row as a `SPAWKINFO["OFS"]'
// (or "OFS") separated string.

static NODE *
construct_row(MYSQL_ROW row)
{
	int i;
	static char *buf = NULL;
	char *s;
	int l;

	if ((buf == NULL) &&
		((buf = malloc(max_row_len + 2)) == NULL))
		fatal("construct_row: cannot allocate row buffer");

	for (s = buf, l = 0, i = 0; i < field_count[cur_server]; i++) {
		register char *p;

		if (i > 0) {
			if ((l += seplen) >= max_row_len)
				huge_row();

			memcpy(s, sepstr, seplen);
			s += seplen;
		}

		if (row[i] == NULL) {
			if (nulllen > 0) {
				if ((l += nulllen) >= max_row_len)
					huge_row();

				memcpy(s, nullstr, nulllen);
				s += nulllen;
			}

			continue;
		}

		for (p = row[i]; *p != '\0'; p++) {
			if (l++ >= max_row_len)
				huge_row();

			*s++ = *p;
		}
	}

	return(make_string(buf, l));
}

///////////////////////////////////////////////////////////////////////
//
// SOURCE: src/release.c
//
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////

// Function `release_server' releases current server from sending
// results, clears any result set for the current server and
// finally pops out the server from the server stack. Function
// returns zero if there is no server to release else returns non
// zero.

static int
release_server(FILE *dbg)
{
	if (cur_server < 0)
		return(0);

	if (dbg != NULL) {
		debug_level();
		fprintf(dbg, "%sRELEASE SERVER %d\n",
			comment_string, n_server);
		fflush(dbg);
	}

	clear_results("release_server");
	field_count[cur_server] = 0;
	if (data_index[cur_server] != NULL)
		(void)set_data_index(NULL, 0);

	affected[cur_server] = 0;
	busy[cur_server] = 0;
	pop_server();
	return(1);
}

///////////////////////////////////////////////////////////////////////
//
// SOURCE: src/data.c
//
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////

#define number_index(x) assoc_lookup((x), \
	tmp_number((AWKNUM) i), FALSE)
#define string_index(x) assoc_lookup((x), \
	tmp_string(p->idx, p->len), FALSE)
#define column_value(col) ((col) == NULL ? \
	make_string(nullstr, nulllen) : \
	make_string((col), strlen((col))))

///////////////////////////////////////////////////////////////////////

// Function `get_data' requests next data row from the current
// server's result set. The first argument is the argument
// passed to the original function (`data', `first', `last') and
// actually is the arguments' tree of the original function.
// These functions may be called with none, one or two
// arguments. Both of these arguments are arrays. The first
// array will be used as data transfer vehicle, while the
// second array (if given) will denote the null valued
// columns. Null valued columns will produce a non-zero
// element in that array with the same index with the
// corresponding column, while normal valued columns will
// not produce elements in that array. The second argument
// is the arguments' count of the original function's arguments,
// whireas the third argument is a simple string which shows the
// name of the caller (passed for debugging reasons), and the fourth
// argument must be zero, except from calls for the last row of data;
// in that case the function must be called with value of 1 for that
// argument. Function `get_data' returns zero if there are no more
// results to return, or a positive number meaned to be the number of
// columns of the row returned.

static int
get_data(NODE *tree, int argc, char *func, int last_row)
{
	NODE *data = NULL;
	NODE *null = NULL;
	NODE **aptr;
	MYSQL_ROW row;
	int err;
	int i;
	int j;
	DATA_INDEX *p;
	int found = 0;

	// The following two lines are common for all three possible
	// callers: `spawk_data', `spawk_first' and `spawk_last'
	// functions, so we put them here, because `get_data' is the
	// very first internal function called from these functions.
	SPAWK_INIT();
	separator();

	if ((argc > 2) && do_lint)
		lintwarn("%s: cannot be called with "
			"more than two arguments", func);

	switch (argc) {
	case 2:
		null = get_array_argument(tree, 1, FALSE);
	case 1:
		data = get_array_argument(tree, 0, FALSE);
	case 0:
		break;
	default:
		fatal("%s: invalid # of arguments", func);
	}

	if (cur_server < 0)
		fatal("%s: no active server", func);

	if (!busy[cur_server])
		fatal("%s: server drained (server %d)", func, n_server);

	if (data != NULL)
		assoc_clear(data);

	if (null != NULL)
		assoc_clear(null);

NEXT_ROW:
	if ((row = mysql_fetch_row(res_set[cur_server])) == NULL) {
		if (err = mysql_errno(conn[cur_server]))
			fatal("%s: SQL mysql_fetch_row error "
				"(%d -- %s)", func, err,
				mysql_error(conn[cur_server]));

		clear_results("get_data");
		// We keep the current server's fields' count for
		// not loosing the value and also set the results'
		// set pointer to null for not `release_server' to
		// try to free the results' buffer.
		i = field_count[cur_server];
		res_set[cur_server] = NULL;
		(void)release_server(NULL);
		return((last_row && found) ? i : 0);
	}

	if ((i = mysql_num_fields(res_set[cur_server])) !=
		field_count[cur_server])
		fatal("%s: invalid # of result fields (%d <> %d)",
			func, i, field_count[cur_server]);

	found = 1;
	if (data == NULL)
		goto RETURN;

	// Each row read is put in the data and null array. So, if
	// we are seeking the last row, we have to clear the arrays
	// to get the new row's data. That's the reason of seting
	// `last_row' flag to 2 after first row get.
	if (last_row > 1) {
		if (data != NULL)
			assoc_clear(data);

		if (null != NULL)
			assoc_clear(null);
	}

	aptr = assoc_lookup(data, (data_index[cur_server] == NULL ?
		tmp_number((AWKNUM) 0) : tmp_string(empty_string, 0)),
		FALSE);
	*aptr = construct_row(row);
	for (p = data_index[cur_server], i = 1, j = 0;
		i <= field_count[cur_server]; i++, j++) {
		if (data_index[cur_server] == NULL) {
			aptr = number_index(data);
			*aptr = column_value(row[j]);
		}
		else if (p != NULL) {
			aptr = string_index(data);
			*aptr = column_value(row[j]);
		}
		else {
			warning("%s: result out of index", func);
			break;
		}

		if ((row[j] == NULL) && (null != NULL)) {
			if (data_index[cur_server] == NULL) {
				aptr = number_index(null);
				*aptr = make_number((AWKNUM) 1);
			}
			else if (p != NULL) {
				aptr = string_index(null);
				*aptr = make_number((AWKNUM) 1);
			}
		}

		if (p != NULL)
			p = p->next;
	}

RETURN:
	switch (last_row) {
	case 1:
		last_row = 2;
	case 2:
		goto NEXT_ROW;
	}

	return(field_count[cur_server]);
}

///////////////////////////////////////////////////////////////////////


// do_spawk_data --- request next data row

// USAGE
// spawk_data([data[, null]])

// RETURNS
// Number of columns in row returned, zero on end-of-results (number).

static NODE *
do_spawk_data(NODE *tree)
{
	set_value(tmp_number((AWKNUM) get_data(tree,
		get_curfunc_arg_count(), "data", 0)));
	return(tmp_number((AWKNUM) 0));
}

///////////////////////////////////////////////////////////////////////

// do_spawk_first --- request next data row and free results

// USAGE
// spawk_first([data[, null]])

// RETURNS
// Number of columns in row returned, zero on end-of-results (number).

static NODE *
do_spawk_first(NODE *tree)
{
	int n;

	if ((n = get_data(tree, get_curfunc_arg_count(),
		"first", 0)) > 0)
		(void)release_server(NULL);

	set_value(tmp_number((AWKNUM) n));
	return(tmp_number((AWKNUM) 0));
}

///////////////////////////////////////////////////////////////////////

// do_spawk_last --- request last data row and free results

// USAGE
// spawk_last([data[, null]])

// RETURNS
// Number of columns in row returned, zero on end-of-results (number).

static NODE *
do_spawk_last(NODE *tree)
{
	set_value(tmp_number((AWKNUM) get_data(tree,
		get_curfunc_arg_count(), "last", 1)));
	return(tmp_number((AWKNUM) 0));
}

///////////////////////////////////////////////////////////////////////
//
// SOURCE: src/clear.c
//
///////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////
//
// SOURCE: src.stable/affected.c
//
///////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////
//
// SOURCE: src.stable/string.c
//
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////

// do_spawk_string --- escape certain characters (quotes, etc)

// USAGE
// spawk_string([string])

// RETURNS
// The string escaped (string).

static NODE *
do_spawk_string(NODE *tree)
{
	int argc;
	NODE *arg;
	char *new;
	char *p;
	char *q;

	if (((argc = get_curfunc_arg_count()) != 1) && do_lint)
		lintwarn("string: must be called with "
			"one argument");

	switch (argc) {
	case 1:
		break;
	default:
		fatal("string: invalid # of arguments");
	}

	force_string(arg = get_scalar_argument(tree, 0, FALSE));
	if ((new = malloc(arg->stlen * 2)) == NULL)
		fatal("out of memory allocating escaped "
			"string buffer");

	for (p = arg->stptr, q = new; *p != '\0'; p++) {
		switch (*p) {
		case '\'':
		case '\"':
		case '\\':
			*q++ = '\\';
		default:
			*q++ = *p;
		}
	}

	*q = '\0';
	free_temp(arg);
	set_value(tmp_string(new, q - new));
	free(new);
	return(tmp_number((AWKNUM) 0));
}

///////////////////////////////////////////////////////////////////////
//
// SOURCE: src.stable/error.c
//
///////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////
//
// SOURCE: src.stable/dlload.c
//
///////////////////////////////////////////////////////////////////////

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
