BEGIN {
	extension("libspawk.so", "dlload")
	SPAWKINFO["database"] = "spawk"
	# spawk_update("DELETE FROM `file`", 2)
	# spawk_update("DELETE FROM `user`", 2)
	for (i = 1001; i < 2100; i++) {
		if (spawk_update("INSERT INTO user (uid, login, " \
			"name, home, dateins, datemod, " \
			"datedel) VALUES (" i ", 'login(" i \
			")', 'name(" i ")', 'home(" i ")'," \
			"20090509,20090510,NULL)"))
			print i ": insert failed (" \
				spawk_error() ")" >"/dev/stderr"
		else
			spawk_update("COMMIT WORK", 2)
	}

	exit(0)
}
