BEGIN {
	SPAWKINFO["database"] = "INFORMATION_SCHEMA"
	SPAWKINFO["null"] = "NIL"
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

	spawk_select("SELECT TABLE_SCHEMA, TABLE_NAME FROM TABLES")
	while (spawk_data(table))
		proc_table()

	exit(0)
}

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
