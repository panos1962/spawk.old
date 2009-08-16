BEGIN {
	# First of all, load the SPAWK module.
	extension("libspawk.so", "dlload")

	# If given the `drop' variable, then drop the `spawk'
	# test database if it's exist.
	if (drop)
		drop_database()

	# If given the `create' variable, then create the
	# `spawk' test database. To define `create' variable
	# run awk as follows:
	#
	#	awk -v create="spawk.dd" -f test11.awk
	#
	# If defined, `create' variable must be the name of a MySQL
	# script. This script will create the `spawk' test database
	# if it's not exist. The script may contain multiple MySQL
	# queries, but these queries must be separated with ";"
	# lines, or the ";" character must be the last character
	# in the line.
	if (create)
		create_database()

	# `SPAWKINFO' array is an array to be filled with values
	# set in MySQL configuration files (`/etc/my.cnf' and
	# `~/.my.cnf'). We can, of course, set values externally.
	# The "database" indexed element is the default database
	# to be used with new database connections.
	#
	# We can, alternatively, use `/etc/spawkrc' and `~/.spawkrc'
	# files to run SQL commnands to initialize new servers.
	SPAWKINFO["database"] = "spawk"

	# We'll need the current date repeatedly later, so put it
	# in the `now' variable.
	now = strftime("%Y%m%d")

	# Now it's time to visit users. We use the password file
	# `/etc/passwd' to access all the users of the machine
	# on hand.
	read_passwd()

	# After processing all the users in the password file, we
	# mark as deleted the users present in the database but not
	# processed (not found in the password file). Mark deleted
	# means setting the `datedel' column to the current date
	# instead of null which means active user.
	mark_deleted()

	# Now it's time to walk the file tree and produce records
	# in the `file' table.
	scan_files()
}

function drop_database(				err) {
	if (monitor)
		print "Deleting test database" >monitor

	spawk_update("DROP DATABASE IF EXISTS `spawk`", 2)

	# I think this is not really needed, because data definition
	# queries are commited automatically, but doesn't bother to
	# re-commit. Note the second argument (2); if the query fails,
	# then an error message is printed and the program (awk)
	# exits with the given exit code (2).
	spawk_update("COMMIT WORK", 2)
}

# As said earlier, if defined the `create' variable must be the
# name of a MySQL script which drops and reconstructs an empty
# (fresh) spawk test database.

function create_database(				err) {
	if (monitor)
		print "Creating test database" >monitor

	while ((err = (getline <create)) > 0) {
		if ($0 ~ /;[ \t]*$/) {
			sub(";[ \\t]*$", "")
			spawk_update($0, 2)
		}
		else
			spawk_query($0 "\n")
	}

	close(create)
	if (err) {
		print create ": " ERRNO >"/dev/stderr"
		exit(2)
	}

	# I think this is not really needed, because data definition
	# queries are commited automatically, but doesn't bother to
	# re-commit. Note the second argument (2); if the query fails,
	# then an error message is printed and the program (awk)
	# exits with the given exit code (2).
	spawk_update("COMMIT WORK", 2)
}

# We use `/etc/passwd' file to seek all the users. Every user has
# a line in the password file. Each line consists of ":" separated
# fields (uid, gid, login, info etc). The UNIX user-id (thrid field)
# is used as the primary key of the `user' table. Users with `nologin'
# shell are considered as inactive and are not processed at all. User
# `root' (zero user-id) is also skipped. For the rest of the users
# the data are checked and either the user is inserted in the database,
# or the user is updated (seting the `datemod' field too).

function read_passwd(				ufile, user) {
	ufile = "/etc/passwd"
	if (monitor)
		print "Reading \"" ufile "\" file" >monitor

	FS = ":"
	while (getline <ufile) {
		if (($3 += 0) <= 0)
			continue

		if ($NF ~ "/nologin$")
			continue
 
		# Insert or update the user in the `user' table.
		if (proc_user($3, $1, $5, $6))
			spawk_update("COMMIT WORK", 2)
	}

	close(ufile)
}

# Function `proc_user' will process each user and must return
# zero on success, or non-zero on failure.

function proc_user(uid, login, name, home,	data, null) {
	if (monitor)
		print "Processing user-id " uid >monitor

	# We maintain the array `processed' to keep track of the
	# users processed. The rest of the users will be marked
	# as deleted at the end of the process.
	processed[userid[login] = uid]

	# Select essential data from the `user' table to see if
	# the user exists in the `user' table, or if update is
	# needed. The second argument of the `spawk_select' function
	# (if exists) is a comma separated string of indices to
	# be used as indices in the returned data array later.
	# If not given, then the returned data array will be indexed
	# with numbers (0 for the whole record, 1 for the first column,
	# 2 for the second column etc).
	spawk_select("SELECT `uid`, `login`, `name`, `home`, " \
		"`datedel` FROM `user` WHERE `uid` = " uid, \
		"uid,login,name,home,datedel")

	# `spawk_first' function gets the first row of data from the
	# current server (connection) and eats any subsequent results.
	# It's first argument is an array to hold the returned data
	# while the second argument (if given) is another array to
	# hole null valued columns only. `spawk_first' returns zero
	# if no results returned, so this will mean that user doesn't
	# exist and must be added in the `user` table.
	if (!spawk_first(data, null))
		return(add_user(uid, login, name, home))

	# The user exists in the database and it's time to check
	# essential user's data. If found differences, the user
	# will be updated and the column `datemod' (modification
	# date) will be se to the current date (`now'). Note the
	# `datedel' column check; this column must be null for
	# active users; if set then means that the user was deleted
	# (not found in the password file) sometimes in the past.
	# If that's the case, then the user must be unmarked now,
	# so the `datedel' column must be set to null.
	if ((user["login"] == login) && (user["name"] == name) && \
		(user["home"] == home) && (!("datedel" in null)))
		return(0)

	# Differences found, or the user was marked deleted. Update
	# the existing `user' record with the new values and unmark
	# deleted (`datedel' set to null).
	if (monitor)
		print "Updateing user-id " uid >monitor

	# This update is not fatal, so there is no second argument
	# to `spawk_update` function (remember, second argument means
	# exit code in case of failure). In that case, if the query
	# fails, we print a message and execute a (unneeded) rollback.
	# The rollback is unneeded beacuse nothing was uncommited.
	# As you may already noticed, we use the return value of
	# `spawk_update' to dense the code later. As a matter if fact
	# `spawk_update' returns zero on success and non-zero on
	# failure.
	if (spawk_update("UPDATE `user` SET `uid` = " uid \
		", `login` = '" login "', `name` = '" \
		spawk_string(name) "', `home` = '" spawk_string(home) \
		"', `datemod` = " now ", `datedel` = NULL " \
		"WHERE `uid` = " uid)) {
		print uid ": user update failed (" \
			spawk_error() ")" >"/dev/stderr"
		return(!spawk_update("ROLLBACK WORK", 2))
	}

	return(1)
}

function add_user(uid, login, name, home) {
	if (spawk_update("INSERT INTO `user` (`uid`, `login`, `name`, \
		`home`, `dateins`, `datemod`) " \
		"VALUES (" uid ",'" login "','" spawk_string(name) \
		"','" spawk_string(home) "'," now "," now")")) {
		print uid ": user insert failed (" \
			spawk_error() ")" >"/dev/stderr"
		return(0)
	}

	return(1)
}

function mark_deleted(						user) {
	spawk_select("SELECT `uid` FROM `user`")
	while (spawk_data(user)) {
		if ((user[1] += 0) in processed)
			continue

		if (!spawk_update("UPDATE `user` SET `datedel` = " \
			now " WHERE `uid` = " user[1])) {
			spawk_update("COMMIT WORK", 2)
			continue
		}

		print user[1] ": mark deleted failed (" \
			spawk_error() ")" >"/dev/stderr"
		spawk_update("ROLLBACK WORK")
	}
}

function scan_files(				ftype, cmd, stat, \
	cnt, dot, added)
{
	FS = " "
	ftype["-"] = "file"
	ftype["d"] = "directory"
	ftype["l"] = "link"
	ftype["b"] = "device"
	ftype["c"] = "device"
	ftype["p"] = "pipe"
	ftype["s"] = "socket"
	
	if (monitor)
		print "Deleting `file' entries for " now >monitor

	if (spawk_update("DELETE FROM `file` WHERE `date` = " now)) {
		print "delete files for `" now "' failed (" \
			spawk_error() ")" >"/dev/stderr"
		spawk_update("ROLLBACK WORK")
		exit(2)
	}

	cmd = "find / -ls 2>/dev/null"
	if (monitor) {
		if (!fdot)
			fdot = 1000

		print "Inserting `file' entries for " now >monitor
		print "\t(a dot will be printed for each " fdot \
			" files processed)" >monitor
	}

	while ((stat = (cmd |& getline)) > 0) {
		if (monitor && (++cnt > 1) && ((cnt % fdot) == 1))
			printf (dot = ".") >monitor
			
		if (!($5 in userid))
			continue

		if (!(($3 = substr($3, 1, 1)) in ftype)) {
			print $0 ": invalid file type" >"/dev/stderr"
			continue
		}

		if ($11 ~ /\\/)
			gsub("\\\\", "\\\\", $11)

		if (spawk_update("INSERT INTO `file` (`inode`, " \
			"`date`, `type`, `user`, `path`, `size`) " \
			"VALUES (" $1 ", " now ", '" ftype[$3] "', " \
			userid[$5] ", '" $11 "'," ($3 == "-" ? \
			$7 : "NULL") ")")) {
			print $0 ": insert file failed" >"/dev/stderr"
			continue
		}

		spawk_update("COMMIT WORK", 2)
		added++
	}

	if (monitor) {
		if (dot)
			print "" >monitor

		print added " files added for " now >monitor
	}

	if (stat) {
		print cmd ": command failed" >"/dev/stderr"
		exit(2)
	}
}
