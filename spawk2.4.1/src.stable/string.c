///////////////////////////////////////////////////////////////////////

// do_spawk_string --- escape certain characters (quotes, etc)

// USAGE
// spawk_string([string])

// RETURNS
// The string escaped (string).

static NODE *
do_spawk_string(NODE *tree)
{
	int argc;
	NODE *arg;
	char *new;
	char *p;
	char *q;

	if (((argc = get_curfunc_arg_count()) != 1) && do_lint)
		lintwarn("string: must be called with "
			"one argument");

	switch (argc) {
	case 1:
		break;
	default:
		fatal("string: invalid # of arguments");
	}

	force_string(arg = get_scalar_argument(tree, 0, FALSE));
	if ((new = malloc(arg->stlen * 2)) == NULL)
		fatal("out of memory allocating escaped "
			"string buffer");

	for (p = arg->stptr, q = new; *p != '\0'; p++) {
		switch (*p) {
		case '\'':
		case '\"':
		case '\\':
			*q++ = '\\';
		default:
			*q++ = *p;
		}
	}

	*q = '\0';
	free_temp(arg);
	set_value(tmp_string(new, q - new));
	free(new);
	return(tmp_number((AWKNUM) 0));
}
