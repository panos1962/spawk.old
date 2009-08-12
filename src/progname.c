///////////////////////////////////////////////////////////////////////

// Function `set_progname' seeks `SPAWKINFO' array for a "program"
// indexed element. If found such an element, then it's assumed as
// the program's name. If not, then the current program name is
// inserted in `SPAWKINFO' indexed "program".
//
// If passed a non zero argument, then the SPAWK version number
// will be filled in SPAWKINFO["version"].
//
// After calling `set_progname' it's guaranteed that the program's
// name and `SPAWKINFO's "program" indexed element coincide.

#define version_string "version"
#define VERSION_LEN 7
#define SPAWKVER ("SPAWK Version " SPAWK_VERSION \
"\nCopyright (C) 2009, Panos I. Papadopoulos")

static void
set_progname(int version)
{
	NODE *arg;
	NODE **aptr;

	if (version) {
		// If there is a `SPAWKINFO' element indexed "version"
		// already, then that's considered and error. This
		// element will be filled with the SPAWK module version.
		if ((arg = in_array(spawkinfo,
			tmp_string(version_string, 
			VERSION_LEN))) != NULL)
			fatal("hacked SPAWK version (%s)", arg->stptr);

		aptr = assoc_lookup(spawkinfo,
			tmp_string(version_string, VERSION_LEN), FALSE);
		*aptr = make_string(SPAWKVER, strlen(SPAWKVER));
	}

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
		aptr = assoc_lookup(spawkinfo,
			tmp_string(program_string, PROGRAM_LEN), FALSE);
		*aptr = make_string((char *)myname, strlen(myname));
	}
}
