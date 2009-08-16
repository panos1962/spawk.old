BEGIN {
	extension("libspawk.so", "dlload")
	SPAWKINFO["database"] = "spawk"
	for (i = 1; i < 100; i++) {
		spawk_select("SELECT uid, login FROM `user`")
		while (spawk_data(data))
			print i, data[0]
	}

	spawk_select("SELECT uid, login FROM `user`")
	print "Begin" >"/dev/stderr"
	while (spawk_data(data))
		print data[0]

	exit(0)
}
