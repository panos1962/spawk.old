#include "my_global.h"
#include "my_sys.h"
#include "mysql.h"
#include "mysqld_error.h"

// The following symbolic constants have been defined in MySQL
// header files above. These symbolic constants are defined in
// gawk header files also. So, I undefine them now and proceed
// with the gawk (re)definitions. This must not be a problem
// assuming that MySQL macros used in this code (if any) don't
// use these symbolic constants.

#undef HAVE_LANGINFO_CODESET
#undef HAVE_MBRLEN
#undef HAVE_MBRTOWC
#undef PACKAGE
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#undef VERSION

#include "awk.h"

///////////////////////////////////////////////////////////////////////

#define SPAWK_MAX_SERVER 16			// max # of servers
static unsigned int max_query_len = 16380;	// max query length
static unsigned int max_row_len = 16380;	// max row length

///////////////////////////////////////////////////////////////////////

// Results are returned from SQL queries in arrays indexed as follows:
// 0 for the whole row (record), 1 for the first column (field), 2 for
// the second column etc. We can, of course, change this default
// behavior supplying a comma separated index string as `spawk_select's
// second argument. In that case the array is indexed with "" (empty
// string) for the whole record, and the indices supplied for the
// columns. If there are more columns than the supplied indices, then
// the extra columns are been discarded. The indices supplied in the
// way we just described are kept in separate `DATA_INDEX' chains for
// every server. Null root node for a specific server means number
// indexing (default) for this server.

typedef struct DATA_INDEX {
	char *idx;			// index string
	size_t len;			// index string's length
	struct DATA_INDEX *next;	// next column's index node
} DATA_INDEX;

///////////////////////////////////////////////////////////////////////

// The following variables are considered global in the code but the
// scope is that source file only (static).

// We use both `n_server' as a server counter and `cur_server' as
// the current server cursor in the server stack. These two
// variables must always differ by one, e.g. when `n_server' is 3,
// `cur_server' is 2, when `n_server' is 6, `cur_server' is 5 etc.
// However, this is not true at the begining; `cur_server' must be
// set to -1 to have things in order, but we set `cur_server' to -2
// initially to be used as a flag; actually this flag will be used
// later to initialize the SPAWK module. During this initialization
// the `cur_server' will be set to -1 which is the correct initial
// value for this cursor. Finally, the reason for using two variables
// instead of one is efficiency; sometimes it's better to use
// `n_server', other times it's more convenient to use `cur_server'.
// We only must take care always to increment or decrement both of
// these two variables.
static int n_server = 0;
static int cur_server = -2;

// The following arrays are various server attributes. We could, of
// course, have defined a structure to describe servers and use a single
// array of this structure data type, but we preferred to use separate
// arrays instead.
static MYSQL *conn[SPAWK_MAX_SERVER];
static char *query[SPAWK_MAX_SERVER];
static char *cursor[SPAWK_MAX_SERVER];
static int busy[SPAWK_MAX_SERVER];
static MYSQL_RES *res_set[SPAWK_MAX_SERVER];
static int field_count[SPAWK_MAX_SERVER];
static DATA_INDEX *data_index[SPAWK_MAX_SERVER];

// The following array may have one extra place in order to avoid
// extra checks. By the way, the `affected' server attribute denotes
// the number of rows affected by the last update. Because this
// attribute concerns the last popped server, having one extra place
// in this array, we can take the next server's attribute as the
// requested value without warring if this is the last server in the
// stack.
static my_ulonglong affected[SPAWK_MAX_SERVER + 1];

// Parameter values in MySQL configuration files (`/etc/my.cnf'
// and `~/.my.cnf') can be set for [client] and [spawk] groups. We
// can, of course, set other parameters as well concerning specific
// program names, e.g. [payroll], [bank] etc. These parameters
// will become part of the awk process after invoking `spawk_program'
// (setting the program's name), or calling the first spawk function
// (except `spawk_debug'). The parameters will be loaded in the awk
// `SPAWKINFO' array just like the environment variables are loaded
// in `ENVIRON' array. The `password_prompt' variable is being used
// to prompt for MySQL password, while `OFS' variable is being used
// as output column separator in SQL queries that return results.
// If not defined, then the default awk `OFS' is being used instead.
// A list of SPAWK specific parameters follows:
//
//	password_prompt	The prompt to be used when asking for
//			password to connect. If not specified,
//			then the default prompt is used.
//
//	OFS		Column separator for returned rows. If not
//			specified, then the "OFS" awk variable is
//			used.
//
//	null		A string to be used for null valued columns
//			when returning rows. If not specified, then
//			nothing is printed for null valued columns.
//
//	max_query_len	The maximum length for any given query to
//			be processed by SPAWK. Default is 16380
//			characaters.
//
//	max_row_len	The maximum length for any returned row.
//			Default is 16380 characaters.

static NODE *spawkinfo = NULL;

// The `debug' file pointer is being used to write messages in a
// debug file. Function `spawk_debug' will help in opening, closing
// or appending messages to a debug file.
static FILE *debug = NULL;

// The following variables are defined only to avoid constant string
// repetitions in the code.
static char *empty_string = "";
static char *comment_string = "-- ";
static char *ofs_string = "OFS";
#define OFS_LEN 3
static char *null_string = "null";
#define NULL_LEN 4
static char *SPAWKINFO = "SPAWKINFO";
static char *select_string = "select";
static char *program_string = "program";
#define PROGRAM_LEN 7
static char *debug_string = "[[DBG]] ";
