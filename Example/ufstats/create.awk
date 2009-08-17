# The following program reads a DDL script to create teh "ufstats"
# database. If the database exists, then the program exits with
# non zero exit status; if we want to drop the existing database
# and then recreate the database we must define the (external)
# variable drop, e.g.
#
#    awk -v drop=yes -f create.awk create.sch

BEGIN {
    SPAWKINFO["program"] = "create"
    extension("libspawk.so", "dlload")
    if (drop)
        spawk_update("DROP DATABASE IF EXISTS ufstats", 2)

    spawk_select("SHOW DATABASES LIKE 'ufstats'")
    if (spawk_first())
        exit(errmsg("ufstats: database exists"))
}

# Skip SQL comments.

/^--/ {
    next
}

# Skip empty lines.

/^[ \t]*$/ {
    next
}

# If a line ends with a ";" character, then execute the current
# query. Current query is the commands already sent to the server.

/;[ \t]*$/ {
    sub("[ \\t]*;[ \\t]*$", "")
    if (spawk_update($0))
        spawk_update("ROLLBACK WORK", 2)
    else
        spawk_update("COMMIT WORK", 2)

    next
}
        
# Normal line. Send that line to the server to be executed later
# when the ";" character shows up.

{
    spawk_query($0)
}

# Function "errmsg" is a utility function to print error messages.
# For code density reasons, the function returns non zero.

function errmsg(msg) {
    print SPAWKINFO["program"] ": " msg >"/dev/stderr"
    return(2)
}
