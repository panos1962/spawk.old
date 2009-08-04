# The following script will print all columns of all tables of all
# databases having unspefied numeric precision, just as the previous
# script did, but with some small defferences.

BEGIN {
	OFS = "."
	extension("libspawk.so", "dlload")
	SPAWKINFO["database"] = "information_schema"

	# Passing a second argument to `spawk_select' will cause
	# string indexing in returned data array. The whole record
	# does not be indexed with 0 but rather with "", while the
	# returned columns will be indexed according to the given
	# string (comma separated indices).
	spawk_select("SELECT TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME, " \
		"NUMERIC_PRECISION FROM COLUMNS",
		"schema,table,column,prec")

	# Passing a second array to `spawk_data' will cause that
	# array to be indexed for null columns only.
	while (spawk_data(data, null)) {
		if ("prec" in null)
			print data["schema"], data["table"]
	}

	exit(0)
}
