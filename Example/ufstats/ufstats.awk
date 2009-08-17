# The following program processes "fcount" data and produces
# per user file count statistics for a given time period.
# If given the "from" (external) variable, then the data
# from the given date/time are processed. You can also up
# bound the time interval by setting the "to" (external)
# variable. If none of those variables are set, then all
# the "fcount" data will be processed.
#
# The output format is one line for each user processed and
# contains the following pipe ("|") separated fields:
#
#    minimum date/time processed
#    maximum date/time processed
#    number of scans processed
#    user login name
#    user info
#    average number of files owned by the user

BEGIN {
    OFS = "|"
    OFMT = "%.0f"    
    SPAWKINFO["program"] = "ufstats"
    SPAWKINFO["database"] = "ufstats"
    extension("libspawk.so", "dlload")
    spawk_query("SELECT DATE_FORMAT(fs_start, '%Y%m%d%H%i%S') " \
        "FROM fscan WHERE 1 = 1")

    if (from)
        spawk_query(" AND fs_start >= " from)

    if (to)
        spawk_query(" AND fs_start < " to)

    if (spawk_update())
        exit(errmsg("invalid from/to"))

    spawk_results()
    min_scan = 99999999999999
    max_scan = 00000000000000
    while (spawk_data(fscan)) {
        if (fscan[1] < min_scan)
            min_scan = fscan[1]

        if (fscan[1] > max_scan)
            max_scan = fscan[1]

        times++
        spawk_select("SELECT fc_user, fc_files " \
            "FROM fcount WHERE fc_scan = " fscan[1])
        while (spawk_data(fcount))
            total[fcount[1]] += fcount[2]
    }

    if (times) {
        for (user in total)
            print min_scan, max_scan, times, user, \
                user_name(user), total[user] / times
    }
    else
        errmsg("no data")

    exit(0)
}

function user_name(login,            user) {
    spawk_select("SELECT cu_info FROM cuser " \
        "WHERE cu_login = '" login "'")
    spawk_first(user)
    return(user[1])
}

function errmsg(msg) {
    print SPAWKINFO["program"] ": " msg >"/dev/stderr"
    return(2)
}
