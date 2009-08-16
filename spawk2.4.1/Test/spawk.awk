# Function `login' accepts user-id as its first (and only) argument
# and returns the corresponding login name of the user with the given
# user-id.
#
# The function could of course run a auery to get the login name,
# but we prefer to read all login names in the global array `login_data'
# the first time the function is called. We use the zero indexed element
# as a flag to know if the table is already in core.

function login(uid,			data) {
	if (!(0 in login_data)) {
		delete login_data	# just in case!
		login_data[0]		# mark in core
		spawk_select("SELECT `uid`, `login` FROM `user`")
		while (spawk_data(data))
			login_data[data[1]] = data[2]
	}

	if (!(uid in login_data))
		print uid ": user not found" >"/dev/stderr"

	return(login_data[uid])
}
