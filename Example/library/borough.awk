BEGIN {
	extension("libspawk.so", "dlload")
	SPAWKINFO["program"] = "borough"
	SPAWKINFO["database"] = "library"
	set_date()
	borough()
	exit(0)
}

function borough(			data) {
	if (spawk_update("SELECT `br_reader`, `br_copy`, " \
		"`br_date`, `br_until` FROM borough " \
		"WHERE br_return IS NULL AND br_until < " date)) {
		print date ": invalid date" >"/dev/stderr"
		exit(2)
	}

	spawk_results()
	while (spawk_data(data)) {
		if (detail)
			print_borough(data)
		else
			print data[0]
	}
}

function print_borough(borough) {
	print "Reader: " reader(borough[1])
	print "Book..: " title(borough[2])
	print "Date..: " borough[3] " - " borough[4]
	print ""
}

function reader(rk,				data) {
	spawk_select("SELECT `rd_name` FROM `reader` " \
		"WHERE `rd_key` = " rk)
	spawk_first(data)
	return(data[1])
}

function title(ck,				data) {
	spawk_select("SELECT `bo_title` FROM `book` " \
		"WHERE `bo_key` = (SELECT `co_book` FROM `copy` " \
		"WHERE `co_key` = " ck ")")
	spawk_first(data)
	return(data[1])
}

function set_date() {
	if (!date) {
		spawk_select("SELECT DATE_FORMAT(CURDATE(), '%Y%m%d')")
		spawk_first(data)
		date = data[1]
	}
}
