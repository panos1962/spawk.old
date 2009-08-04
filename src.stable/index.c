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
