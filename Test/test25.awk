# We extend previous example even more, allowing the schema to be
# specified except from data types. For this purpose we use the
# array `schema' just like we used array `type' for the data types
# of interest.

BEGIN {
	OFS = "."
	type["int"]
	type["char"]
	type["varchar"]
	type["enum"]
	schema["mysql"]
	schema["INFORMATION_SCHEMA"]
	extension("/usr/lib/libspawk.so", "dlload")
	spawk_select("SELECT TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME, " \
		"DATA_TYPE FROM INFORMATION_SCHEMA.COLUMNS")
	while (spawk_data(column)) {
		if ((column[1] in schema) && (column[4] in type))
			proc_column()
	}

	for (i in asked_index)
		print_index(i)

	exit(0)
}

function proc_column(				data) {
	spawk_select("SELECT CONSTRAINT_SCHEMA, CONSTRAINT_NAME " \
		"FROM INFORMATION_SCHEMA.KEY_COLUMN_USAGE " \
		"WHERE TABLE_SCHEMA = '" column[1] \
		"' AND TABLE_NAME = '" column[2] \
		"' AND COLUMN_NAME = '" column[3] \
		"' ORDER BY CONSTRAINT_SCHEMA, CONSTRAINT_NAME")
	while (spawk_data(data)) {
		asked_column[column[1] OFS column[2] OFS column[3]] = \
			column[4]
		asked_index[data[0]]
	}
}

function print_index(idx,			a, data) {
	print idx
	split(idx, a, OFS)
	spawk_select("SELECT TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME " \
		"FROM INFORMATION_SCHEMA.KEY_COLUMN_USAGE " \
		"WHERE CONSTRAINT_SCHEMA  = '" a[1] \
		"' AND CONSTRAINT_NAME = '" a[2] \
		"' ORDER BY ORDINAL_POSITION")
	while (spawk_data(data)) {
		printf "\t" (data[0] in asked_column ? "*" : " ") data[0]
		if (data[0] in asked_column)
			printf " (" asked_column[data[0]] ")"

		print ""
	}
}
