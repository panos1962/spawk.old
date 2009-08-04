# The following script will print all columns of all tables of all
# databases having unspecified numeric precision.

BEGIN {
	# Awk's "OFS" output field separator is used as columns'
	# separator also for returned rows. We may set another
	# separator for returned rows setting `SPAWKINFO["OFS"]'
	# to an arbitrary string.

	OFS = "."
	extension("libspawk.so", "dlload")
	SPAWKINFO["database"] = "information_schema"

	spawk_select("SELECT TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME, " \
		"NUMERIC_PRECISION FROM COLUMNS")

	# Passing a second array to `spawk_data' will cause that
	# array to be indexed for null columns only.
	while (spawk_data(data, null)) {
		if (4 in null)
			print data[1], data[2], data[3]
	}

	exit(0)
}
