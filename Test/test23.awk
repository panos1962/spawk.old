# This program is like the previous one, but instead of seeking the
# nullable columns of the database, it seeks columns of type 'char'
# and print the indices involved among with all their columns.

BEGIN {
	OFS = "."
	extension("/usr/lib/libspawk.so", "dlload")
	spawk_select("SELECT TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME " \
		"FROM INFORMATION_SCHEMA.COLUMNS " \
		"WHERE DATA_TYPE = 'char'")
	while (spawk_data(column))
		proc_column()

	for (i in char_index)
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
		char_column[column[0]]
		char_index[data[0]]
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
	while (spawk_data(data))
		print "\t" (data[0] in char_column ? "*" : " ") data[0]
}
