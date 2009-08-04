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
