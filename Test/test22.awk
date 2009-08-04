# The following program selects all nullable columns (columns not
# not defined with the "NOT NULL" column attribute) and store in
# the array `null_index' the indices (constraints) in which these
# columns are involved. At the end all of the indices stored in
# `null_index' array are printed along with all of the columns
# involved in each index; columns caused the printing (nullable
# are marked with an asterisk (*)).

BEGIN {
	OFS = "."
	extension("libspawk.so", "dlload")
	spawk_select("SELECT TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME " \
		"FROM INFORMATION_SCHEMA.COLUMNS " \
		"WHERE IS_NULLABLE = 'YES'")
	while (spawk_data(column))
		proc_column()

	for (i in null_index)
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
		null_column[column[0]]
		null_index[data[0]]
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
		print "\t" (data[0] in null_column ? "*" : " ") data[0]
}
