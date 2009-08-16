BEGIN {
	extension("libspawk.so", "dlload")

	# Name the program and set `progname' to that name. We
	# could, of course, use `SPAWKINFO["program"]' instead,
	# but `progname' is more convenient and lighter.
	spawk_program(progname = "create_payroll")

	# Create the database `spawk' if not exists. Payroll tables
	# belong to the tutorial `spawk' database. After create,
	# make `spawk' the current database.
	spawk_update("CREATE DATABASE IF NOT EXISTS spawk", 2)
	spawk_update("USE spawk", 2)

	# Now it's time to drop payroll tables and recreate them.
	# If anything goes wrong, exit.
	if (!drop_payroll()) {
		print progname \
			": payroll: cannot drop schema" >"/dev/stderr"
		exit(2)
	}

	if (create_payroll()) {
		print progname \
			": payroll: cannot create schema" >"/dev/stderr"
		exit(drop_payroll())
	}

	exit(0)
}

# Function `create_payroll' calls `create_table' for all payroll
# tables. If something goes wrong, return non-zero, else return
# zero.

function create_payroll() {
	if (create_table("person", \
		"pekey NUMERIC(6) NOT NULL PRIMARY KEY," \
		"pename CHARACTER(40) NOT NULL," \
		"INDEX USING BTREE (pename)," \
		"pedept CHARACTER(30) NOT NULL"))
		return(1)

	if (create_table("pewhat", \
		"pekey NUMERIC(4) NOT NULL PRIMARY KEY," \
		"pedesc CHARACTER(30) NOT NULL"))
		return(1)

	if (create_table("peparam", \
		"pape NUMERIC(6) NOT NULL," \
		"FOREIGN KEY (pape) REFERENCES person (pekey)," \
		"pawhat NUMERIC(4) NOT NULL," \
		"pafrom DATE NOT NULL," \
		"paval NUMERIC(9) NOT NULL"))
		return(1)

	if (create_table("payroll", \
		"pakey NUMERIC(4) NOT NULL PRIMARY KEY," \
		"padesc CHARACTER(60) NOT NULL," \
		"padate DATE NOT NULL"))
		return(1)

	if (create_table("pawhat", \
		"pakey NUMERIC(4) NOT NULL PRIMARY KEY," \
		"padesc CHARACTER(30) NOT NULL"))
		return(1)

	if (create_table("paparam", \
		"papa NUMERIC(4) NOT NULL," \
		"FOREIGN KEY (papa) REFERENCES payroll (pakey)," \
		"pawhat NUMERIC(4) NOT NULL," \
		"paval NUMERIC(9) NOT NULL"))
		return(1)

	if (create_table("pdwhat", \
		"pdkey NUMERIC(4) NOT NULL PRIMARY KEY," \
		"pddesc CHARACTER(30) NOT NULL"))
		return(1)

	if (create_table("paydata", \
		"pdpa NUMERIC(4) NOT NULL PRIMARY KEY," \
		"FOREIGN KEY (pdpa) REFERENCES payroll (pakey)," \
		"pdpe NUMERIC(6) NOT NULL," \
		"FOREIGN KEY (pdpe) REFERENCES person (pekey)," \
		"pdwhat NUMERIC(4) NOT NULL," \
		"pdval NUMERIC(9) NOT NULL"))
		return(1)

	if (create_table("piwhat", \
		"pikey NUMERIC(4) NOT NULL PRIMARY KEY," \
		"pidesc CHARACTER(30) NOT NULL"))
		return(1)

	if (create_table("pout", \
		"popa NUMERIC(4) NOT NULL," \
		"pope NUMERIC(6) NOT NULL," \
		"FOREIGN KEY (pope) REFERENCES person (pekey)," \
		"PRIMARY KEY USING BTREE (popa, pope)," \
		"pocalc DATE NOT NULL," \
		"podept CHARACTER(30) NOT NULL"))
		return(1)

	if (create_table("pitem", \
		"pipa NUMERIC(4) NOT NULL PRIMARY KEY," \
		"pipe NUMERIC(6) NOT NULL," \
		"FOREIGN KEY (pipa, pipe) " \
		"REFERENCES pout (popa, pope)," \
		"piwhat NUMERIC(4) NOT NULL," \
		"pival NUMERIC(9) NOT NULL"))
		return(1)

	return(0)
}

# Function `create_table' creates a table. Two arguments are passed:
# table's name as first argument and table's columns' specifications
# as a string. Returns no-zero on error, else zero.

function create_table(table, spec) {
	if (spawk_update("CREATE TABLE " table "(" spec ")")) {
		print progname ": " table \
			": cannot create table" >"/dev/stderr"
		return(1)
	}

	return(0)
}

# Function `drop_payroll' is straightforward. Drop all payroll
# tables. If anything goes wrong, then return zero, else return zero.

function drop_payroll() {
	if (drop_table("pitem")) return(0)
	if (drop_table("pout")) return(0)
	if (drop_table("piwhat")) return(0)
	if (drop_table("paydata")) return(0)
	if (drop_table("pdwhat")) return(0)
	if (drop_table("paparam")) return(0)
	if (drop_table("pawhat")) return(0)
	if (drop_table("payroll")) return(0)
	if (drop_table("peparam")) return(0)
	if (drop_table("pewhat")) return(0)
	if (drop_table("person")) return(0)
	return(2)
}

function drop_table(table) {
	if (spawk_update("DROP TABLE IF EXISTS " table)) {
		print progname ": " table \
			": cannot drop table (" spawk_error() \
			" -- " spawk_error() >"/dev/stderr"
		return(1)
	}

	return(0)
}
