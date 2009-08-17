BEGIN {
    SPAWKINFO["program"] = "fscan"
    SPAWKINFO["database"] = "ufstats"
    extension("libspawk.so", "dlload")
    set_interval()
    cmd = "find / -type f -ls"
    user_file = "/etc/passwd"
    while (fscan())
        system("sleep " interval)
}

# Function "set_interval" is an initialization function to set
# the time interval between consequent scans. The time interval
# is given in seconds (default 1 hour) and will be measured after
# each scan end using UNIX "sleep" command.

function set_interval() {
    if (!interval)
        interval = 3600
    else if (interval !~ /^[ \t]*[0-9][0-9]*[ \t]*$/)
        exit(errmsg(interval ": invalid time interval"))
    else
        interval += 0

    # If the (external) variable "monitor" is set, then
    # various messages will be printed to the standard
    # output.

    if (monitor)
        print interval
}

# Function "fscan" is the core function which does all the job. It
# starts by reading the password file and updateing the "cuser"
# table. Then it inserts an "open" "fscan" record for the current
# scan; an "fscan" record considered as "open" while its "fs_end"
# column is NULL valued. The "fs_end" column will be filled with
# the scan's end date/time when the scan ends. After inserting
# the "fscan" record, the function proceeds to the files' counting
# process reading the "find" UNIX command output. For each file
# found it increases the "count" array for the owner of the file.
# After scanning the file system ("find" end) the function proceeds
# in "fcount" inserts for each user. Then it commits the transactions
# and returns.

function fscan(         start, end, count, stat, login, err) {
    scan_users(start = strftime("%Y%m%d%H%M%S", systime()))
    if (monitor) {
        printf start "-"
        fflush("/dev/stdout")
    }

    # Insert a "fscan" record for the current date/time. The
    # "end" column value will be filled when that scan ends.
    # If a "fscan" record with the same "start" column value
    # already exists, then the time interval is too small
    # and the command respawns too rapidly; ignore that.

    if ((err = spawk_update("INSERT INTO fscan (fs_start) " \
        "VALUES (" start ")")) == 1062) {
        errmsg(cmd ": command spawning too fast (" start ")")
        return(1)
    }

    if (err)
        exit(errmsg("insert fscan failed (" spawk_error() ")"))

    # Now run the "find" command and read the results. Given
    # the "-ls" option, the user is at the 5th field. Accumulate
    # files' counts in array "count".

    while ((stat = (cmd |& getline)) > 0)
        count[$5]++

    # Check if the "find" command terminated normally. If not,
    # then exit with non zero exit status.

    if (stat < 0)
        exit(errmsg(cmd ": " ERRNO))

    close(cmd)
    end = strftime("%Y%m%d%H%M%S", systime())
    if (monitor) {
        print end
        fflush("/dev/stdout")
    }

    # Now insert the files' counts for that scan in the
    # database. Some "find" results may contain numbers
    # as user login names; those are UNIX user ids for
    # user not found in the password file and probably
    # there are no such entries in out "cuser" table
    # also; insert new users using those numeric ids
    # as login names in order not to loose those counts.

    for (login in count) {
        # If the login not found in the "cuser" table,
        # insert it as "unknown user", setting the
        # "cu_delete" column value to the date/time
        # of the current scan.

        spawk_select("SELECT cu_login FROM cuser " \
            "WHERE cu_login = '" login "'")
        if (!spawk_first())
            insert_user(login, "** Unknown User **", start)

        spawk_update("INSERT INTO fcount " \
            "(fc_scan, fc_user, fc_files) VALUES (" \
            start ",'" login "'," count[login] ")", 2)
    }

    # Now it's time to "close" the scan, setting the "fs_end"
    # column value to the date/time when the current scan ended.
    # After "closing" the scan, commit transactions.

    spawk_update("UPDATE fscan SET fs_end = " end \
        " WHERE fs_start = " start, 2)
    spawk_update("COMMIT WORK", 2)
    return(1)
}

# Function "scan_user" scans the "/etc/passwd" file and inserts
# the users in the database. Before doing so, we first set the
# "cu_delete" value to the current scan's date/time. During the
# password file scan we reset the "cu_delete" to NULL; that way
# the users not found in the password file for the current scan
# will be left with the "cu_delete" column filled.

function scan_users(dt,         stat, fs1) {
    spawk_update("UPDATE cuser SET cu_delete = " dt, 2)
    # Temporarily, we set the FS to ":" in order to
    # read the password file the easy way (awk).

    fs1 = FS
    FS = ":"
    while ((stat = (getline <user_file)) > 0) {
        if (insert_user($1, $5, dt))
            spawk_update("UPDATE cuser " \
                "SET cu_delete = NULL " \
                "WHERE cu_login = '" login "'", 2)
        else
            exit(errmsg($1 ": cuser insert failed: " \
                spawk_error()))
    }

    if (stat < 0)
        exit(errmsg(user_file ": " ERRNO))

    # Close the password file and reset the FS separator
    # to its default value in order to read the "find"
    # output.

    close(user_file)
    FS = fs1
}

# Function "insert_user" is used to add "cuser" records in the
# database. The "cuser" column values ("cu_login", "cu_info" and
# "cu_delete") are passed as function's arguments.

function insert_user(login, info, dt,       err) {
    # If the user already exists in the database, then
    # it's ok, but return non zero, else return zero.
    # If the user fails to insert for any other reason,
    # then that is a fatal error and exit non zero.

    if ((err = spawk_update("INSERT INTO cuser " \
        "(cu_login, cu_info, cu_delete) " \
        "VALUES ('" login "','" spawk_string(info) \
        "'," dt ")")) == 1062)
        return(1)

    if (err == 0)
        return(1)

    exit(errmsg(login ": insert user failed (info = " info \
        ", delete = " dt "): " spawk_error()))
}

# Function "errmsg" is a tiny utility function to print error
# messages. For code density reasons it returns non zero.

function errmsg(msg) {
    print SPAWKINFO["program"] ": " msg >"/dev/stderr"
    return(2)
}
