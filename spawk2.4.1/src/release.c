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
