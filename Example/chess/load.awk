BEGIN {
	extension("libspawk.so", "dlload")
	SPAWKINFO["program"] = "LoadData"
	SPAWKINFO["database"] = "chess"

system("mysql -A <chess.dd")

	# We'll need lot of random data here, so set the process-id
	# as the seed for the random generation function.

	srand(PROCINFO["pid"])
	load_data()
}

function load_data() {
	# The date bounds for all the tournaments to be loaded
	# can be given (externally) using the `begin' and `end'
	# dates. If not given, then set the period to a decade.

	abort(set_begin_end(19980101, 20071231))

	# External variable `tournaments' is the number of tournaments
	# to be loaded in the database. Assume that there are about
	# 1000 tournaments per year, or else 3 for every day. If not
	# set, then set that number to a default value of the number
	# of days multiplied by 3.

	if (!tournaments)
		tournaments = int(days * 3)

	# Number of players involved can be given using the
	# (external) variable `players' will be calculated according
	# to the number of turnaments, let's say an average of
	# 10 players per tournament.

	if (!players)
		players = tournaments * 10

	if (monitor) {
		print "Interval...: " begin " - " end \
			" (" days " days)" >monitor
		print "Players....: " players " (" bdate_min \
			" - " bdate_max ", " bdate_days ")" >monitor
		print "Tournaments: " tournaments >monitor
		fflush(monitor)
	}

	generate_words()
	load_players()
	load_tournaments()
}

# Function `set_begin_end' computes the lower and upper date
# bounds for our data to load. That is the tournaments (and games)
# to be loaded must lie between two days: `begin' and `end'.
# The two variables can be set externally (via command line
# -v options), but you can set only the `begin', or only the
# `end' bound date, or neither of two. In those cases the
# time period will be calculated to a decade passed as two
# dates in arguments `from' and `to' (1998-01-01 2008-12-31).
# In any case the function will set the `begin' and `end' dates
# and compute the days between those two dates in the `days'
# variable.

function set_begin_end(from, to,		data) {
	if (monitor) {
		print "Setting begin/end dates" >monitor
		fflush(monitor)
	}

	if (begin && end)
		return(check_begin_end())

	# One of the `begin', `end' variables, or both are not
	# set. In that case compute the time period of the
	# tournaments to be dispersed from the `from' and `to'
	# arguments (usually a dedade) and use that period
	# to compute the missing ends.

	spawk_select("SELECT DATEDIFF(" to "," from ")")
	if ((!spawk_first(data)) || (data[1] < 1))
		return(errmsg(from "-" to \
			": invalid default begin/end dates"))

	# If the `begin' is set, then calculate the `end' date
	# by adding the days calculated. If, on the other hand,
	# the `end' date is set, then calculate the `begin'
	# date by subtracting the calculated days.

	if (begin) {
		spawk_select("SELECT DATE_FORMAT(" begin \
			" + INTERVAL " data[1] " DAY, '%Y%m%d')")
		if ((!spawk_first(data)) || (data[1] == ""))
			return(errmsg(begin ": invalid begin date"))

		end = data[1]
	}
	else {
		spawk_select("SELECT DATE_FORMAT(" end \
			" - INTERVAL " data[1] " DAY, '%Y%m%d')")
		if ((!spawk_first(data)) || (data[1] == ""))
			return(errmsg(end ": invalid end date"))

		begin = data[1]
	}

	# Now check the (calculated) `begin' and `end' dates and
	# compute the days between them.

	return(check_begin_end())
}

# Function `check_begin_end' is the last step in the setting
# of the `begin' and `end' dates. The function will return
# zero on success, or else print a message and return non zero.

function check_begin_end() {
	if (monitor) {
		print "Checking begin/end dates" >monitor
		fflush(monitor)
	}

	spawk_select("SELECT DATEDIFF(" end "," begin")")
	if ((!spawk_first(data)) || (data[1] < 1))
		return(errmsg(begin "-" end ": invalid begin/end dates"))

	# Function `check_begin_end' checks the `begin' and `end'
	# dates and calculate the days between those two dates.
	# Those days will set the `days' global variable value.

	days = data[1]
	spawk_select("SELECT DATE_FORMAT(" begin \
		" - INTERVAL 25550 DAY, '%Y%m%d'), DATE_FORMAT(" \
		end " - INTERVAL 5800 DAY, '%Y%m%d')")
	spawk_first(data)
	spawk_select("SELECT DATEDIFF(" (bdate_max = data[2]) "," \
		(bdate_min = data[1]) ")")
	spawk_first(data)
	bdate_days = data[1]
	return(0)
}

function load_players(				l, i) {
	if (monitor) {
		print "Loading " players " players" >monitor
		fflush(monitor)
	}

	player_list[++i] = "Deep Blue (IBM)"
	player_list[++i] = "Fritz 2.0"
	player_list[++i] = "Fritz 3.0"
	player_list[++i] = "Rybka"
	player_list[++i] = "Kasparov Gary"
	player_list[++i] = "Karpov Anatoli"
	player_list[++i] = "Fisher Bobby"
	for (i = 1; i <= players; i++) {
		spawk_update("INSERT INTO player (pl_key, pl_name, " \
			"pl_bdate) VALUES (" i ",'" \
			(i in player_list ? player_list[i] : \
			random_word() " " random_word()) \
			"'," bdate_min " + INTERVAL " \
			random(bdate_days) " DAY)", 2)

		if ((i % 1000) == 0) {
			if (monitor) {
				print "Commiting " i \
					" player insertions" >monitor
				fflush(monitor)
			}

			spawk_update("COMMIT WORK", 2)
		}
	}

	spawk_update("COMMIT WORK", 2)
	if (monitor) {
		print "Commited " players " player insertions" >monitor
		fflush(monitor)
	}
}

function load_tournaments(				i) {
	if (monitor) {
		print "Loading " tournaments " tournaments" >monitor
		fflush(monitor)
	}

	max_level = 3		# maximum tournament depth
	max_sub = 10		# maximum number of sub-tournaments
	max_duration = 100	# maximum tournament's dur in days
	min_games = 4		# minimum tournament's games count
	max_games = 100		# maximum tournament's games count
	tdl = int(column_length("tournament", "tr_desc") * 0.40)
	points_list[0] = "10,0"
	points_list[1] = "0,10"
	points_list[2] = "5,5"
	for (tr_key = 0; load_tournament();)
		;

	spawk_update("COMMIT WORK", 2)
	if (monitor) {
		print "Commited " tournaments \
			" tournament insertions" >monitor
		print "Commited " gm_key " games insertions" >monitor
		fflush(monitor)
	}
}

function load_tournament(ht, level,			n, desc, \
	l, nw, w, tk, games) {
	if (++tr_key > tournaments)
		return(0)

	nw = 2 + random(10)
	desc = random_word()
	l = length(desc) + 1
	while (nw-- > 0) {
		if ((l += length(w = random_word())) > tdl)
			break

		desc = desc " " w
		l++
	}

	if (!ht)
		ht = "NULL"

	spawk_update("INSERT INTO tournament (tr_key, tr_desc, " \
		"tr_begin, tr_end, tr_other) VALUES (" tr_key ",'" \
		desc "',20010101,20010101," ht ")", 2)
	games = random(max_games)
	if (level)
		games /= level

	if (games < min_games)
		games = min_games

	while (games-- > 0)
		load_game()

	if ((tr_key % 1000) == 0) {
		if (monitor) {
			print "Commiting " tr_key \
				" tournament insertions" >monitor
			fflush(monitor)
		}

		spawk_update("COMMIT WORK", 2)
	}

	if ((level < max_level) && (random(5) == 1)) {
		n = 2 + random(max_sub)
		for(tk = tr_key; n-- > 0;) {
			if (!load_tournament(tk, level + 1))
				return(0)
		}
	}

	return(1)
}

function load_game() {
	spawk_update("INSERT INTO game (gm_key, gm_tournament, " \
		"gm_kind, gm_minutes, gm_white, gm_welo, gm_black, " \
		"gm_belo, gm_begin, gm_end, gm_wpoints, gm_bpoints, " \
		"gm_stop) VALUES (" ++gm_key "," tr_key "," \
		1 "," 120 "," random_player() "," 0 "," \
		random_player() "," 0 ",20080101,20080210," \
		points_list[random(3)] ",1)", 2)
	if ((gm_key % 1000) == 0) {
		if (monitor) {
			print "Commiting " gm_key \
				" games insertions" >monitor
			fflush(monitor)
		}

		spawk_update("COMMIT WORK", 2)
	}
}

function generate_words(			i, upper, \
	lower, l, s) {
	minwl = 4		# minimum word length
	maxwl = 16		# maximum word length
	if (!words)		# words database size
		words = 10000

	if (monitor) {
		print "Generating " words " random words (" \
		minwl " - " maxwl ")" >monitor
		fflush(monitor)
	}

	for (i = 65; i < 91; i++)
		upper[i - 65] = sprintf("%c", i)

	for (i = 97; i < 123; i++)
		lower[i - 97] = sprintf("%c", i)

	for (i = 0; i < words; i++) {
		l = minwl + random(maxwl - minwl)
		s = upper[random_character()]
		while (--l > 0)
			s = s lower[random_character()]

		word_list[i] = s
	}
}

function random_word() {
	return(word_list[random(words)])
}

function random_string(len1, len2, spaces,		l, s, \
	i, space, dif, w) {
	if ((l = int(len1 + random(dif = len2 -len1))) > len2)
		l = len2
	else if (l < len1)
		l = len1

	if (spaces) {
		if ((spaces = random(spaces)) <= 0)
			spaces = 1

		if (spaces == 1) {
			if ((i = random(l)) < mwl)
				i = mwl
			else if (l - i <= mwl)
				i = int(l / 2)

			space[i]
		}
		else {
			for (i = 0; i < spaces; i++)
				space[len1 + random(dif)]
		}
	}

	s = random_upper()
	for (i = 1; i < l; i++) {
		if (i in space) {
			s = s " "
			s = s random_upper()
			i++
		}
		else
			s = s random_lower()
	}

	return(s)
}

function random_character(			c, n) {
	while ((c = int(rand() * 26) % 26) == last_random_character) {
		if (++n > 3)
			break
	}

	return(last_random_character = c)
}

function random_player(				p) {
	if ((p = random(players) + 1) == previous_player)
		p = previous_player + (p <= 1 ? 1 : -1)

	return(previous_player = p)
}

# Function `random' is a utility function accepting a positive
# integer `n' and returning a random number in [0, n).

function random(n) {
	return(int((rand() * n)))
}

function column_length(table, column,			data) {
	spawk_select("SELECT CHARACTER_MAXIMUM_LENGTH " \
		"FROM information_schema.COLUMNS " \
		"WHERE TABLE_SCHEMA = 'chess' AND TABLE_NAME = '" \
		table "' AND COLUMN_NAME = '" column "'")
	spawk_first(data)
	if (data[1] < 1)
		fatal(table "." column ": cannot identify column")

	return(data[1] + 0)
}

function errmsg(msg) {
	print SPAWKINFO["program"] ": " msg >"/dev/stderr"
	return(1)
}

function fatal(msg) {
	exit(errmsg(msg))
}

function abort(n) {
	if (n)
		exit(errmsg("program aborted"))
}
