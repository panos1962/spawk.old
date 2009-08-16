BEGIN {
	# Load the SPAWK module.
	extension("libspawk.so", "dlload")

	# `SPAWKINFO' array is an array to be filled with values
	# set in MySQL configuration files (`/etc/my.cnf' and
	# `~/.my.cnf'). We can, of course, set values externally.
	# The "database" indexed element is the default database
	# to be used with new database connections.
	#
	# We can, alternatively, use `/etc/spawkrc' and `~/.spawkrc'
	# files to run SQL commnands to initialize new servers.
	SPAWKINFO["database"] = "information_schema"

	# Run query with `spawk_select' and process the results
	# using `spawk_data' repeatedly. Rows are returned in
	# `table' global array.
	spawk_select("SELECT TABLE_SCHEMA, TABLE_NAME FROM TABLES")
	while (spawk_data(table))
		proc_table()

	exit(0)
}

# Function `proc_table' is called for each table selected from the
# main query. After printing table schema and name initiate a new
# query to select tables's columns. Use `spawk_data' repeatedly to
# get and print the column names. Those queries will not be handled
# by the server processing the main query, but a new server will
# be created to handle them (depth two).

function proc_table(				data) {
	print "Table: " table[1] "." table[2]
	spawk_select("SELECT COLUMN_NAME FROM COLUMNS " \
		"WHERE TABLE_SCHEMA = '" table[1] \
		"' AND TABLE_NAME = '" table[2] "'")
	while (spawk_data(data))
		print "\t" data[0]
}
