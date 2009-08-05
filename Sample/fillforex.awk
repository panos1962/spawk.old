BEGIN {
	# Load the SPAWK module.
	extension("libspawk_r.so", "dlload")

	# `SPAWKINFO' array is an array to be filled with values
	# set in MySQL configuration files (`/etc/my.cnf' and
	# `~/.my.cnf'). We can, of course, set values externally.
	# The "database" indexed element is the default database
	# to be used with new database connections.
	# We can, alternatively, use `/etc/spawkrc' and `~/.spawkrc'
	# files to run SQL commnands to initialize new servers.
	SPAWKINFO["database"] = "forex"
	SPAWKINFO["user"] = "araja"
    # Database forex is
    # Tables are pair,date, and trade
    # | PairID | tinyint(3) unsigned | NO   | PRI | NULL    | auto_increment | 
    # | Pair   | char(8)             | YES  |     | NULL    |                | 
    #
    # | DateID  | smallint(5) unsigned | NO   | PRI | NULL    | auto_increment | 
    # | DateStr | char(10)             | YES  |     | NULL    |                | 
    #
    # | PairID  | tinyint(3) unsigned  | YES  |     | NULL    |       | 
    # | DateID  | smallint(5) unsigned | YES  |     | NULL    |       | 
    # | TimeVal | smallint(5) unsigned | YES  |     | NULL    |       | 
    # | Bid     | smallint(5) unsigned | YES  |     | NULL    |       | 
    # | Ask     | smallint(5) unsigned | YES  |     | NULL    |       | 

	# This script analyzes the Forex data to extract the trade information and 
	# enters into the MySQL data base;

	FS = ","
	pairID = 0
	dateID = 0
# *** PANOS ***
# Letter "c" stands for "current"?
	cpairStr = ""
	cdateStr = ""
}

function GetValue(valStr)
{
	# The values can be .dddddd, 1.dddddd, dd.dddddd, ddd.dddddd
	# Get a value which is 5 digits except in .dddddd case where we get 4 digits
    # Actual date is as follows
    # 368412932,EUR/USD,2007-12-30 17:00:03,1.471400,1.471700,D
    # 368412945,EUR/GBP,2007-12-30 17:00:03,.737600,.737900,D
    # 368412943,GBP/JPY,2007-12-30 17:00:03,224.460000,224.550000,D
    # 375023976,AUD/JPY,2008-01-27 17:00:00.000,93.740000,93.810000,D
	ndx = index(valStr, ".")
	if (ndx == 1)
		value = 0
	else if (ndx == 2)
		value = int(substr(valStr, 1, 1)) * 10000
	else if (ndx == 3)
		value = int(substr(valStr, 1, 2)) * 1000
	else if (ndx == 4)
		value = int(substr(valStr, 1, 3)) * 100

	st = ndx + 1
	if (ndx == 1)	# If . is the first character then use only 4 digits after that
		ndx = 2

	value = value + int(substr(valStr, st, 6 - ndx))
	return(value)
}

{
	# Extract Pair, date/time string, bid, and ask
# *** PANOS ***
# Letter "i" stands for "input"?
	ipairStr = $2
	idateTime = $3
	bidStr = $4
	askStr = $5
	# if input pair is not equal current pair then we need to do database lookup
	if (ipairStr != cpairStr)
	{
		cpairStr = ipairStr
		# do database lookup
		spawk_select("select PairID from pair where Pair='" ipairStr "'")
# *** PANOS ***
# To fetch just the first record of a given query just use `spawk_first'
# function instead of `spawk_data' in succession. It's just the same
# but musch faster (the loop executes inside the SPAWK object insetead
# of awk interference) and much simpler of course:

		if (spawk_first(data))
			pairID = data[1]

# *** PANOS ***
# instead of:
#		if (spawk_data(data))
#		{
#			# if found then get the pairID
#			pairID = data[1]
##			print "PairID " pairID
#			# Flush output
#			while (spawk_data(data))
#			{
#			}
#		}
		else
		{
			# if pair is not found then insert into pair table
# *** PANOS ***
# Dr. Raja, you can use `spawk_debug' function to log all the SQL
# commands given via SPAWK. Just call `spawk_debug' in the BEGIN
# section, e.g.
#
# spawk_debug("raja1001", "create")
#
# will create the file "raja1001" to log all the SQL commands given
# via SPAWK. After the end of execution you can just look to this
# file to check out the SQL commands proecessed.
# By the way, you can switch log off and on during the execution:
#
# spawk_debug()
#
# Without any arguments, the debug log procedure is turned off.
# When you want to turn it on again:
#
# spawk_debug("raja1001")
#
# When called with a single argument, `spawk_debug' just appends
# the coomands, rather than creating the file. If you follow this
# procedure (switching on/off), it's much better to store the debug
# file name in an awk variable, e.g. sql_log = "raja1001", so you
# can call `spawk_debug' with this name instead of literal file
# name.
#
# Using `spawk_debug' will help you avoid all the "print" commands
# used for debuging purpose.
			spawk_update("insert into pair (Pair) values ('" ipairStr "');")
# *** PANOS ***
# There is really no need of using ";" to end a query. SPAWK just
# ignores the ";" at the end of a query! Actually, ";" is a query
# separation character used by `mysql' client; ";" is not part
# of SQL query, it's just used to separate successive queries.
# SPAWK doesn't handle that kind of queries.

# *** PANOS *** (2)
# What if the above insert fails? No messages, neither any other
# action taken. If you want you can just exit using the second
# argument in `spawk_update':
#
#			spawk_update("insert into pair (Pair) values ('" ipairStr "')", 2)
#
# I also removed the ";" mark!
# Now, if the insert fails, awk will exit with exit status of 2.
# Alternatively, you can take some other action, e.g.
#
#			if (spawk_update("insert into pair (Pair) values ('" ipairStr "');")) {
#				print "some message..." >"/dev/stderr"
#				next
#			}
#
# The `spawk_errno' and `spawk_error' functions will be of much help
# on situations like these.
			# lookup again to find the PairID
			spawk_select("select PairID from pair where Pair='" ipairStr "'")
			if ( spawk_first(data) )
			{
				pairID = data[1]
				print "New PairID " pairID " for " ipairStr
			}
		}

# *** PANOS ***
# I believe now is the right time to commit. Just call `spawk_update'
# with a commit command. In such calls I recommend to call
# `spawk_update' with an exit code; actually, commit failure means
# trouble.
		spawk_update("COMMIT WORK", 2)
	}

	# Extract the date string and time strings
	dateStr = substr(idateTime, 1, 10)
	timeStr = substr(idateTime, 12, 8)

# *** PANOS ***
# From this point I just changed the code, using the guidelines
# mentioned above. I also turned out the time convertion into
# a function:
	# Extract hour, minute, and seconds timeFlds
	if ((timeVal = get_timeVal()) >= 43200) {
		PM = 1
		timeVal -= 43200
	}
	else
		PM = 0

	if (dateStr != cdateStr)
	{
		cdateStr = dateStr
		# Lookup dateStr in the date table
		spawk_select("select DateID from date where DateStr='" dateStr "'")		
		if (spawk_first(data))
			dateID = data[1]
		else
		{
			# if date is not found then insert into date table
			if (spawk_update("insert into date (DateStr) values ('" dateStr "')"))
			{
				print "insert failed..." >"/dev/stderr"
				next
			}

			# lookup again to find the PairID
			spawk_select("select DateID from date where DateStr='" dateStr "'")
			if (spawk_first(data) )
			{
				dateID = data[1]
				print "New DateID " dateID " for " dateStr
			}
		}
	}

	bid = GetValue(bidStr)
	ask = GetValue(askStr)
#	print "Bid " bid " Ask " ask
	# Check if the entry is already in the data base before updateing it
	pairID_PM = (pairID * 2) + PM
	spawk_select("select DateID from trade where PairID=" pairID_PM " and DateID=" dateID " and TimeVal=" timeVal)
	if (spawk_first(data))
	{
		dateID = data[1]
		print "Date for " PairID " Date " dateID " Time " timeVal " already present"
	}
	else
	{
		print "insert into trade (PairID,DateID,TimeVal,Bid,Ask) values (" pairID_PM "," dateID "," timeVal "," bid "," ask ");"
		if (spawk_update("insert into trade (PairID,DateID,TimeVal,Bid,Ask) values (" pairID_PM "," dateID "," timeVal "," bid "," ask ")"))
		{
			print "insert failed..." >"/dev/stderr"
			next
		}
	}
}

function get_timeVal(				hms)
{
	# Extract hour, minute, and seconds hms
	split(timeStr, hms, ":")
	hms[3] = int(substr(hms[3], 1, 2))
	return((((hms[1] * 60) + hms[2]) * 60) + hms[3])
}
