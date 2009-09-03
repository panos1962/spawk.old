BEGIN {
	spawk_database("information_schema")
	spawk_database("spawk")
	spawk_exec_update("`file`", "inode = 0, " \
		"date = date + 1", "path LIKE 'a%'")
	spawk_update("COMMIT WORK", 2)
	spawk_exec_select("`path`, `inode`", "`file`", "`inode` > 0")
	while (spawk_data(data)) {
		print data[""]
		delete data[""]
		for (i in data)
			print ">>" i "<<", data[i]
	}

	exit(0)
}
