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
