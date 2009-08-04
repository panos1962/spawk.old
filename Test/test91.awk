BEGIN {
	extension("libspawk.so", "dlload")
	SPAWKINFO["database"] = "information_schema"
	spawk_update("SELECT TABLE_SCHEMA, TABLE_NAME " \
		"FROM COLUMNS")

	spawk_results()
	spawk_clear()
	spawk_select("SELECT COLUMN_NAME FROM COLUMNS")
	while (spawk_data(data))
		print data[0]

	exit(0)
}
