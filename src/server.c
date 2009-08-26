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
			"not null data index pointer", n_server);
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
