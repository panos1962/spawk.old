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
