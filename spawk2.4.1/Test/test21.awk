# The following script selects and prints all indices (constraints)
# in the database where at least one nullable column is involved.
# Nullable columns are those columns that can accept null values,
# thus columns not defined with the "NOT NULL" column attribute.
# The logic is: select all nullable columns and for each such column
# print the column and all of the constraints where this column is
# involved.

BEGIN {
	OFS = "."
	extension("libspawk.so", "dlload")
	spawk_select("SELECT TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME " \
		"FROM INFORMATION_SCHEMA.COLUMNS " \
		"WHERE IS_NULLABLE = 'YES'")
	while (spawk_data(column))
		proc_column()

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
		# We use as a flag the `0' index elemnet of `column'
		# array. If exists, then means that this is the first
		# print for this column, so print the column and
		# delete the `0' indexed element.
		if (0 in column) {
			print column[0]
			delete column
		}

		print "\t" data[0]
	}
}
