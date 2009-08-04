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
