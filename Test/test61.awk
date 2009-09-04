BEGIN {
	spawk_database("information_schema")
	spawk_database("spawk")
	print spawk_exec_insert("`file`", "inode, date, type, user, " \
		"path, size", "123, 20090812, 'file', 501, " \
		"'asdasd', 100", "SAK")
spawk_errmsg("SJDGSJDGJ", 2)
exit(2)
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
