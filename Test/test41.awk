BEGIN {
	extension("libspawk.so", "dlload")
	spawk_update("CREATE DATABASE IF NOT EXISTS `spawk`", 2)
	spawk_update("USE `spawk`")
	spawk_update("CREATE TABLE IF NOT EXISTS `process` (" \
		"`pid` NUMERIC(5) NOT NULL," \
		"`uid` NUMERIC(6) NOT NULL," \
		"`command` VARCHAR(512) NOT NULL" \
		")", 2)
	spawk_update("COMMIT WORK", 2)
	ps = "ps -e -o pid,uid,cmd"
	while ((ps |& getline) > 0)
		add_process($1, $2, $3)

	if (monitor) {
		printf (cnt + 0) " records added successfully"
		if (err += 0) {
			printf ", " err " record"
			if (err > 1)
				printf "s"

			printf " failed to add"
		}

		print ""
	}

	exit(0)
}

function add_process(pid, uid, cmd) {
	gsub("'", "\\'", cmd)
	if (spawk_update("INSERT INTO `process` " \
		"(`pid`, `uid`, `command`) VALUES (" \
		pid "," uid ",'" cmd "')")) {
		print pid ", " uid ", " cmd ": " \
			spawk_error() >"/dev/stderr"
		err++
		return(1)
	}

	cnt++
	return(spawk_update("COMMIT WORK", 2))
}
