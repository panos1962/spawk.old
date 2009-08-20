BEGIN {
	extension("libspawk.so", "dlload")
	SPAWKINFO["database"] = "chess"
	tcl = "tr_key,tr_desc,tr_begin,tr_end,tr_other"
	gcl = "gm_white,gm_black,gm_wpoints,gm_bpoints"
	score_list["10:0"] = "(1-0)"
	score_list["0:10"] = "(0-1)"
	score_list["5:5"] = "(1/2-1/2)"
	score_list[":"] = ""
}

{
	select_tournaments($0 ~ /^[ \t]*[0-9][0-9]*[ \t]*$/ ? \
		"tr_key = " $0 : "tr_desc LIKE '" spawk_string($0) "'")
	while (spawk_data(tournament))
		print_tournament(tournament)
}

function select_tournaments(condition) {
	spawk_query("SELECT " tcl " FROM tournament")
	if (condition)
		spawk_query(" WHERE " condition)

	spawk_select(" ORDER BY tr_begin, tr_key", tcl)
}

function print_tournament(tournament, level,		other) {
	print indent(level) tournament["tr_desc"], \
		tournament["tr_begin"]
	if (games)
		print_games(tournament["tr_key"], level)

	if (tree) {
		select_tournaments("tr_other = " tournament["tr_key"])
		while (spawk_data(other))
			print_tournament(other, level + 1)
	}
}

function print_games(tk, level,			game) {
	spawk_select("SELECT " gcl " FROM game WHERE gm_tournament = " \
		tk " ORDER BY gm_begin, gm_key", gcl)
	while (spawk_data(game))
		print indent(level + 1) player(game["gm_white"]), \
			"VS", player(game["gm_black"]), \
			score(game["gm_wpoints"] ":" game["gm_bpoints"])
}

function player(pk,			data) {
	if (pk in player_list)
		return(player_list[pk])

	spawk_select("SELECT pl_name FROM player " \
		"WHERE pl_key = " pk)
	spawk_first(data)
	return(player_list[pk] = data[1])
}

function score(wb) {
	return (wb in score_list ? score_list[wb] : "(?)")
}

function indent(level,			i, ind) {
	if (level in indent_ok)
		return(indent_ok[level])

	for (i = level; i > 0; i--)
		ind = ind "\t"

	return(indent_ok[level] = ind)
}
