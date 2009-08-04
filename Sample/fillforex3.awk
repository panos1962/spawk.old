# This script analyzes the Forex data to extract the
# trade information and enters into the MySQL data base.

BEGIN {
	extension("libspawk_r.so", "dlload")
	SPAWKINFO["database"] = "forex"
	SPAWKINFO["user"] = "araja"
	FS = ","
}

function GetValue(x,				ndx, val, st)
{
	# The values can be .dddddd, 1.dddddd, dd.dddddd, ddd.dddddd
	# Get a value which is 5 digits except in .dddddd case where
	# we get 4 digits.
	# Actual date is as follows
	# 368412932,EUR/USD,2007-12-30 17:00:03,1.471400,1.471700,D
	# 368412945,EUR/GBP,2007-12-30 17:00:03,.737600,.737900,D
	# 368412943,GBP/JPY,2007-12-30 17:00:03,224.460000,224.550000,D
	# 375023976,AUD/JPY,2008-01-27 17:00:00.000,93.740000,93.810000,D
	if ((ndx = index(x, ".")) == 1)
		val = 0
	else if (ndx == 2)
		val = int(substr(x, 1, 1)) * 10000
	else if (ndx == 3)
		val = int(substr(x, 1, 2)) * 1000
	else if (ndx == 4)
		val = int(substr(x, 1, 3)) * 100

	st = ndx + 1
	# If "." is the first character then use only 4 digits
	# after that.
	if (ndx == 1)
		ndx = 2

	return(val + int(substr(x, st, 6 - ndx)))
}

function GetTimeVal(x,				hms)
{
	# Extract hour, minute, and seconds in to hms array.
	split(x, hms, ":")
	hms[3] = int(substr(hms[3], 1, 2))
	return((((hms[1] * 60) + hms[2]) * 60) + hms[3])
}

{
	# If input pair not equals current pair then perform
	# database lookup.
	if ((ipairStr = $2) != cpairStr) {
		spawk_select("select PairID from pair where Pair='" \
			(cpairStr = ipairStr) "'")
		if (spawk_first(data))
			pairID = data[1]
		else if (spawk_update("INSERT INTO `pair` (`Pair`) " \
			"VALUES ('" ipairStr "')")) {
			spawk_command("ROLLBACK WORK", 2)
			print "insert `pair` failed (`Pair` = '" \
				ipairStr "') (" spawk_error() \
				")" > "/dev/stderr"
			next
		}
		else {
			# Lookup again to find the PairID.
			spawk_select("SELECT `PairID` FROM `pair` " \
				"WHERE `Pair' = '" ipairStr "'")
			if (!spawk_first(data)) {
				spawk_update("ROLLBACK WORK", 2)
				print "`pair' not found after insert! " \
					"(`Pair` = '" ipairStr \
					"')" >"/dev/stderr"
				exit(2)
			}

			pairID = data[1]
			print "New PairID " pairID " for '" ipairStr "'"
		}
	}

	# Convert hour, minute, and seconds into seconds.
	# Keep in mind that 12 hours are 43200 seconds.
	if ((timeVal = GetTimeVal(substr(idateTime = $3, \
		12, 8))) >= 43200) {
		PM = 1
		timeVal -= 43200
	}
	else
		PM = 0

	if ((dateStr = substr(idateTime, 1, 10)) != cdateStr) {
		# Lookup dateStr in the date table
		spawk_select("SELECT `DateID` FROM `date` " \
			"WHERE `DateStr' = '" (cdateStr = dateStr) "'")
		if (spawk_first(data))
			dateID = data[1]
		else if (spawk_update("INSERT INTO date (`DateStr`) " \
			"VALUES ('" dateStr "')")) {
			spawk_command("ROLLBACK WORK", 2)
			print "insert `date` failed (`DateStr` = '" \
				ipairStr "') (" spawk_error() \
				")" > "/dev/stderr"
			next
		}
		else {
			# Lookup again to find the `PairID`.
			spawk_select("SELECT `DateID` FROM `date` " \
				"WHERE `DateStr` = '" dateStr "'")
			if (!spawk_first(data) ) {
				spawk_update("ROLLBACK WORK", 2)
				print "`date` not found after insert! " \
					"(`DateStr` = '" dateStr \
					"')" >"/dev/stderr"
				exit(2)
			}

			dateID = data[1]
			print "New DateID " dateID " for '" dateStr "'"
		}
	}

	# Check if the entry is already in the data base before
	# updateing it.
	spawk_select("SELECT `DateID` FROM `trade` WHERE `PairID` = " \
		(pairID_PM = (pairID * 2) + PM) " AND `DateID` = " \
		dateID " AND TimeVal = " timeVal)
	if (spawk_first(data))
		print "Date for " PairID " Date " data[1] " Time " \
			timeVal " already present"
	else if (spawk_update("INSERT INTO `trade` (`PairID`, " \
		"`DateID`, `TimeVal`, `Bid`, `Ask`) VALUES (" pairID_PM \
		"," dateID "," timeVal "," (bid = GetValue($4)) "," \
		(ask = GetValue($5)) ")")) {
		spawk_command("ROLLBACK WORK", 2)
		print "insert `trade` failed " "(`PairID` = " pairID_PM \
			", `DateID` = " dateID ", `TimeVal` = " timeVal \
			", `Bid` = " bid ", `Ask` = " ask ") (" \
			spawk_error() ")" > "/dev/stderr"
			next
	}

	# Everything ok with current input data line, so that's
	# the correct time to commit.
	spawk_commad("COMMIT WORK", 2)
}
