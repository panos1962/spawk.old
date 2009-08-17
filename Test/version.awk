BEGIN  {
	extension("libspawk.so", "dlload")
	print SPAWKINFO["version"]
	exit(0)
}
