BEGIN {
	extension("libspawk.so", "dlload")
	SPAWKINFO["program"] = "bfind"
	SPAWKINFO["database"] = "library"
	abort(set_criteria())
	if (!sort)
		sort = "ORDER BY `bo_title`, `bo_key`"

	FS = "\t"
	OFS = "\t"
}

NF > 0 {
	spawk_query("SELECT `bo_key`, `bo_title` " \
		"FROM `book` WHERE 1 = 1")
	for (i = 1; i <= n_criteria; i++) {
		if ((criteria_list[i] == "b") && (NF >= i)) {
			spawk_query(" AND `bo_key` = " $i)
			continue
		}

		if ((criteria_list[i] == "t") && (NF >= i)) {
			spawk_query(" AND `bo_title` LIKE '" \
				spawk_string($i) "'")
			continue
		}

		if ((criteria_list[i] == "s") && (NF >= i)) {
			spawk_query(" AND `bo_key` IN (SELECT " \
				"`su_book` FROM `subject` WHERE " \
				"`su_subject` LIKE '" \
				spawk_string($i) "')")
			continue
		}

		if ((criteria_list[i] == "a") && (NF >= i)) {
			spawk_query(" AND `bo_key` IN (SELECT " \
				"`au_book` FROM `author` WHERE " \
				"`au_writer` IN (SELECT `wr_key` " \
				"FROM `writer` WHERE `wr_name` " \
				"LIKE '" spawk_string($i) "'))")
			continue
		}
	}

	if (spawk_update(" " sort)) {
		errmsg("invalid pattern(s) (" spawk_error() ")")
		next
	}

	spawk_results()
	while (spawk_data(book))
		print_book(book)
}

function print_book(book,			data) {
	print book[2]
	spawk_select("SELECT `au_writer` FROM `author` " \
		"WHERE `au_book` = " book[1])
	if (!spawk_data(data))
		return

	print "Authors:"
	do {
		print OFS writer(data[1])
	} while (spawk_data(data))

	spawk_select("SELECT `su_subject` FROM `subject` " \
		"WHERE `su_book` = " book[1])
	if (!spawk_data(data))
		return

	print "Subjects:"
	do {
		print OFS data[1]
	} while (spawk_data(data))

	spawk_select("SELECT `co_key`, `co_location` FROM `copy` " \
		"WHERE `co_book` = " book[1])
	if (!spawk_data(data))
		return

	print "Copies:"
	do {
		print OFS data[0]
	} while (spawk_data(data))
}

function writer(wk,			data) {
	spawk_select("SELECT `wr_name` FROM `writer` " \
		"WHERE `wr_key` = " wk)
	spawk_first(data)
	return(data[1])
}

function set_criteria(				l, i, c, errs) {
	if ((!criteria) || ((l = length(criteria)) <= 0)) {
		criteria_list[++n_criteria] = "b"
		return(0)
	}

	
	for (i = 1; i <= l; i++) {
		if ((c = substr(criteria, i, 1)) ~ /[bB]/) {
			errs += set_criteria_item(c, "b")
			continue
		}

		if (c ~ /[tT]/) {
			errs += set_criteria_item(c, "t")
			continue
		}

		if (c ~ /[sS]/) {
			errs += set_criteria_item(c, "s")
			continue
		}

		if (c ~ /[aAwW]/) {
			errs += set_criteria_item(c, "a")
			continue
		}
	}

	return(errs + 0)
}

function set_criteria_item(s, c,			i) {
	for (i in criteria_list) {
		if (criteria_list[i] == c)
			return(errmsg(criteria \
				" : multiple criteria (" s ")"))
	}

	criteria_list[++n_criteria] = c
	return(0)
}

function errmsg(msg) {
	print SPAWKINFO["program"] ": " msg >"/dev/stderr"
	return(1)
}

function abort(n) {
	if (n)
		exit(errmsg("program aborted"))
}
