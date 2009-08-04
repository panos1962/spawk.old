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
	SPAWKINFO["database"] = "INFORMATION_SCHEMA"
	SPAWKINFO["null"] = "NIL"
SPAWKINFO["ssl-key"] = "key"
SPAWKINFO["ssl-cert"] = "cert"
SPAWKINFO["ssl-ca"] = "ca"
SPAWKINFO["ssl-capath"] = "capath"
SPAWKINFO["ssl-cipher"] = "cipher"

	# The following lines check the empty or "white" queries
	# SPAWK handling.
	if (spawk_update("  \t\t\n\n\r\r\f\f  ") || spawk_update("")) {
		print SPAWKINFO["program"] ": bad empty or \"white\" " \
			"query handling" >"/dev/stderr"
		exit(2)
	}

	spawk_select("SELECT TABLE_NAME FROM TABLES " \
		"ORDER BY TABLE_NAME", "table")
	while (spawk_data(table)) {
		if (!first_table)
			first_table = table["table"]

		last_table = table[""]
	}

	if (first_table == last_table) {
		print SPAWKINFO["program"] \
			": first/last `TABLE_NAME'" >"/dev/stderr"
		exit(2)
	}

	spawk_select("SELECT TABLE_NAME FROM TABLES ORDER BY TABLE_NAME")
	if ((spawk_first(table) != 1) || (table[1] != first_table)) {
		print SPAWKINFO["program"] \
			": first `TABLE_NAME' failed: >" table[1] \
			"<>" first_table "<" >"/dev/stderr"
		exit(2)
	}

	spawk_select("SELECT TABLE_NAME FROM TABLES ORDER BY TABLE_NAME")
	if ((spawk_last(table) != 1) || (table[1] != last_table)) {
		print SPAWKINFO["program"] \
			": last `TABLE_NAME' failed: >" table[1] \
			"<>" last_table "<" >"/dev/stderr"
		exit(2)
	}

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
	spawk_select("SELECT COLUMN_NAME, DATA_TYPE, " \
		"NUMERIC_PRECISION, NUMERIC_SCALE FROM COLUMNS " \
		"WHERE TABLE_SCHEMA = '" table[1] \
		"' AND TABLE_NAME = '" table[2] "'", \
		"column,type,precision,scale")
	while (spawk_data(data))
		print "\t" data["column"], data["type"], \
			data["precision"], data["scale"]
}
