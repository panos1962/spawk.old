BEGIN {
        SPAWKINFO["database"] = "chess"
        SPAWKINFO["program"] = "check"
        extension("libspawk.so", "dlload")

        # If the `all' (external) variable is set, then run checks
        # for all the tournaments in the database, else tournaments'
        # keys (or descriptions) of the the tournaments to be checked
        # will be read from the input.

        if (all)
                exit(check_all_tournaments())
}

# If the input line is just a number, then that number is assumed
# to be plain `tr_key', else the current input line is assumed to
# be a regular expression (in MySQL LIKE's terms) to match tournaments'
# descriptions.

{
        if ($0 ~ /^[ \t]*[0-9][0-9]*[ \t]*$/)
                check_tournament($0 + 0)
        else {
                spawk_select("SELECT tr_key FROM tournament " \
                        "WHERE tr_desc LIKE '" spawk_string($0) \
                        "' ORDER BY tr_key")
                while (spawk_data(tournament))
                        check_tournament(tournament[1])
        }
}

# Function `check_all_tournaments' selects all tournaments in the
# database and check each tournament for data validity.

function check_all_tournaments(                 data, errs) {
        spawk_select("SELECT tr_key FROM tournament " \
                "ORDER BY tr_key;")
        while (spawk_data(data))
                errs += check_tournament(data[1])

        return(errs + 0)
}

# Function `check_tournament' accepts a tournament's key as its
# first parameter and checks the validity of the data in the
# specified tournament.

function check_tournament(tr_key,                       data) {
        spawk_select("SELECT tr_desc, tr_begin, tr_end, tr_other " \
                "FROM tournament WHERE tr_key = " tr_key)
        if (!spawk_first(data))
                return(!errmsg(tr_key ": tournament not found!"))

        monmsg("Checking tournament \"" data[1] "\" [" tr_key "]...")

        # Here comes the actual check code. For now we don't
        # actually check the given tournament, but only print
        # the tournament's data to the standard output.

        print tr_key
        monmsg()
        return(0)
}

# The `monmsg' function is used for monitoring the check process.
# If given the `monitor' (external) variable, then that value is
# assumed to be a file's name and the monitor messages will be
# printed to that file; usually that file is the terminal's screen
# (`/dev/tty').

function monmsg(msg) {
	if (monitor == "")
		return

	printf (msg ? msg : "\n") >monitor
	fflush(monitor)
}

# Function `errmsg' is a utility function used for error message
# print. Return a non-zero number for code density reasons.

function errmsg(msg) {
        print SPAWKINFO["program"] ": " msg >"/dev/stderr"
        return(1)
}
