[Print](http://code.google.com/p/spawk/wiki/Tutorial?show=content,meta)

  * [Introduction](Tutorial#Introduction.md)
  * [Counting Tables and Columns](Tutorial#Counting_Tables_and_Columns.md)
  * [Data Types' Statistics](Tutorial#Data_Types'_Statistics.md)
  * [User/Files Statistics](Tutorial#User/Files_Statistics.md)
  * [SPAWK Playing Chess](Tutorial#SPAWK_Playing_Chess.md)
    * [Checking Tournaments](Tutorial#Checking_Tournaments.md)
    * [Printing Games](Tutorial#Printing_Games.md)
    * [Loading Random Chess Data](Tutorial#Loading_Random_Chess_Data.md)
  * [SPAWK Librarian](Tutorial#SPAWK_Librarian.md)
    * [Printing Books' Data](Tutorial#Printing_Books'_Data.md)
    * [Checking Circulation Delays](Tutorial#Checking_Circulation_Delays.md)
    * [Using Awk Libraries](Tutorial#Using_Awk_Libraries.md)
<u>
<h1>Introduction</h1>
</u>

In that page you'll find a lot of SPAWK example codes. Most of the examples are referred to `information_schema`, thus the data dictionary of every MySQL database, but you'll also find examples of creating new tables via SPAWK.

[Top](Tutorial.md)

<u>
<h1>Counting Tables and Columns</h1>
</u>

The following SPAWK code reads schema (database) names and prints the schema name, the number of tables and the number of columns of the specified schemas:
```
BEGIN {
    extension("libspawk.so", "dlload")
    SPAWKINFO["database"] = "information_schema"
}

{
    spawk_select("SELECT COUNT(*) FROM TABLES " \
        "WHERE TABLE_SCHEMA = '" $1 "'")
    spawk_first(data)
    table_count = data[1] + 0

    spawk_select("SELECT COUNT(*) FROM COLUMNS " \
        "WHERE TABLE_SCHEMA = '" $1 "'")
    spawk_first(data)

    print $1, table_count, data[1] + 0
}
```
If saved in the `schematc` file, then you can run:
```
awk -f schematc
```
and provide schema (database) names to the standard input of the command. For each given schema there will be printed the schema name, the number of tables and the number of columns of the schema at the standard output. If the schema names are in the files `schema1` and `schema2`, then you can run:
```
awk -f schematc schema1 schema2
```
Now let's say we want to give an "option" for printing all the schemas of the database without reading schema names from the input. To do so it's a good practice to use awk functions in the previous code. Let's make it better. We'll hide the tables' and columns' counting for a given schema in a function that takes the schema name as its first argument:
```
BEGIN {
    extension("libspawk.so", "dlload")
    SPAWKINFO["database"] = "information_schema"
}

{
    schematc($1)
}

# The `schematc' function takes as its first argument
# a schema name and prints to the standard output the
# given schema name followed by the number of the tables
# and columns in that schema. Notice the `data' argument
# which stands for an awk function "local" variable, that
# is a variable with local scope; `data' variable is
# known only inside the `schematc' function. The same is
# true for the `tcnt' variable which is used for temporary
# tables' count storage.

function schematc(schema,                 data, tcnt) {
    spawk_select("SELECT COUNT(*) FROM TABLES " \
        "WHERE TABLE_SCHEMA = '" schema "'")
    spawk_first(data)
    tcnt = data[1] + 0

    spawk_select("SELECT COUNT(*) FROM COLUMNS " \
        "WHERE TABLE_SCHEMA = '" schema "'")
    spawk_first(data)

    print schema, tcnt, data[1] + 0
}
```
Using that kind of approach it's easy to improve the code to take some kind of option to print all the schemas of the database, instead of the given schemas. Options may be given using awk "external" awk variables, that is variables specified in the command line via `-v` awk options. The deal is: if given the `all` variable, then print all of the database schemas automatically, else read schemas from the input files or from the standard input.
```
BEGIN {
    extension("libspawk.so", "dlload")
    SPAWKINFO["database"] = "information_schema"
    if (all)
        exit(count_all())
}

{
    schematc($1)
}

function count_all(                schema) {
    spawk_select("SELECT SCHEMA_NAME FROM SCHEMATA")
    while (spawk_data(schema))
        schematc(schema[1])

    return(0)
}

function schematc(schema,                 data, tcnt) {
    spawk_select("SELECT COUNT(*) FROM TABLES " \
        "WHERE TABLE_SCHEMA = '" schema "'")
    spawk_first(data)
    tcnt = data[1] + 0

    spawk_select("SELECT COUNT(*) FROM COLUMNS " \
        "WHERE TABLE_SCHEMA = '" schema "'")
    spawk_first(data)

    print schema, tcnt, data[1] + 0
}
```
Now, if you want to run the command for all schemas in the database, just give:
```
awk -v all=1 -f schematc
```
else run the command as previously (without setting the `all` variable in the command line).

Now it's time to make the code even better. Let's say we want to count columns in a per table basis rather than count all the columns for the given schemas. We have to modify the output not to print schema, tables' count and columns' count, but rather print schema, table and columns' count, where the columns' count is for each table printed. We can then process that output with other awk programs (or any other programs) to add counts in a per schema basis, or load the output in spreadsheet forms, or process that output otherway.
```
BEGIN {
    extension("libspawk.so", "dlload")
    SPAWKINFO["database"] = "information_schema"
    if (all)
        exit(count_all())
}

{
    schematc($1)
}

function count_all(                schema) {
    spawk_select("SELECT SCHEMA_NAME FROM SCHEMATA")
    while (spawk_data(schema))
        schematc(schema[1])

    return(0)
}

function schematc(schema,          table, data) {
    spawk_select("SELECT TABLE_NAME FROM TABLES " \
        "WHERE TABLE_SCHEMA = '" schema "'")
    while (spawk_data(table)) {
        spawk_select("SELECT COUNT(*) FROM COLUMNS " \
        "WHERE TABLE_SCHEMA = '" schema \
            "' AND TABLE_NAME = '" table[1] "'")
        spawk_first(data)
        print schema, table[1], data[1] + 0
    }
}
```
In order to run the program old fashioned, that is printing counts in a per schema basis rather than printing counts for each table of the given schemas, let's add an "option" using the `schema` awk variable. The deal is: if the `schema` awk variable is set, then the counts are printed in a per schema basis, else print counts for each table in the given schemas:
```
BEGIN {
    extension("libspawk.so", "dlload")
    SPAWKINFO["database"] = "information_schema"
    if (all)
        exit(count_all())
}

{
    schematc($1)
}

function count_all(                schema) {
    spawk_select("SELECT SCHEMA_NAME FROM SCHEMATA")
    while (spawk_data(schema))
        schematc(schema[1])

    return(0)
}

# We have to rename the first argument as `sch' in
# order to avoid conflict with the "global" `schema'
# option variable.

function schematc(sch,          table, data, \
    tcnt, ccnt) {
    spawk_select("SELECT TABLE_NAME FROM TABLES " \
        "WHERE TABLE_SCHEMA = '" sch "'")
    while (spawk_data(table)) {
        tcnt++
        spawk_select("SELECT COUNT(*) FROM COLUMNS " \
        "WHERE TABLE_SCHEMA = '" sch \
            "' AND TABLE_NAME = '" table[1] "'")
        spawk_first(data)
        if (schema)
            ccnt += data[1]
        else
            print sch, table[1], data[1] + 0
    }

    if (schema)
        print sch, tcnt + 0, ccnt + 0
}
```
Now the program is complete, using two external (command line) variables as "options": if set, the `all` variable, then all of the database schemas are processed automatically, or else the schemas will be readed from the input files or the standard input. If set, the `schema` variable, then the output is in the form: schema, tables' count, coloumns' count. If not set, the `schema` variable, then the output is in the form: schema, table, columns' count, where columns' count refers to each table printed, not the corresponding schema.

To make the program fully functional, we'll embrace all those functions and options in the `schematc` shell script:
```
#!/bin/sh

progname=`basename $0`

errs=
all=0
schema=0

while getopts ":as" arg
do
    case "${arg}" in
    a)
        all=1
        ;;
    s)
        schema=1
        ;;
    \?)
        echo "${progname}: ${OPTARG}: illegal option" >&2
        errs=yes
        ;;
    esac
done
if [ -n "${errs}" ]; then
    echo "usage: ${progname} [-a] [-s] [files...]" >&2
    exit 1
fi

shift `expr ${OPTIND} - 1`
if [ \( ${all} -ne 0 \) -a \( $# -ne 0 \) ]; then
    echo "${progname}: no input files allowed with -a option" >&2
    exit 1
fi

exec awk -v all=${all} -v schema=${schema} 'BEGIN {
    q = "'\''"
    extension("libspawk.so", "dlload")
    SPAWKINFO["database"] = "information_schema"
    if (all)
        exit(count_all())
}

{
    schematc($1)
}

function count_all(                schema) {
    spawk_select("SELECT SCHEMA_NAME FROM SCHEMATA")
    while (spawk_data(schema))
        schematc(schema[1])

    return(0)
}

function schematc(sch,          table, data, \
    tcnt, ccnt) {
    spawk_select("SELECT TABLE_NAME FROM TABLES " \
        "WHERE TABLE_SCHEMA = " q sch q)
    while (spawk_data(table)) {
        tcnt++
        spawk_select("SELECT COUNT(*) FROM COLUMNS " \
        "WHERE TABLE_SCHEMA = " q sch q \
            " AND TABLE_NAME = " q table[1] q)
        spawk_first(data)
        if (schema)
            ccnt += data[1]
        else
            print sch, table[1], data[1] + 0
    }

    if (schema)
        print sch, tcnt + 0, ccnt + 0
}' "$@"
```
Now, to run the command for all schemas in a per table count basis, give:
```
schematc -a
```
To run for all schemas in a per schema count basis:
```
schematc -as
```
To run the command for schemas specified in `schema1` and `schema2` files, counting columns for each distinct table:
```
schematc schema1 schema2
```
To run the same command but count for schemas rtaher than for tables:
```
schematc -s schema1 schema2
```
It's so easy to use SPAWK!

[Top](Tutorial.md)

<u>
<h1>Data Types' Statistics</h1>
</u>

Now it's time to produce some statistics on database's columns' data types. Let's say we want to produce a report concerning the columns' datatypes. The report will be sorted by schema (database), table and column name. Right to the code:
```
BEGIN {
    extension("libspawk.so", "dlload")
    SPAWKINFO["database"] = "information_schema"
    exit(visit_schemas())
}

function visit_schemas(              schema) {
    spawk_select("SELECT SCHEMA_NAME FROM SCHEMATA " \
        "ORDER BY SCHEMA_NAME")
    while (spawk_data(schema))
        visit_tables(schema[1])
}

function visit_tables(schema,        table) {
    spawk_select("SELECT TABLE_NAME FROM TABLES " \
        "WHERE TABLE_SCHEMA = '" schema \
        "' ORDER BY TABLE_NAME")
    while (spawk_data(table))
        visit_table(schema, table[1])
}

function visit_table(schema, table,         column) {
    spawk_select("SELECT COLUMN_NAME, DATA_TYPE " \
        "FROM COLUMNS WHERE TABLE_SCHEMA = '" schema \
        "' AND TABLE_NAME = '" table \
        "' ORDER BY COLUMN_NAME")
    while (spawk_data(column))
        print schema, table, column[0]
}
```
Saving the script in `datatypes`, run:
```
awk -f datatypes
```
e voila!

We could, of course, write a much simpler program:
```
BEGIN {
    extension("libspawk.so", "dlload")
    SPAWKINFO["database"] = "information_schema"
    spawk_select("SELECT TABLE_SCHEMA, TABLE_NAME, " \
        "COLUMN_NAME, DATA_TYPE FROM COLUMNS " \
        "ORDER BY TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME")
    while (spawk_data(column))
        print column[0]

    exit(0)
}
```
but the previous version is more flexible. However, even that simplified version can easily be improved. Let's say we want to provide the schemas, or the tables, or even the columns for which to run the report. To do so we'll use an "external" variable named `type` which takes 3 values: `schema` for processing schemas, `table` for processing tables, `column` for processing only specific columns and, finally, null (empty) for processing all the database columns (default).
```
BEGIN {
    # If `type` external variable is not specified, then process
    # all of the database columns. That will be done in the the
    # END awk section during `exit`.
    if (type == "")
        exit(0)
}

# Read input for schemas/tables/columns. Each object can be
# specified either in simple form, e.g. person, tax, etc, or
# in complex form, e.g. payroll.person for the "person" table
# in the "payroll" schema, person.name for the "name" column
# in the "person" table (of all schemas), or payroll.person.zip
# for the "zip" column of "person" table of "payroll" schema.

{
    ok[$1]
}

END {
    extension("libspawk.so", "dlload")
    SPAWKINFO["database"] = "information_schema"
    spawk_select("SELECT TABLE_SCHEMA, TABLE_NAME, " \
        "COLUMN_NAME, DATA_TYPE FROM COLUMNS " \
        "ORDER BY TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME")
    while (spawk_data(column)) {
        print_it = 0
        if (type == "")
            print_it = 1
        else if ((type == "schema") && (column[1] in ok))
            print_it = 1
        else if (type == "table") {
            for (i in ok) {
                if (((n = split(i, stc, ".")) <= 1) && \
                    (column[2] == i)) {
                    print_it = 1
                    break
                } else if ((n == 2) && (column[1] == stc[1]) && \
                    (column[2] == stc[2])) {
                    print_it = 1
                    break
                }
            }
        } else if (type == "column") {
            for (i in ok) {
                if (((n = split(i, stc, ".")) <= 1) && \
                    (column[3] == i)) {
                    print_it = 1
                    break
                } else if ((n == 2) && (column[2] == stc[1]) && \
                    (column[3] == stc[2])) {
                    print_it = 1
                    break
                } else if ((n == 3) && (column[1] == stc[1]) && \
                    (column[2] == stc[2]) && (column[3] == stc[3])) {
                    print_it = 1
                    break
                } 
            }
        }

        if (print_it)
            print column[0]
    }
}
```
Let's do some code refinement. First, why not use "." as the default awk input field delimiter? That simple code poke will simplify lot of things. Second, I think it's a bad thing to split again and again. It's much easier (for the machine) to incorporate some regular expression facilities using awk. Last, but not least, some formal refinement is essential.
```
BEGIN {
    FS = "."
    if (type == "") {
        ok["^.*$"]
        exit(0)
    }
}

# Use "@" for schema/table/column delimiter in order
# not to confuse with the "." special regular expression
# meaning.

{
    if (type == "column") {
        if (NF == 1)
            obj = ".*@.*@" $1 "@.*"
        else if (NF == 2)
            obj = ".*@" $1 "@" $2 "@.*"
        else if (NF == 3)
            obj = $1 "@" $2 "@" $3 "@.*"
    }
    else if (type == "table") {
        if (NF == 1)
            obj = ".*@" $1 "@.*@.*"
        else if (NF == 2)
            obj = $1 "@" $2 "@.*"
    }
    else if (type == "schema") {
        if (NF == 1)
            obj = $1 "@.*@.*@.*"
    }

    if (obj)
        ok["^" obj "$"]
    else
        print $0 ": input syntax error" >"/dev/stderr"
}

END {
    extension("libspawk.so", "dlload")
    SPAWKINFO["database"] = "information_schema"
    SPAWKINFO["OFS"] = "@"
    spawk_select("SELECT TABLE_SCHEMA, TABLE_NAME, " \
        "COLUMN_NAME, DATA_TYPE FROM COLUMNS " \
        "ORDER BY TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME")
    while (spawk_data(column)) {
        for (i in ok) {
            if (column[0] ~ i) {
                print column[1], column[2], column[3], column[4]
                break
            }
        }
    }
}
```
Now run the command for all columns of all tables in all database schemas:
```
awk -f datatypes
```
To run the command for only "mysql" schema, give:
```
awk -v type=schema -f datatypes
```
and provide "mysql" to the standard input.

To provide data types' statistics for the "name" named columns, just give:
```
awk -v type=column -f datatypes
```
and provide "name" as input.

For datatypes' statistics for all tables named "user" or "host" in all schemas, give:
```
awk -v type=table -f datatypes
```
and provide "user" and "host" as input.

Now, let's introduce the `level` external variable for printing statistics in database, schema, table, or column level. If the `level` variable is null (empty), then a global datatypes' statistics report is printed. If the `level` variable is valued "schema", the datatypes' statistics will be printed in a per schema basis. If the `level` variable is valued "table", then the report is printed in a per table basis, whereas if the `level` variable is "column" valued, then the report is printed in a per column basis. It sounds more complicated than it is:
```
...
END {
    extension("libspawk.so", "dlload")
    SPAWKINFO["database"] = "information_schema"
    SPAWKINFO["OFS"] = "@"
    spawk_select("SELECT TABLE_SCHEMA, TABLE_NAME, " \
        "COLUMN_NAME, DATA_TYPE FROM COLUMNS " \
        "ORDER BY TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME")
    while (spawk_data(column)) {
        for (i in ok) {
            if (column[0] ~ i) {
                if (level == "")
                    count[column[4]]++
                else if (level == "schema")
                    count[column[1] " " column[4]]++
                else if (level == "table")
                    count[column[1] " " column[2] " " column[4]]++
                else if (level == "column")
                    print column[1], column[2], column[3], column[4]

                break
            }
        }
    }

    if (level != "column") {
        n = asorti(count, tnuoc)
        for (i = 1; i <= n; i++)
            print tnuoc[i], count[tnuoc[i]]
    }
}
```
Using the so called "[indirect functions](http://awk.info/?Funky)" of version 3.2 of gawk, we can make the code even better:
```
...
END {
    do_dount[""] = "count_database"
    do_count["schema"] = "count_schema"
    do_count["table"] = "count_table"
    do_count["column"] = "count_column"

    extension("libspawk.so", "dlload")
    SPAWKINFO["database"] = "information_schema"
    SPAWKINFO["OFS"] = "@"
    spawk_select("SELECT TABLE_SCHEMA, TABLE_NAME, " \
        "COLUMN_NAME, DATA_TYPE FROM COLUMNS " \
        "ORDER BY TABLE_SCHEMA, TABLE_NAME, COLUMN_NAME")
    while (spawk_data(column)) {
        for (i in ok) {
            if (column[0] ~ i) {
                @do_count[level]()
                break
            }
        }
    }

    if (level != "column") {
        n = asorti(count, tnuoc)
        for (i = 1; i <= n; i++)
            print tnuoc[i], count[tnuoc[i]]
    }
}

function count_all() {
    count[column[4]]++
}

function count_schema() {
    count[column[1] " " column[4]]++
}

function count_table() {
    cunt[column[1] " " column[2] " " column[4]]++
}

function count_column() {
    print column[1], column[2], column[3], column[4]
}
```

[Top](Tutorial.md)

<u>
<h1>User/Files Statistics</h1>
</u>
<p>
We are going to develop a MySQL based application in order to monitor user/files statistics in Linux/UNIX platforms. To be more specific, given a UNIX/Linux based system, we are going to develop a SPAWK program that counts all the files of the host system in regular time intervals and keep the per user counts in database records. The schema will be called "ufstats" and will contain the following tables:<br>
<pre><code>CREATE DATABASE ufstats;<br>
USE ufstats;<br>
<br>
-- Table "cuser" stands for [c]computer [user] and will hold<br>
-- a record for each user found in the password file. If a<br>
-- user is not found during a files count cycle (scan), then<br>
-- that user will be marked as deleted by means of the "cu_delete"<br>
-- column; that column will be left NULL for users found in the<br>
-- password file, while for users not found in the password file<br>
-- the date/time of scan will be filled in "cu_delete" column. The<br>
-- "cu_info" column contains the corresponding password file info field.<br>
<br>
CREATE TABLE cuser (<br>
    cu_login CHARACTER(32) NOT NULL PRIMARY KEY,<br>
    cu_info CHARACTER(128) NOT NULL,<br>
    cu_delete DATE<br>
) ENGINE = "INNODB";<br>
<br>
-- Table "fscan" holds a record for each file scan executed. The<br>
-- "fs_start" column is the begin date/time of the scan and will<br>
-- be used as a primary key as there will be references to that<br>
-- table, whereas "fs_end" holds the date/time of the scan end.<br>
-- Column "fs_end" may accept NULL values while the scan is in<br>
-- progress.<br>
<br>
 TABLE fscan (<br>
    fs_start DATETIME NOT NULL,<br>
    PRIMARY KEY USING BTREE (<br>
        fs_start<br>
    ),<br>
    fs_end DATETIME<br>
) ENGINE = "INNODB";<br>
<br>
-- Table "fcount" holds the per user files' count statistics for<br>
-- each executed scan. The table has only three columns, namely<br>
-- the "fc_scan" column which is the scan's date/time (primary<br>
-- key of the "fscan" table), the "fc_user" column which is,<br>
-- of course, the login name of each user (primary key of the<br>
-- "cuser" table) and, finally, the file count for that scan<br>
-- and user.<br>
<br>
CREATE TABLE fcount (<br>
    fc_scan DATETIME NOT NULL, <br>
    FOREIGN KEY (<br>
        fc_scan<br>
    ) REFERENCES fscan (<br>
        fs_start<br>
    ),<br>
    fc_user CHARACTER(32) NOT NULL,<br>
    FOREIGN KEY (<br>
        fc_user<br>
    ) REFERENCES cuser (<br>
        cu_login<br>
    ),<br>
    fc_files NUMERIC(9) NOT NULL<br>
) ENGINE = "INNODB";<br>
</code></pre>
<p>
Save the commands above to the file <code>create.sch</code>. To create (or recreate) the database we can simply run:<br>
</p>
<pre><code>mysql -A &lt;create.sch<br>
</code></pre>
<p>
But why not to use SPAWK in order to create (or recreate) the schema?<br>
</p>
<pre><code># The following program reads a DDL script to create teh "ufstats"<br>
# database. If the database exists, then the program exits with<br>
# non zero exit status; if we want to drop the existing database<br>
# and then recreate the database we must define the (external)<br>
# variable drop, e.g.<br>
#<br>
#    awk -v drop=yes -f create.awk create.sch<br>
<br>
BEGIN {<br>
    SPAWKINFO["program"] = "create"<br>
    extension("libspawk.so", "dlload")<br>
    if (drop)<br>
        spawk_update("DROP DATABASE IF EXISTS ufstats", 2)<br>
<br>
    spawk_select("SHOW DATABASES LIKE 'ufstats'")<br>
    if (spawk_first())<br>
        exit(errmsg("ufstats: database exists"))<br>
}<br>
<br>
# Skip SQL comments.<br>
<br>
/^--/ {<br>
    next<br>
}<br>
<br>
# Skip empty lines.<br>
<br>
/^[ \t]*$/ {<br>
    next<br>
}<br>
<br>
# If a line ends with a ";" character, then execute the current<br>
# query. Current query is the commands already sent to the server.<br>
<br>
/;[ \t]*$/ {<br>
    sub("[ \\t]*;[ \\t]*$", "")<br>
    if (spawk_update($0))<br>
        spawk_update("ROLLBACK WORK", 2)<br>
    else<br>
        spawk_update("COMMIT WORK", 2)<br>
<br>
    next<br>
}<br>
        <br>
# Normal line. Send that line to the server to be executed later<br>
# when the ";" character shows up.<br>
<br>
{<br>
    spawk_query($0)<br>
}<br>
<br>
# Function "errmsg" is a utility function to print error messages.<br>
# For code density reasons, the function returns non zero.<br>
<br>
function errmsg(msg) {<br>
    print SPAWKINFO["program"] ": " msg &gt;"/dev/stderr"<br>
    return(2)<br>
}<br>
</code></pre>
<p>
Given that the above awk script is saved in a file named <code>create.awk</code>, we can simply run:<br>
</p>
<pre><code>awk -v drop=yes -f create.awk ufstats.sch<br>
</code></pre>
<p>
to recreate the <code>ufstats</code> database.<br>
</p>
<p>
After loading the schema in the database, we're now ready to write the SPAWK application's code for the file system scanner. After doing some initialization work in the <code>BEGIN</code> section, we start an infinite loop running the <code>find</code> UNIX command again and again. After each scan we sleep for a time interval specified by the <code>interval</code> (external) variable and then run the <code>find</code> command again. For each scan we add a "fscan" record in the database holding the scan's start and end date/times. During each scan we're also update the "cuser" table that holds a record for each user found in the password file. After each scan we're inserting the computed files' counts in the database (one record for every user owning at least one file), commit the transaction and proceed to the next scan. Enjoy!<br>
</p>
<pre><code>BEGIN {<br>
    SPAWKINFO["program"] = "fscan"<br>
    SPAWKINFO["database"] = "ufstats"<br>
    extension("libspawk.so", "dlload")<br>
    set_interval()<br>
    cmd = "find / -type f -ls"<br>
    user_file = "/etc/passwd"<br>
    while (fscan())<br>
        system("sleep " interval)<br>
}<br>
<br>
# Function "set_interval" is an initialization function to set<br>
# the time interval between consequent scans. The time interval<br>
# is given in seconds (default 1 hour) and will be measured after<br>
# each scan end using UNIX "sleep" command.<br>
<br>
function set_interval() {<br>
    if (!interval)<br>
        interval = 3600<br>
    else if (interval !~ /^[ \t]*[0-9][0-9]*[ \t]*$/)<br>
        exit(errmsg(interval ": invalid time interval"))<br>
    else<br>
        interval += 0<br>
<br>
    # If the (external) variable "monitor" is set, then<br>
    # various messages will be printed to the standard<br>
    # output.<br>
<br>
    if (monitor)<br>
        print interval<br>
}<br>
<br>
# Function "fscan" is the core function which does all the job. It<br>
# starts by reading the password file and updating the "cuser"<br>
# table. Then it inserts an "open" "fscan" record for the current<br>
# scan; an "fscan" record considered as "open" while its "fs_end"<br>
# column is NULL valued. The "fs_end" column will be filled with<br>
# the scan's end date/time when the scan ends. After inserting<br>
# the "fscan" record, the function proceeds to the files' counting<br>
# process reading the "find" UNIX command output. For each file<br>
# found it increases the "count" array for the owner of the file.<br>
# After scanning the file system ("find" end) the function proceeds<br>
# in "fcount" inserts for each user in the "count" array. Then it<br>
# commits the transaction and returns.<br>
<br>
function fscan(         start, end, count, stat, login, err) {<br>
    # The "scan_users" function is the first step in file<br>
    # count. That function scans the password file and<br>
    # updates the "cuser" table.<br>
<br>
    scan_users(start = strftime("%Y%m%d%H%M%S", systime()))<br>
    if (monitor) {<br>
        printf start "-"<br>
        fflush("/dev/stdout")<br>
    }<br>
<br>
    # Insert a "fscan" record for the current date/time. The<br>
    # "end" column value will be filled when that scan ends.<br>
    # If a "fscan" record with the same "start" column value<br>
    # already exists, then the time interval is too small<br>
    # and the command respawns too rapidly; ignore that.<br>
<br>
    if ((err = spawk_update("INSERT INTO fscan (fs_start) " \<br>
        "VALUES (" start ")")) == 1062) {<br>
        errmsg(cmd ": command spawning too fast (" start ")")<br>
        return(1)<br>
    }<br>
<br>
    # Check the "spawk_update" return value. The "duplicate<br>
    # key" (error code 1062) have been checked already. Now<br>
    # if the "spawk_update" return value is non zero, then<br>
    # we are facing some other erroneus situation and the<br>
    # program must be aborted.<br>
 <br>
    if (err)<br>
        exit(errmsg("insert fscan failed (" spawk_error() ")"))<br>
<br>
    # Now run the "find" command and read the results. Given<br>
    # the "-ls" option, the user is at the 5th field. Accumulate<br>
    # files' counts in array "count".<br>
<br>
    while ((stat = (cmd |&amp; getline)) &gt; 0)<br>
        count[$5]++<br>
<br>
    # Check if the "find" command terminated normally. If not,<br>
    # then exit with non zero exit status.<br>
<br>
    if (stat &lt; 0)<br>
        exit(errmsg(cmd ": " ERRNO))<br>
<br>
    # We must close (terminate) the "find" command! If<br>
    # not closed, all subsequent cycles will return zero<br>
    # and will not produce no results at all.<br>
 <br>
    close(cmd)<br>
<br>
    # We're interesting on the time consumed in file<br>
    # system scans. Now is the time to mark the end<br>
    # date/time of the scan. We'll later fill that<br>
    # as the scan's "fs_end" column value.<br>
<br>
    end = strftime("%Y%m%d%H%M%S", systime())<br>
    if (monitor) {<br>
        print end<br>
        fflush("/dev/stdout")<br>
    }<br>
<br>
    # Now insert the files' counts for that scan in the<br>
    # database. Some "find" results may contain numbers<br>
    # as user login names; those are UNIX user ids for<br>
    # users not found in the password file and probably<br>
    # there are no such entries in our "cuser" table<br>
    # also; insert new users using those numeric ids<br>
    # as login names in order not to loose those counts.<br>
<br>
    for (login in count) {<br>
        # If the login not found in the "cuser" table,<br>
        # insert it as "unknown user", setting the<br>
        # "cu_delete" column value to the date/time<br>
        # of the current scan.<br>
<br>
        spawk_select("SELECT cu_login FROM cuser " \<br>
            "WHERE cu_login = '" login "'")<br>
        if (!spawk_first())<br>
            insert_user(login, "** Unknown User **", start)<br>
<br>
        spawk_update("INSERT INTO fcount " \<br>
            "(fc_scan, fc_user, fc_files) VALUES (" \<br>
            start ",'" login "'," count[login] ")", 2)<br>
    }<br>
<br>
    # Now it's time to "close" the scan, setting the "fs_end"<br>
    # column value to the date/time when the current scan ended.<br>
    # After "closing" the scan, commit transaction.<br>
<br>
    spawk_update("UPDATE fscan SET fs_end = " end \<br>
        " WHERE fs_start = " start, 2)<br>
    spawk_update("COMMIT WORK", 2)<br>
    return(1)<br>
}<br>
                <br>
# Function "scan_user" scans the "/etc/passwd" file and inserts<br>
# the users in the database. Before doing so, we first set the<br>
# "cu_delete" value to the current scan's date/time. During the<br>
# password file scan we reset the "cu_delete" to NULL; that way<br>
# the users not found in the password file for the current scan<br>
# will be left with the "cu_delete" column filled.<br>
<br>
function scan_users(dt,         stat, fs1) {<br>
    spawk_update("UPDATE cuser SET cu_delete = " dt, 2)<br>
    # Temporarily, we set the FS to ":" in order to<br>
    # read the password file the easy way (awk).<br>
<br>
    fs1 = FS<br>
    FS = ":"<br>
    while ((stat = (getline &lt;user_file)) &gt; 0) {<br>
        if (insert_user($1, $5, dt))<br>
            spawk_update("UPDATE cuser " \<br>
                "SET cu_delete = NULL " \<br>
                "WHERE cu_login = '" login "'", 2)<br>
        else<br>
            exit(errmsg($1 ": cuser insert failed: " \<br>
                spawk_error()))<br>
    }<br>
<br>
    if (stat &lt; 0)<br>
        exit(errmsg(user_file ": " ERRNO))<br>
<br>
    # Close the password file and reset the FS separator<br>
    # to its default value in order to read the "find"<br>
    # output.<br>
<br>
    close(user_file)<br>
    FS = fs1<br>
}<br>
<br>
# Function "insert_user" is used to add "cuser" records in the<br>
# database. The "cuser" column values ("cu_login", "cu_info" and<br>
# "cu_delete") are passed as function's arguments.<br>
<br>
function insert_user(login, info, dt,       err) {<br>
    # If the user already exists in the database, then<br>
    # it's ok, but return non zero, else return zero.<br>
    # If the user fails to insert for any other reason,<br>
    # then that is a fatal error and exit non zero.<br>
<br>
    if ((err = spawk_update("INSERT INTO cuser " \<br>
        "(cu_login, cu_info, cu_delete) " \<br>
        "VALUES ('" login "','" spawk_string(info) \<br>
        "'," dt ")")) == 1062)<br>
        return(1)<br>
<br>
    if (err == 0)<br>
        return(1)<br>
<br>
    exit(errmsg(login ": insert user failed (info = " info \<br>
        ", delete = " dt "): " spawk_error()))<br>
}<br>
<br>
# Function "errmsg" is a tiny utility function to print error<br>
# messages. For code density reasons it returns non zero.<br>
<br>
function errmsg(msg) {<br>
    print SPAWKINFO["program"] ": " msg &gt;"/dev/stderr"<br>
    return(2)<br>
}<br>
</code></pre>
<p>
The above program may be given in a daemon like manner to run in a regular time basis producing and storing results in the database:<br>
</p>
<pre><code>nohup awk -v monitor=yes -v interval=300 -f fscan.awk &gt;fscan.out 2&gt;fscanf.err &amp;<br>
</code></pre>
<p>
The program will run from now on and the files' count data will be collected and stored in the database about every 5 minutes. Now let's produce some statistics concerning in user files' counts. The following program may be run at any given time to process the data already stored in the database and will produce files' count statistics for a given time period:<br>
<p>
<pre><code># The following program processes "fcount" data and produces<br>
# per user file count statistics for a given time period.<br>
# If given the "from" (external) variable, then the data<br>
# from the given date/time are processed. You can also up<br>
# bound the time interval by setting the "to" (external)<br>
# variable. If none of those variables are set, then all<br>
# the "fcount" data will be processed.<br>
#<br>
# The output format is one line for each user processed and<br>
# contains the following pipe ("|") separated fields:<br>
#<br>
#    minimum date/time processed<br>
#    maximum date/time processed<br>
#    number of scans processed<br>
#    user login name<br>
#    user info<br>
#    average number of files owned by the user<br>
<br>
BEGIN {<br>
    OFS = "|"<br>
    OFMT = "%.0f"    <br>
    SPAWKINFO["program"] = "ufstats"<br>
    SPAWKINFO["database"] = "ufstats"<br>
    extension("libspawk.so", "dlload")<br>
    spawk_query("SELECT DATE_FORMAT(fs_start, '%Y%m%d%H%i%S') " \<br>
        "FROM fscan WHERE 1 = 1")<br>
<br>
    if (from)<br>
        spawk_query(" AND fs_start &gt;= " from)<br>
<br>
    if (to)<br>
        spawk_query(" AND fs_start &lt; " to)<br>
<br>
    if (spawk_update())<br>
        exit(errmsg("invalid from/to"))<br>
<br>
    spawk_results()<br>
    min_scan = 99999999999999<br>
    max_scan = 00000000000000<br>
    while (spawk_data(fscan)) {<br>
        if (fscan[1] &lt; min_scan)<br>
            min_scan = fscan[1]<br>
<br>
        if (fscan[1] &gt; max_scan)<br>
            max_scan = fscan[1]<br>
<br>
        times++<br>
        spawk_select("SELECT fc_user, fc_files " \<br>
            "FROM fcount WHERE fc_scan = " fscan[1])<br>
        while (spawk_data(fcount))<br>
            total[fcount[1]] += fcount[2]<br>
    }<br>
<br>
    if (times) {<br>
        for (user in total)<br>
            print min_scan, max_scan, times, user, \<br>
                user_name(user), total[user] / times<br>
    }<br>
    else<br>
        errmsg("no data")<br>
<br>
    exit(0)<br>
}<br>
<br>
function user_name(login,            user) {<br>
    spawk_select("SELECT cu_info FROM cuser " \<br>
        "WHERE cu_login = '" login "'")<br>
    spawk_first(user)<br>
    return(user[1])<br>
}<br>
<br>
function errmsg(msg) {<br>
    print SPAWKINFO["program"] ": " msg &gt;"/dev/stderr"<br>
    return(2)<br>
}<br>
</code></pre>
<p>
To print total files' count per user statistics for, let's say July 2009:<br>
</p>
<pre><code>awk -v from=20090701000000 -v to=20090801000000 -f ufstats.awk<br>
</code></pre>
<p>
or for the whole 2008 year:<br>
</p>
<pre><code>awk -v from=20080101000000 -v to=20090101000000 -f ufstats.awk<br>
</code></pre>

<a href='Tutorial.md'>Top</a>

<u>
<h1>SPAWK Playing Chess</h1>
</u>
<p>
Given a database of chess games, we're going to develop SPAWK programs to extract useful information otherwise difficult to extract. The database schema is named <b><i>chess</i></b> and is a simplified version of the real data dictionary; however, we preserve most of the games' information in order to demonstrate SPAWK boundless forces. Here follows the <i>chess</i> data dictionary:<br>
</p>
<pre><code># Database "chess" is about archiving of all chess games.<br>
# Games are organized in tournaments where each tournament<br>
# may belong to a more general schedule. Basic tables are<br>
# "player" where each player has a row, "tounament" where<br>
# there is a correspondig row for each tournament, and<br>
# "game" where each palyed game has a corresponding row.<br>
# Another very important table has to do with the movements<br>
# played in each game. That table is named "movement" and<br>
# contains all the movements of all the games played ever.<br>
<br>
CREATE DATABASE chess;<br>
USE chess;<br>
<br>
# Table "player" holds a single row (record) for each player<br>
# ever played an official game. There exists a lot more amount<br>
# of information to store for each player (nationality etc),<br>
# but it's out of the scope of that example.<br>
#<br>
# "Player" Columns<br>
# ----------------<br>
# All of the "player"'s columns names start with "pl_".<br>
#<br>
# The "pl_key" is a numeric key for the player to be used in<br>
# other tables referring "player" rows.<br>
#<br>
# The "pl_name" column holds the player's name and among with<br>
# the "pl_bdate" column constitute a unique key.<br>
<br>
<br>
CREATE TABLE player (<br>
        pl_key          NUMERIC(9) NOT NULL,<br>
        PRIMARY KEY pl_key USING HASH (<br>
                pl_key<br>
        ),<br>
        pl_name         CHARACTER(40) NOT NULL,<br>
        pl_bdate        DATE NOT NULL,<br>
        UNIQUE INDEX pl_name USING BTREE (<br>
                pl_name,<br>
                pl_bdate<br>
        )<br>
)<br>
ENGINE = "InnoDB";<br>
<br>
# Each game will be intergrated in a "tournament". So "tournament"<br>
# table holds related sets of games. Tournaments may belong to<br>
# other (larger) tournaments.<br>
#<br>
# "Tournament" Columns<br>
# --------------------<br>
# All of the "tournament"'s columns names start with "tr_".<br>
#<br>
# The "tr_key" is a numeric primary key for the tournament to be<br>
# references from aother tables.<br>
#<br>
# The "tr_desc" is a description of the tournament.<br>
#<br>
# The "tr_begin" column holds the date when the tournament begins,<br>
# while the "tr_end" column holds the end date of the tournament.<br>
# Each of those two columns may hold null values in order to<br>
# consider as tournaments various sets of games, e.g. "named" games,<br>
# etc.<br>
#<br>
# The "tr_other" column is filled in tournaments that belong to<br>
# other (larger) tournaments. In those cases the ancestor's<br>
# key is filled in. When there is no ancestor tournament, then<br>
# that column's value must be set to null.<br>
<br>
CREATE TABLE tournament (<br>
        tr_key          NUMERIC(9) NOT NULL,<br>
        PRIMARY KEY tr_key USING HASH (<br>
                tr_key<br>
        ),<br>
        tr_desc         CHARACTER(128) NOT NULL,<br>
        UNIQUE INDEX tr_desc USING BTREE (<br>
                tr_desc<br>
        ),<br>
        tr_begin        DATE,<br>
        INDEX tr_begin USING BTREE (<br>
                tr_begin<br>
        ),<br>
        tr_end          DATE,<br>
#       Column "tr_other" references another tournament which<br>
#       is the ancestor of that one. Because of null values<br>
#       involved in most of the rows (not every tournament has<br>
#       an ancestor tournament), the index constraint is not<br>
#       a foreign key, but a btree index is used.<br>
        tr_other        NUMERIC(9),<br>
        INDEX USING BTREE (<br>
                tr_other<br>
        )<br>
)<br>
ENGINE = "InnoDB";<br>
<br>
# Table "game" holds a row for each played game. There always<br>
# two players involved in a game, and usually a game belongs<br>
# to a tournament.<br>
#<br>
# "Game" Columns<br>
# --------------------<br>
# All of the "game"'s columns names start with "gm_".<br>
#<br>
# The "gm_key" is a numeric primary key to identify the game.<br>
# That key is referenced mainly from the "movement" rows to<br>
# add movements in a game.<br>
#<br>
# The "gm_tournament" column is the tournamnt's key of the<br>
# tournament to which that game belongs. If the game doesn't<br>
# belong to any tournament, then that column may be null valued.<br>
#<br>
# The "gm_kind" column shows the kind of the game. Among with the<br>
# "gm_minutes" column shows the type of the game, e.g. "30 Minutes<br>
# Rapid", or "5 Minutes Blitz" etc.<br>
#<br>
# The "gm_white" and "gm_black" columns shows the white and black<br>
# players involved in the game. "gm_welo" and "gm_belo" columns<br>
# are the ELO values for white and black players just before that<br>
# game.<br>
#<br>
# The "gm_begin" is the date/time of the beging of that game.<br>
# While "gm_end" date/time column is null, the game is still in<br>
# action. When the game ends, then the end date/time will be filled<br>
# in the "gm_end" column.<br>
#<br>
# The "gm_wpoints" and "gm_bpoints" columns hold the points added<br>
# to the white and black players respectively, e.g. if the white<br>
# won the game, then 10 points go to the white player and 0 to the<br>
# black player, if the game is equal, then 5 points go to each<br>
# player etc.<br>
#<br>
# The "gm_stop" column shows how the game ended (Mate, Pat, etc).<br>
<br>
CREATE TABLE game (<br>
        gm_key          NUMERIC(9) NOT NULL,<br>
        PRIMARY KEY gm_key USING HASH (<br>
                gm_key<br>
        ),<br>
        gm_tournament   NUMERIC(9),<br>
        FOREIGN KEY gm_tournament (<br>
                gm_tournament<br>
        ) REFERENCES tournament (<br>
                tr_key<br>
        ),<br>
        gm_kind ENUM (<br>
                'normal',<br>
                'blitz',<br>
                'rapid'<br>
        ) NOT NULL,<br>
        gm_minutes NUMERIC(4),<br>
        gm_white NUMERIC(9) NOT NULL,<br>
        FOREIGN KEY gm_white (<br>
                gm_white<br>
        ) REFERENCES player (<br>
                pl_key<br>
        ),<br>
        gm_welo         NUMERIC(5) NOT NULL,<br>
        gm_black NUMERIC(9) NOT NULL,<br>
        FOREIGN KEY gm_black (<br>
                gm_black<br>
        ) REFERENCES player (<br>
                pl_key<br>
        ),<br>
        gm_belo         NUMERIC(5) NOT NULL,<br>
        gm_begin        DATETIME NOT NULL,<br>
        # While the game is not yet finished, the ending date/time<br>
        # is null. The same is true for the points and the way of<br>
        # the game ended. All of these columns, though, will be<br>
        # null allowed.<br>
        gm_end          DATETIME,<br>
        gm_wpoints      NUMERIC(2),<br>
        gm_bpoints      NUMERIC(2),<br>
        gm_stop         ENUM (<br>
                'Resign',<br>
                'Mate',<br>
                'Pat'<br>
        )<br>
)<br>
ENGINE = "InnoDB";<br>
<br>
# Table "movement" is huge, conatining all the movements of all<br>
# the official games played ever. Each movement belongs, of course,<br>
# to a specific game.<br>
#<br>
# "Movement" Columns<br>
# ------------------<br>
# All of the "movement"'s columns names start with "mv_".<br>
#<br>
# The "mv_game" column references the game's primary key.<br>
# There is no foreign key constraint for economy reasons,<br>
# beacause that column is involved in a btree key among<br>
# with the "mv_movement" column which shows the rank of the<br>
# movement, e.g. 1st, 2nd, etc.<br>
#<br>
# The "mv_piece" column show the piece moved.<br>
#<br>
# The "mv_from" column shows the square from which the piece<br>
# has been moved, while the "mv_to" column shows the square<br>
# to which the piece arrived. The square notation is in the<br>
# classical style of "a1", "h7", etc.<br>
#<br>
# The "mv_raise" column holds the piece to raise a pawn arrived<br>
# to the last row. For most of the movements that column is<br>
# null valued (no raise).<br>
#<br>
# The "mv_board" columns is a very important column that holds<br>
# the board's image after that movement. There are 128 characters<br>
# in that column, grouped in cuples where each cuple is in the<br>
# form "[WB_][PNBRQK_]" where "[WB_]" stands for the color of the<br>
# piece standing in a specific board square (W: White, B: Black,<br>
# _: empty), and "[PNBRQK]" statnds for the piece kind in that<br>
# specific square (P: Pawn, N: Knight, B: Bishop, R: Rook,<br>
# Q: Queen, K: King, _: empty). The squares are ranked as 1 for<br>
# a1, 2 for b2, 3 for c3,... 9 for a2, 10 for b2,... 63 for g8<br>
# and 64 for h8. That is a "mv_board" value after the first<br>
# (white's) movement of N from b1 to c3 is something like:<br>
# "WR__WBWQWKWBWNWRWPWPWPWPWPWPWPWP____WN____..." <br>
<br>
CREATE TABLE movement (<br>
        mv_game         NUMERIC(9) NOT NULL,<br>
        mv_movement     NUMERIC(4) NOT NULL,<br>
        UNIQUE INDEX mv_key USING BTREE (<br>
                mv_game,<br>
                mv_movement<br>
        ),<br>
        mv_piece        ENUM (<br>
                'P',<br>
                'N',<br>
                'B',<br>
                'R',<br>
                'Q',<br>
                'K',<br>
                'OO',<br>
                'OOO'<br>
        ) NOT NULL,<br>
        mv_from         CHARACTER(2),<br>
        mv_to           CHARACTER(2),<br>
        mv_raise        ENUM (<br>
                'P',<br>
                'N',<br>
                'B',<br>
                'R',<br>
                'Q'<br>
        ),<br>
        mv_board        CHARACTER(128),<br>
        INDEX mv_board USING BTREE (<br>
                mv_board<br>
        )<br>
)<br>
ENGINE = "InnoDB";<br>
</code></pre>
<p>
The SQL comments start with "<code>#</code>" rather than the correct "<code>--</code>" in order to avoid bad automatic coloring from the wiki coloring mechanisms (the "<code>'</code>" characters in SQL comments cause problematic coloring). So if you copy/paste the schema, you may need to fix the comments! In my version of MySQL client (<code>mysql</code>) the "<code>#</code>" comments are acceptable.<br>
</p>
<p>
Our first chess database SPAWK program will not be an easy one. We're going to develop a program to check the validity of the data involved in various database objects. But what does "validity of the data" mean? We are going to check the following aspects:<br>
</p>
<ul><li>Infinite tournament chains<br>
</li><li>Date data<br>
</li><li>Movements/board integrity<br>
<p>
Let's grub the bull by the horns right away!<br>
</p>
<pre><code>BEGIN {<br>
        SPAWKINFO["database"] = "chess"<br>
        SPAWKINFO["program"] = "check"<br>
        extension("libspawk.so", "dlload")<br>
<br>
        # If the `all' (external) variable is set, then run checks<br>
        # for all the tournaments in the database, else tournaments'<br>
        # keys (or descriptions) of the the tournaments to be checked<br>
        # will be read from the input.<br>
<br>
        if (all)<br>
                exit(check_all_tournaments())<br>
}<br>
<br>
# If the input line is just a number, then that number is assumed<br>
# to be plain `tr_key', else the current input line is assumed to<br>
# be a regular expression (in MySQL LIKE's terms) to match tournaments'<br>
# descriptions.<br>
<br>
{<br>
        if ($0 ~ /^[ \t]*[0-9][0-9]*[ \t]*$/)<br>
                check_tournament($0 + 0)<br>
        else {<br>
                spawk_select("SELECT tr_key FROM tournament " \<br>
                        "WHERE tr_desc LIKE '" spawk_string($0) \<br>
                        "' ORDER BY tr_key")<br>
                while (spawk_data(tournament))<br>
                        check_tournament(tournament[1])<br>
        }<br>
}<br>
<br>
# Function `check_all_tournaments' selects all tournaments in the<br>
# database and check each tournament for data validity.<br>
<br>
function check_all_tournaments(                 data, errs) {<br>
        spawk_select("SELECT tr_key FROM tournament " \<br>
                "ORDER BY tr_key;")<br>
        while (spawk_data(data))<br>
                errs += check_tournament(data[1])<br>
<br>
        return(errs + 0)<br>
}<br>
<br>
# Function `check_tournament' accepts a tournament's key as its<br>
# first parameter and checks the validity of the data in the<br>
# specified tournament.<br>
<br>
function check_tournament(tr_key,                       data) {<br>
        spawk_select("SELECT tr_desc, tr_begin, tr_end, tr_other " \<br>
                "FROM tournament WHERE tr_key = " tr_key)<br>
        if (!spawk_first(data))<br>
                return(errmsg(tr_key ": tournament not found!"))<br>
<br>
        monmsg("Checking tournament \"" data[1] "\" [" tr_key "]...")<br>
<br>
        # Here comes the actual check code. For now we don't<br>
        # actually check the given tournament, but only print<br>
        # the tournament's data to the standard output.<br>
<br>
        print tr_key<br>
        monmsg()<br>
        return(0)<br>
}<br>
<br>
# The `monmsg' function is used for monitoring the check process.<br>
# If given the `monitor' (external) variable, then that value is<br>
# assumed to be a file's name and the monitor messages will be<br>
# printed to that file; usually that file is the terminal's screen<br>
# (`/dev/tty').<br>
<br>
function monmsg(msg) {<br>
        if (monitor == "") {<br>
                printf (msg ? msg : "\n") &gt;monitor<br>
                fflush(monitor)<br>
        }<br>
}<br>
<br>
# Function `errmsg' is a utility function used for error message<br>
# print. Return a non-zero number for code density reasons.<br>
<br>
function errmsg(msg) {<br>
        print SPAWKINFO["program"] ": " msg &gt;"/dev/stderr"<br>
        return(1)<br>
}<br>
</code></pre>
<p>
The program uses <code>all</code> (external) variable's value to check all the tournaments in the database (non zero <code>all</code> value), or check the tournaments provided in the input either as tournaments' keys or as tournaments' descriptions (zero <code>all</code> value). The core function is, of course, the <code>check_tournament</code> function, which returns zero for normal check and non zero when encounters severe problems. For now that function is just a place holder; we don't really check nothing in that stage of program's development; we just print the tournament's key and return zero. We're going to develop that function step by step in the next sections.<br>
<p>
We use the <code>monitor</code> (external) variable to enable/disable monitoring. When the program runs for huge numbers of tournaments, it might be useful to check the current tournament while the program is running. In that case set the <code>monitor</code> variable to a file name (usually <code>/dev/tty</code>) in order to monitor the whole process.<br>
</p></li></ul>

<u>
<h2>Checking Tournaments</h2>
</u>
<p>
Now it's time to rewrite the <code>check_tournament</code> function. That will be a repeatitive process; we're going to add a little more checks in every revision. One major aspect in the process of tounaments' checking is the tournaments' hierarchical nature. Recall that there exists a field named <code>tr_other</code> which is the ancestor tournament's key, e.g. if the <code>tr_other</code> column's value for the tournament with <code>tr_key</code> value of 1056 is 1023, that means that the 1023 tournament is the ancestor of the current tournament, or that the 1056 tournament is a sub-tournament of tournament 1023. Given the <i>chess</i> data dictionary it's clear that the sub-tournaments are of major importance in chowle project. Simultaneus games are another example of the sub-tournaments use; actually the only way to enroll a simultaneous game is to insert that game as a "sub-tournament" that is composed of separate games composing the simultaneous game.<br>
</p>
<p>
To make things easier we must define a clear policy having to do with the hierarchical tournaments' structure. When checking a tournament there comes the question of checking the sub-tournaments too or not. Another aspect is to check the ancestor tournament (if there is one, of course) or not. There are many similar questions to be answered before writing a successful check function. Before writing code it's better to list the various check phases to take for each tournament to check:<br>
</p>
<ul><li>Check the tournament's dates <code>tr_begin</code> and <code>tr_end</code> for integrity. That is to check the <code>tr_begin</code> for not to be null and, if the <code>tr_end</code> date is not null also, check that date to be a later date than the <code>tr_begin</code> date.<br>
</li><li>If there is an ancestor tournament (hyper-tournament), then check the tournaments dates relative to that tournament also.<br>
</li><li>After checking the games of the tournament, then check the sub-tournaments according to the value of the <code>sub</code> (external) variable's value. If the <code>sub</code> has a value of <code>dates</code>, then only the dates of the sub-tournaments are checked. If the <code>sub</code> value is <code>full</code>, then the sub-tournaments are fully checked recursively. If the <code>sub</code> variable is empty (not assigned a value), then only the sub-tournaments chain is cheked; that check takes place in the previous (more detailed) checks too. In case of full database check (<code>all</code> variable is set) we unset the `sub' variable in order to avoid multiple checks of the same tournaments:</li></ul>

<pre><code>BEGIN {<br>
        ...<br>
        if (all)<br>
                sub = ""<br>
        ...<br>
}<br>
</code></pre>
<p>
Before proceed in the tournament's check code, let's write a utility function to access a tournament and return the turnament's data in an array; such a function will make the final code easier to write and maintain. The function will return zero for success and non-zero for not found tournaments:<br>
</p>
<pre><code>function get_tournament(tk, data) {<br>
        delete data<br>
        spawk_select("SELECT tr_desc," \<br>
                "DATE_FORMAT(tr_begin, \"%Y%m%d\")," \<br>
                "DATE_FORMAT(tr_end, \"%Y%m%d\"), tr_other " \<br>
                "FROM tournament WHERE tr_key = " tk,<br>
                "tr_desc,tr_begin,tr_end,tr_other")<br>
        return(spawk_first(data) ? 0 : \<br>
                errmsg(tk ": tournament not found"))<br>
}<br>
</code></pre>
<p>
The above function takes as its first argument a tournament's key (<code>tr_key</code>) value and tries to access that tournament in the database. On success the tournament's column values are returned in the array passed as second argument to the function (optional) and the function returns zero, the array will be empty and the function returns non zero. Notice the second <code>spawk_select</code>'s argument. Passing a coma separated string to <code>spawk_select</code> function the returned array (tournament data) will be indexed using that string's fields instead of the classical number scheme. That's a classical utility SPAWK function.<br>
</p>
<p>
Given the <code>get_tournament</code> function it's much easier to proceed to the <code>check_tournament</code> function code. Because of later recursive calls we introduce a second argument named <code>level</code> to be passed to the <code>check_tournament</code> function. In case of recursive calls that argument will increased by one and passed to the recursive function call. That way we always know the depth of the checked tournament. The <code>monmsg</code> function must be slightly change to reflect the <code>level</code> value:<br>
</p>
<pre><code># If passed a second argument to monmsg function, then that<br>
# denotes the tournament level. The message to print will<br>
# be intended accordingly. We also drop the newline convention<br>
# because it complicates the code too much and doesn't worth<br>
# of it.<br>
<br>
function monmsg(msg, level) {<br>
        while (level-- &gt; 0)<br>
                printf "\t" &gt;monitor<br>
<br>
        printf msg &gt;monitor<br>
        fflush(monitor)<br>
}<br>
</code></pre>
<p>
Now follows the <code>check_tournament</code> code:<br>
<pre><code>function check_tournament(tk, level,            cur, oth) {<br>
        if (get_tournamet(tk, cur))<br>
                return(1)<br>
<br>
        monmsg("Checking tournament \"" data[1] "\" [" tk "]", level)<br>
        if (cur["tr_begin"] == "")<br>
                return(errmsg(tk ": null tr_begin value"))<br>
<br>
        if (cur["tr_end"] &amp;&amp; (cur["tr_end"] &lt; cur["tr_begin"]))<br>
                return(errmsg(tk ": invalid tr_begin/tr_end values"))<br>
<br>
        # If there exists a hyper-tournament for the tournament on hand,<br>
        # then check various dates between the two tournaments.<br>
<br>
        if (cur["tr_other"]) {<br>
                if (get_tournament(cur["tr_other"], oth))<br>
                        return(errmsg(tk ": hyper-tournament not found"))<br>
<br>
                if (oth["tr_begin"] &amp;&amp; (cur["tr_begin"] &lt; oth["tr_begin"]))<br>
                        return(errmsg(tk ": hyper-tournament invalid tr_begin"))<br>
<br>
                if (cur["tr_end"] &amp;&amp; oth["tr_end"] &amp;&amp; \<br>
                        (cur["tr_end" &gt; oth["tr_end"]))<br>
                        return(errmsg(tk ": hyper-tournament invalid tr_end"))<br>
        }<br>
<br>
        # We only check the tournament's games for the tournament on hand.<br>
        # If we're checking a sub-tournament, then we follow the `sub'<br>
        # variable's value in order to proceed in games' check or not.<br>
        <br>
        if (((!level) || (sub == "full")) &amp;&amp; check_the_games(tk))<br>
                return(1)<br>
<br>
        # Now, it's time to visit the sub-tournaments. We check the `sub'<br>
        # value to make quick or thorough check.<br>
<br>
        spawk_select("SELECT tr_key FROM tournament " \<br>
                "WHERE tr_other = " tk)<br>
        while (spawk_data(oth)) {<br>
                if (check_tournament(oth[1], ++level))<br>
                        return(errmsg(tk ": failed sub-tournament check"))<br>
        }<br>
<br>
<br>
        return(0)<br>
}<br>
</code></pre>
<p>
That's a second draft of the <code>check_tournament</code> function code. The code of <code>check_the_games</code> now follows:<br>
</p>
<pre><code>function check_the_games(tk,            data) {<br>
        spawk_select("SELECT gm_key FROM game " \<br>
                "WHERE gm_tournament = " tk \<br>
                " ORDER BY gm_begin")<br>
        while (spawk_data(data)) {<br>
                if (check_game(data[1]))<br>
                        return(1)<br>
        }<br>
}<br>
</code></pre>
<p>
The <code>check_game</code> code is now missing. But it seems it's time to redeploy forces. The code becomes complicated. Let's take a nap and continue later...<br>
</p>

<i>To be continued...</i>

<a href='Tutorial.md'>Top</a>

<u>
<h2>Printing Games</h2>
</u>
<p>
In that chapter we're going to develop some SPAWK programs to print games. Let's begin with a simple printing program that reads tournament keys (or description patterns) and prints some tournament's data as well as the tournament's games in date order:<br>
</p>
<pre><code>BEGIN {<br>
        extension("libspawk.so", "dlload")<br>
        SPAWKINFO["database"] = "chess"<br>
<br>
        # We set `tcl' string to be the projection list of<br>
        # the tournament columns to be printed. We use a<br>
        # variable instead of literally list the columns<br>
        # in the select clause, because there are two<br>
        # such select clauses in the program. In the<br>
        # next version of the program we're going to<br>
        # fix that by inserting a `select_tournament'<br>
        # function to select the tournaments' data.<br>
<br>
        tcl = "tr_key, tr_desc, tr_begin, tr_end, tr_other"<br>
}<br>
<br>
{<br>
        # Check input to see if it composes a plain tournament<br>
        # primary key (`tr_key'). If not, then the input line<br>
        # is considered to be a tournament's description pattern,<br>
        # e.g. "%Interzonal%", "%Cuba%" etc.<br>
<br>
        spawk_select("SELECT " tcl " FROM tournament WHERE " \<br>
                ($0 ~ /^[ \t]*[0-9][0-9]*[ \t]*$/ ? \<br>
                "tr_key = " $0 : "tr_desc LIKE '" spawk_string($0) "'"))<br>
<br>
        while (spawk_data(tournament))<br>
                print_tournament(tournament)<br>
}<br>
<br>
# Function `print_tournament' is the core of out printing program.<br>
# We pass a tournament returned data array as first argument, while<br>
# the second argument (optional) denotes the "level". That "level"<br>
# is by default zero, but in sub-tournaments it increases by one.<br>
# That "level" value will be used for proper indenting, in order<br>
# to make clear the sub-tournaments hierarchy. The indenting is<br>
# also used in games' print.<br>
<br>
function print_tournament(tournament, level,            other) {<br>
        print indent(level) tournament[0]<br>
<br>
        # By default the games of the tournament on hand are not<br>
        # printed. If we want to print the games also, then the<br>
        # (external) `games' variable must be set.<br>
<br>
        if (games)<br>
                print_games(tournament[1], level)<br>
<br>
        # Now it's time to print the sub-tournaments. Sub-tournaments<br>
        # are not printed by default. To print the sub-tournaments<br>
        # for each printed tournament, we must set the (external)<br>
        # variable `tree'.<br>
<br>
        if (tree) {<br>
                spawk_select("SELECT " tcl " FROM tournament " \<br>
                        "WHERE tr_other = " tournament[1] \<br>
                        " ORDER BY tr_begin")<br>
                while (spawk_data(other))<br>
                        print_tournament(other, level + 1)<br>
        }<br>
}<br>
<br>
# Function `print_games' gets a tournament's key (`tr_key') as<br>
# its first parameter and prints the games of that tournament<br>
# in date order. The second argument passed to `print_games' is<br>
# about sub-tournament indenting in order to get a clear picture<br>
# of the tournaments/sub-turnaments games hierarchy.<br>
<br>
function print_games(tk, level,                 game) {<br>
        spawk_select("SELECT gm_key, DATE_FORMAT(gm_begin, " \<br>
                "'%Y%m%d'), gm_white, gm_black, " \<br>
                "gm_wpoints, gm_bpoints FROM game " \<br>
                "WHERE gm_tournament = " tk \<br>
                " ORDER BY gm_begin, gm_key")<br>
        while (spawk_data(game))<br>
                print indent(level + 1) game[1], game[2], \<br>
                        player(game[3]), player(game[4])<br>
}<br>
<br>
# Function `player' is a utility function that returns the<br>
# name of the player whose key (`pl_key') value is passed<br>
# as the first (and only) parameter.<br>
<br>
function player(pk,                     data) {<br>
        spawk_select("SELECT pl_name FROM player " \<br>
                "WHERE pl_key = " pk)<br>
        spawk_first(data)<br>
        return(data[1])<br>
}<br>
<br>
# Function `indent' is a very low level utility function which,<br>
# given the "level" in tournament/sub-tournament tree, returns<br>
# a proper indent string composed of the right number of tab<br>
# characters. For efficiency reasons, the composed indent strings<br>
# are stored in the `indent_ok' array to be returned in later<br>
# calls.<br>
<br>
function indent(level,                  i, ind) {<br>
        if (level in indent_ok)<br>
                return(indent_ok[level])<br>
<br>
        for (i = level; i &gt; 0; i--)<br>
                ind = ind "\t"<br>
<br>
        return(indent_ok[level] = ind)<br>
}<br>
</code></pre>
<p>
If the above program is saved in <code>print.awk</code>, then in order to print tournaments without the games and without sub-tournaments:<br>
</p>
<pre><code>awk -f print.awk<br>
</code></pre>
<p>
To print the tournaments among with its games:<br>
</p>
<pre><code>awk -v games=yes -f print.awk<br>
</code></pre>
<p>
while to print the sub-tournaments too:<br>
</p>
<pre><code>awk -v tree=yes -v games=yes -f print.awk<br>
</code></pre>
<p>
That's SPAWK! An elegant bridge between two powerful tools: awk and SQL in full cooperation break into pieces the most difficult database tasks. Our next version of the <code>print.awk</code> program will convince you for the unbounded forces of SPAWK:<br>
</p>
<pre><code>BEGIN {<br>
        extension("libspawk.so", "dlload")<br>
        SPAWKINFO["database"] = "chess"<br>
<br>
        # We set `tcl' string to be the projection list of<br>
        # the tournament columns to be printed. We use a<br>
        # variable instead of literally list the columns<br>
        # in the select clause, because we plan to use<br>
        # the same columns' list for returned data array<br>
        # indexing; for the same reason no spaces are<br>
        # used in the `tcl' string (spaces may be used for<br>
        # readability, but the indices will be harder to<br>
        # read and use then. We could define the `tcl'<br>
        # string later, as local to the `select_tournaments'<br>
        # function, but there is no reason to do so, because<br>
        # that function will be called repeatedly and it<br>
        # will be a resource waste to push and pop that<br>
        # variable in the stack every time the function<br>
        # is called.<br>
<br>
        tcl = "tr_key,tr_desc,tr_begin,tr_end,tr_other"<br>
<br>
        # In accordance with the `tcl' tournaments' columns<br>
        # list, we define the `gcl' string for the games'<br>
        # projection columns list.<br>
<br>
        gcl = "gm_key,gm_begin,gm_white,gm_black,gm_wpoints,gm_bpoints"<br>
<br>
        # The array `score_list' is used to decode the score<br>
        # as "(1-0)", "(0-1)", "(1/2-1/2)". That list is used<br>
        # in the `score' function later.<br>
<br>
        score_list["10:0"] = "(1-0)"<br>
        score_list["0:10"] = "(0-1)"<br>
        score_list["5:5"] = "(1/2-1/2)"<br>
        score_list[":"] = ""<br>
<br>
        if (all) {<br>
                select_tournaments()<br>
                while (spawk_data(tournament))<br>
                        print_tournament(tournament)<br>
<br>
                exit(0)<br>
        }<br>
}<br>
<br>
# Skip empty input lines.<br>
<br>
/^[ \t]*$/ {<br>
        next<br>
}<br>
<br>
{<br>
        # Check input to see if it composes a plain tournament<br>
        # primary key (`tr_key'). If not, then the input line<br>
        # is considered to be a tournament's description pattern,<br>
        # e.g. "%Interzonal%", "%Cuba%" etc.<br>
<br>
        select_tournaments($0 ~ /^[ \t]*[0-9][0-9]*[ \t]*$/ ? \<br>
                "tr_key = " $0 : "tr_desc LIKE '" spawk_string($0) "'")<br>
        while (spawk_data(tournament))<br>
                print_tournament(tournament)<br>
}<br>
<br>
# Function `select_tournaments' initiates a tournaments' selection<br>
# query based on the condition passed as first (and only) argument.<br>
# If you don't pass a condition (or the condition is an empty string),<br>
# then all the tournaments will be selected. Notice the use of<br>
# `spawk_query' function calls to construct the final query according<br>
# to the condition passed.<br>
<br>
function select_tournaments(condition) {<br>
        spawk_query("SELECT " tcl " FROM tournament")<br>
        if (condition)<br>
                spawk_query(" WHERE " condition)<br>
<br>
        spawk_select(" ORDER BY tr_begin, tr_key", tcl)<br>
}<br>
<br>
# Function `print_tournament' is the core of our printing program.<br>
# We pass a tournament returned data array as first argument, while<br>
# the second argument (optional) denotes the "level". That "level"<br>
# is by default zero, but in sub-tournaments it increases by one.<br>
# That "level" value will be used for proper indenting, in order<br>
# to make clear the sub-tournaments hierarchy. The indenting is<br>
# also used in games' print.<br>
<br>
function print_tournament(tournament, level,            other) {<br>
        print indent(level) tournament[""]<br>
<br>
        # By default the games of the tournament on hand are not<br>
        # printed. If we want to print the games also, then the<br>
        # (external) `games' variable must be set.<br>
<br>
        if (games)<br>
                print_games(tournament["tr_key"], level)<br>
<br>
        # Now it's time to print the sub-tournaments. Sub-tournaments<br>
        # are not printed by default. To print the sub-tournaments<br>
        # for each printed tournament, we must set the (external)<br>
        # variable `tree'.<br>
<br>
        if (tree) {<br>
                select_tournaments("tr_other = " tournament["tr_key"])<br>
                while (spawk_data(other))<br>
                        print_tournament(other, level + 1)<br>
        }<br>
}<br>
<br>
# Function `print_games' gets a tournament's key (`tr_key') as<br>
# its first parameter and prints the games of that tournament<br>
# in date order. The second argument passed to `print_games' is<br>
# about sub-tournament indenting in order to get a clear picture<br>
# of the tournaments/sub-turnaments games hierarchy.<br>
<br>
function print_games(tk, level,                 game) {<br>
        spawk_select("SELECT " gcl " FROM game WHERE gm_tournament = " \<br>
                tk " ORDER BY gm_begin, gm_key", gcl)<br>
        while (spawk_data(game))<br>
                print indent(level + 1) game["gm_key"], \<br>
                        game["gm_begin"], player(game["gm_white"]), \<br>
                        player(game["gm_black"]), \<br>
                        score(game["gm_wpoints"] ":" game["gm_bpoints"])<br>
}<br>
<br>
# Function `player' is a utility function that returns the<br>
# name of the player whose key (`pl_key') value is passed<br>
# as the first (and only) parameter.<br>
<br>
function player(pk,                     data) {<br>
        spawk_select("SELECT pl_name FROM player " \<br>
                "WHERE pl_key = " pk)<br>
        spawk_first(data)<br>
        return(data[1])<br>
}<br>
<br>
# Function `score' is a utility function to print the score of a game.<br>
# The white and black points are passed to the function as a ":"<br>
# separated string and the score is printed as 1-0, 0-1, 1/2-1/2<br>
# according to the points passed.<br>
<br>
function score(wb) {<br>
        return (wb in score_list ? score_list[wb] : "(?)")<br>
}<br>
<br>
# Function `indent' is a very low level utility function which,<br>
# given the "level" in tournament/sub-tournament tree, returns<br>
# a proper indent string composed of the right number of tab<br>
# characters. For efficiency reasons, the composed indent strings<br>
# are stored in the `indent_ok' array to be returned in later<br>
# calls.<br>
<br>
function indent(level,                  i, ind) {<br>
        if (level in indent_ok)<br>
                return(indent_ok[level])<br>
<br>
        for (i = level; i &gt; 0; i--)<br>
                ind = ind "\t"<br>
<br>
        return(indent_ok[level] = ind)<br>
}<br>
</code></pre>
<p>
The comments may be removed for the experienced SPAWK user, as the code is very clear and comprehensible. We're also abolish the <code>all</code> facility and the empty lines' check; those aspects were only inserted to demonstrate the SPAWK forces. Some other simplifications are needed in order to decrease the output width, e.g. no <code>gm_key</code> is printed. Here is the pure SPAWK code for printing chess tournaments:<br>
</p>
<pre><code>BEGIN {<br>
        extension("libspawk.so", "dlload")<br>
        SPAWKINFO["database"] = "chess"<br>
        tcl = "tr_key,tr_desc,tr_begin,tr_end,tr_other"<br>
        gcl = "gm_white,gm_black,gm_wpoints,gm_bpoints"<br>
        score_list["10:0"] = "(1-0)"<br>
        score_list["0:10"] = "(0-1)"<br>
        score_list["5:5"] = "(1/2-1/2)"<br>
        score_list[":"] = ""<br>
}<br>
<br>
{<br>
        select_tournaments($0 ~ /^[ \t]*[0-9][0-9]*[ \t]*$/ ? \<br>
                "tr_key = " $0 : "tr_desc LIKE '" spawk_string($0) "'")<br>
        while (spawk_data(tournament))<br>
                print_tournament(tournament)<br>
}<br>
<br>
function select_tournaments(condition) {<br>
        spawk_query("SELECT " tcl " FROM tournament")<br>
        if (condition)<br>
                spawk_query(" WHERE " condition)<br>
<br>
        spawk_select(" ORDER BY tr_begin, tr_key", tcl)<br>
}<br>
<br>
function print_tournament(tournament, level,            other) {<br>
        print indent(level) tournament["tr_desc"], \<br>
                tournament["tr_begin"]<br>
        if (games)<br>
                print_games(tournament["tr_key"], level)<br>
<br>
        if (tree) {<br>
                select_tournaments("tr_other = " tournament["tr_key"])<br>
                while (spawk_data(other))<br>
                        print_tournament(other, level + 1)<br>
        }<br>
}<br>
<br>
function print_games(tk, level,                 game) {<br>
        spawk_select("SELECT " gcl " FROM game WHERE gm_tournament = " \<br>
                tk " ORDER BY gm_begin, gm_key", gcl)<br>
        while (spawk_data(game))<br>
                print indent(level + 1) player(game["gm_white"]), \<br>
                        "VS", player(game["gm_black"]), \<br>
                        score(game["gm_wpoints"] ":" game["gm_bpoints"])<br>
}<br>
<br>
function player(pk,                     data) {<br>
        if (pk in player_list)<br>
                return(player_list[pk])<br>
<br>
        spawk_select("SELECT pl_name FROM player " \<br>
                "WHERE pl_key = " pk)<br>
        spawk_first(data)<br>
        return(player_list[pk] = data[1])<br>
}<br>
<br>
function score(wb) {<br>
        return (wb in score_list ? score_list[wb] : "(?)")<br>
}<br>
<br>
function indent(level,                  i, ind) {<br>
        if (level in indent_ok)<br>
                return(indent_ok[level])<br>
<br>
        for (i = level; i &gt; 0; i--)<br>
                ind = ind "\t"<br>
<br>
        return(indent_ok[level] = ind)<br>
}<br>
</code></pre>
<p>
We're talking for about 60 lines of code to do things demanding hundreds of lines and dozens of libraries for other packages (php, python, perl, etc) to accomplish.<br>
</p>

<a href='Tutorial.md'>Top</a>

<u>
<h2>Loading Random Chess Data</h2>
</u>
<p>
In order to run the above SPAWK programs you'll need the <i>chess</i> database loaded with players', tournaments' and games' data. Probably you don't have such a database available in your local sysetm. But why bother? Let's make a test <i>chess</i> database loaded with random data. How to do such a thing? SPAWK!<br>
</p>
<pre><code>BEGIN {<br>
        extension("libspawk.so", "dlload")<br>
        SPAWKINFO["program"] = "LoadData"<br>
        SPAWKINFO["database"] = "chess"<br>
<br>
        # We'll need lot of random data here, so set the process-id<br>
        # as the seed for the random generation function.<br>
<br>
        srand(PROCINFO["pid"])<br>
        load_data()<br>
}<br>
<br>
function load_data() {<br>
        # The date bounds for all the tournaments to be loaded<br>
        # can be given (externally) using the `begin' and `end'<br>
        # dates. If not given, then set the period to a decade.<br>
<br>
        abort(set_begin_end(19980101, 20071231))<br>
<br>
        # External variable `tournaments' is the number of tournaments<br>
        # to be loaded in the database. Assume that there are about<br>
        # 1000 tournaments per year, or else 3 for every day. If not<br>
        # set, then set that number to a default value of the number<br>
        # of days multiplied by 3.<br>
<br>
        if (!tournaments)<br>
                tournaments = int(days * 3)<br>
<br>
        # Number of players involved can be given using the<br>
        # (external) variable `players' will be calculated according<br>
        # to the number of turnaments, let's say an average of<br>
        # 10 players per tournament.<br>
<br>
        if (!players)<br>
                players = tournaments * 10<br>
<br>
        if (monitor) {<br>
                print "Interval...: " begin " - " end \<br>
                        " (" days " days)" &gt;monitor<br>
                print "Players....: " players " (" bdate_min \<br>
                        " - " bdate_max ", " bdate_days ")" &gt;monitor<br>
                print "Tournaments: " tournaments &gt;monitor<br>
                fflush(monitor)<br>
        }<br>
<br>
        generate_words()<br>
        load_players()<br>
        load_tournaments()<br>
}<br>
<br>
# Function `set_begin_end' computes the lower and upper date<br>
# bounds for our data to load. That is the tournaments (and games)<br>
# to be loaded must lie between two days: `begin' and `end'.<br>
# The two variables can be set externally (via command line<br>
# -v options), but you can set only the `begin', or only the<br>
# `end' bound date, or neither of two. In those cases the<br>
# time period will be calculated to a decade passed as two<br>
# dates in arguments `from' and `to' (1998-01-01 2008-12-31).<br>
# In any case the function will set the `begin' and `end' dates<br>
# and compute the days between those two dates in the `days'<br>
# variable.<br>
<br>
function set_begin_end(from, to,                data) {<br>
        if (monitor) {<br>
                print "Setting begin/end dates" &gt;monitor<br>
                fflush(monitor)<br>
        }<br>
<br>
        if (begin &amp;&amp; end)<br>
                return(check_begin_end())<br>
<br>
        # One of the `begin', `end' variables, or both are not<br>
        # set. In that case compute the time period of the<br>
        # tournaments to be dispersed from the `from' and `to'<br>
        # arguments (usually a dedade) and use that period<br>
        # to compute the missing ends.<br>
<br>
        spawk_select("SELECT DATEDIFF(" to "," from ")")<br>
        if ((!spawk_first(data)) || (data[1] &lt; 1))<br>
                return(errmsg(from "-" to \<br>
                        ": invalid default begin/end dates"))<br>
<br>
        # If the `begin' is set, then calculate the `end' date<br>
        # by adding the days calculated. If, on the other hand,<br>
        # the `end' date is set, then calculate the `begin'<br>
        # date by subtracting the calculated days.<br>
<br>
        if (begin) {<br>
                spawk_select("SELECT DATE_FORMAT(" begin \<br>
                        " + INTERVAL " data[1] " DAY, '%Y%m%d')")<br>
                if ((!spawk_first(data)) || (data[1] == ""))<br>
                        return(errmsg(begin ": invalid begin date"))<br>
<br>
                end = data[1]<br>
        }<br>
        else {<br>
                spawk_select("SELECT DATE_FORMAT(" end \<br>
                        " - INTERVAL " data[1] " DAY, '%Y%m%d')")<br>
                if ((!spawk_first(data)) || (data[1] == ""))<br>
                        return(errmsg(end ": invalid end date"))<br>
<br>
                begin = data[1]<br>
        }<br>
<br>
        # Now check the (calculated) `begin' and `end' dates and<br>
        # compute the days between them.<br>
<br>
        return(check_begin_end())<br>
}<br>
<br>
# Function `check_begin_end' is the last step in the setting<br>
# of the `begin' and `end' dates. The function will return<br>
# zero on success, or else print a message and return non zero.<br>
<br>
function check_begin_end() {<br>
        if (monitor) {<br>
                print "Checking begin/end dates" &gt;monitor<br>
                fflush(monitor)<br>
        }<br>
<br>
        spawk_select("SELECT DATEDIFF(" end "," begin")")<br>
        if ((!spawk_first(data)) || (data[1] &lt; 1))<br>
                return(errmsg(begin "-" end ": invalid begin/end dates"))<br>
<br>
        # Function `check_begin_end' checks the `begin' and `end'<br>
        # dates and calculate the days between those two dates.<br>
        # Those days will set the `days' global variable value.<br>
<br>
        days = data[1]<br>
        spawk_select("SELECT DATE_FORMAT(" begin \<br>
                " - INTERVAL 25550 DAY, '%Y%m%d'), DATE_FORMAT(" \<br>
                end " - INTERVAL 5800 DAY, '%Y%m%d')")<br>
        spawk_first(data)<br>
        spawk_select("SELECT DATEDIFF(" (bdate_max = data[2]) "," \<br>
                (bdate_min = data[1]) ")")<br>
        spawk_first(data)<br>
        bdate_days = data[1]<br>
        return(0)<br>
}<br>
<br>
function load_players(                          l, i) {<br>
        if (monitor) {<br>
                print "Loading " players " players" &gt;monitor<br>
                fflush(monitor)<br>
        }<br>
<br>
        player_list[++i] = "Deep Blue (IBM)"<br>
        player_list[++i] = "Fritz 2.0"<br>
        player_list[++i] = "Fritz 3.0"<br>
        player_list[++i] = "Rybka"<br>
        player_list[++i] = "Kasparov Gary"<br>
        player_list[++i] = "Karpov Anatoli"<br>
        player_list[++i] = "Fisher Bobby"<br>
        for (i = 1; i &lt;= players; i++) {<br>
                spawk_update("INSERT INTO player (pl_key, pl_name, " \<br>
                        "pl_bdate) VALUES (" i ",'" \<br>
                        (i in player_list ? player_list[i] : \<br>
                        random_word() " " random_word()) \<br>
                        "'," bdate_min " + INTERVAL " \<br>
                        random(bdate_days) " DAY)", 2)<br>
<br>
                if ((i % 1000) == 0) {<br>
                        if (monitor) {<br>
                                print "Commiting " i \<br>
                                        " player insertions" &gt;monitor<br>
                                fflush(monitor)<br>
                        }<br>
<br>
                        spawk_update("COMMIT WORK", 2)<br>
                }<br>
        }<br>
<br>
        spawk_update("COMMIT WORK", 2)<br>
        if (monitor) {<br>
                print "Commited " players " player insertions" &gt;monitor<br>
                fflush(monitor)<br>
        }<br>
}<br>
<br>
function load_tournaments(                              i) {<br>
        if (monitor) {<br>
                print "Loading " tournaments " tournaments" &gt;monitor<br>
                fflush(monitor)<br>
        }<br>
<br>
        max_level = 3           # maximum tournament depth<br>
        max_sub = 10            # maximum number of sub-tournaments<br>
        max_duration = 100      # maximum tournament's dur in days<br>
        min_games = 4           # minimum tournament's games count<br>
        max_games = 100         # maximum tournament's games count<br>
        tdl = int(column_length("tournament", "tr_desc") * 0.40)<br>
        points_list[0] = "10,0"<br>
        points_list[1] = "0,10"<br>
        points_list[2] = "5,5"<br>
        for (tr_key = 0; load_tournament();)<br>
                ;<br>
<br>
        spawk_update("COMMIT WORK", 2)<br>
        if (monitor) {<br>
                print "Commited " tournaments \<br>
                        " tournament insertions" &gt;monitor<br>
                print "Commited " gm_key " games insertions" &gt;monitor<br>
                fflush(monitor)<br>
        }<br>
}<br>
<br>
function load_tournament(ht, level,                     n, desc, \<br>
        l, nw, w, tk, games) {<br>
        if (++tr_key &gt; tournaments)<br>
                return(0)<br>
<br>
        nw = 2 + random(10)<br>
        desc = random_word()<br>
        l = length(desc) + 1<br>
        while (nw-- &gt; 0) {<br>
                if ((l += length(w = random_word())) &gt; tdl)<br>
                        break<br>
<br>
                desc = desc " " w<br>
                l++<br>
        }<br>
<br>
        if (!ht)<br>
                ht = "NULL"<br>
<br>
        spawk_update("INSERT INTO tournament (tr_key, tr_desc, " \<br>
                "tr_begin, tr_end, tr_other) VALUES (" tr_key ",'" \<br>
                desc "',20010101,20010101," ht ")", 2)<br>
        games = random(max_games)<br>
        if (level)<br>
                games /= level<br>
<br>
        if (games &lt; min_games)<br>
                games = min_games<br>
<br>
        while (games-- &gt; 0)<br>
                load_game()<br>
<br>
        if ((tr_key % 1000) == 0) {<br>
                if (monitor) {<br>
                        print "Commiting " tr_key \<br>
                                " tournament insertions" &gt;monitor<br>
                        fflush(monitor)<br>
                }<br>
<br>
                spawk_update("COMMIT WORK", 2)<br>
        }<br>
<br>
        if ((level &lt; max_level) &amp;&amp; (random(5) == 1)) {<br>
                n = 2 + random(max_sub)<br>
                for(tk = tr_key; n-- &gt; 0;) {<br>
                        if (!load_tournament(tk, level + 1))<br>
                                return(0)<br>
                }<br>
        }<br>
<br>
        return(1)<br>
}<br>
<br>
function load_game() {<br>
        spawk_update("INSERT INTO game (gm_key, gm_tournament, " \<br>
                "gm_kind, gm_minutes, gm_white, gm_welo, gm_black, " \<br>
                "gm_belo, gm_begin, gm_end, gm_wpoints, gm_bpoints, " \<br>
                "gm_stop) VALUES (" ++gm_key "," tr_key "," \<br>
                1 "," 120 "," random_player() "," 0 "," \<br>
                random_player() "," 0 ",20080101,20080210," \<br>
                points_list[random(3)] ",1)", 2)<br>
        if ((gm_key % 1000) == 0) {<br>
                if (monitor) {<br>
                        print "Commiting " gm_key \<br>
                                " games insertions" &gt;monitor<br>
                        fflush(monitor)<br>
                }<br>
<br>
                spawk_update("COMMIT WORK", 2)<br>
        }<br>
}<br>
<br>
function generate_words(                        i, upper, \<br>
        lower, l, s) {<br>
        minwl = 4               # minimum word length<br>
        maxwl = 16              # maximum word length<br>
        if (!words)             # words database size<br>
                words = 10000<br>
<br>
        if (monitor) {<br>
                print "Generating " words " random words (" \<br>
                minwl " - " maxwl ")" &gt;monitor<br>
                fflush(monitor)<br>
        }<br>
<br>
        for (i = 65; i &lt; 91; i++)<br>
                upper[i - 65] = sprintf("%c", i)<br>
<br>
        for (i = 97; i &lt; 123; i++)<br>
                lower[i - 97] = sprintf("%c", i)<br>
<br>
        for (i = 0; i &lt; words; i++) {<br>
                l = minwl + random(maxwl - minwl)<br>
                s = upper[random_character()]<br>
                while (--l &gt; 0)<br>
                        s = s lower[random_character()]<br>
<br>
                word_list[i] = s<br>
        }<br>
}<br>
<br>
function random_word() {<br>
        return(word_list[random(words)])<br>
}<br>
<br>
function random_string(len1, len2, spaces,              l, s, \<br>
        i, space, dif, w) {<br>
        if ((l = int(len1 + random(dif = len2 -len1))) &gt; len2)<br>
                l = len2<br>
        else if (l &lt; len1)<br>
                l = len1<br>
<br>
        if (spaces) {<br>
                if ((spaces = random(spaces)) &lt;= 0)<br>
                        spaces = 1<br>
<br>
                if (spaces == 1) {<br>
                        if ((i = random(l)) &lt; mwl)<br>
                                i = mwl<br>
                        else if (l - i &lt;= mwl)<br>
                                i = int(l / 2)<br>
<br>
                        space[i]<br>
                }<br>
                else {<br>
                        for (i = 0; i &lt; spaces; i++)<br>
                                space[len1 + random(dif)]<br>
                }<br>
        }<br>
<br>
        s = random_upper()<br>
        for (i = 1; i &lt; l; i++) {<br>
                if (i in space) {<br>
                        s = s " "<br>
                        s = s random_upper()<br>
                        i++<br>
                }<br>
                else<br>
                        s = s random_lower()<br>
        }<br>
<br>
        return(s)<br>
}<br>
<br>
function random_character(                      c, n) {<br>
        while ((c = int(rand() * 26) % 26) == last_random_character) {<br>
                if (++n &gt; 3)<br>
                        break<br>
        }<br>
<br>
        return(last_random_character = c)<br>
}<br>
<br>
function random_player(                         p) {<br>
        if ((p = random(players) + 1) == previous_player)<br>
                p = previous_player + (p &lt;= 1 ? 1 : -1)<br>
<br>
        return(previous_player = p)<br>
}<br>
<br>
# Function `random' is a utility function accepting a positive<br>
# integer `n' and returning a random number in [0, n).<br>
<br>
function random(n) {<br>
        return(int((rand() * n)))<br>
}<br>
<br>
function column_length(table, column,                   data) {<br>
        spawk_select("SELECT CHARACTER_MAXIMUM_LENGTH " \<br>
                "FROM information_schema.COLUMNS " \<br>
                "WHERE TABLE_SCHEMA = 'chess' AND TABLE_NAME = '" \<br>
                table "' AND COLUMN_NAME = '" column "'")<br>
        spawk_first(data)<br>
        if (data[1] &lt; 1)<br>
                fatal(table "." column ": cannot identify column")<br>
<br>
        return(data[1] + 0)<br>
}<br>
<br>
function errmsg(msg) {<br>
        print SPAWKINFO["program"] ": " msg &gt;"/dev/stderr"<br>
        return(1)<br>
}<br>
<br>
function fatal(msg) {<br>
        exit(errmsg(msg))<br>
}<br>
<br>
function abort(n) {<br>
        if (n)<br>
                exit(errmsg("program aborted"))<br>
}<br>
</code></pre>
In order to run the above programs, first create an empty <i>chess</i> database:<br>
<pre><code>mysql -A &lt;&lt;+<br>
DROP DATABASE IF EXISTS chess;<br>
+<br>
mysql -A chess &lt;chess.dd<br>
awk -v begin=20010311 -v end=20020630 -v monitor=/dev/tty  -f load.awk<br>
</code></pre>
where <code>chess.dd</code> is the schema file. You'll see something like this:<br>
<pre><code>Setting begin/end dates<br>
Checking begin/end dates<br>
Interval...: 20010311 - 20020630 (476 days)<br>
Players....: 14280 (19310329 - 19860813, 20226)<br>
Tournaments: 1428<br>
Generating 10000 random words (4 - 16)<br>
Loading 14280 players<br>
Commiting 1000 player insertions<br>
Commiting 2000 player insertions<br>
Commiting 3000 player insertions<br>
...<br>
Commiting 13000 player insertions<br>
Commiting 14000 player insertions<br>
Commited 14280 player insertions<br>
Loading 1428 tournaments<br>
Commiting 1000 games insertions<br>
Commiting 2000 games insertions<br>
Commiting 3000 games insertions<br>
...<br>
Commiting 33000 games insertions<br>
Commiting 34000 games insertions<br>
Commiting 1000 tournament insertions<br>
Commiting 35000 games insertions<br>
Commiting 36000 games insertions<br>
...<br>
Commiting 47000 games insertions<br>
Commiting 48000 games insertions<br>
Commited 1428 tournament insertions<br>
Commited 48000 games insertions<br>
</code></pre>
The <i>chess</i> database is now loaded and we're ready to print some data. Let's print some tournaments:<br>
<pre><code>awk -f print.awk<br>
1<br>
Tpfzuhbz Gzqehmyegqiyi Cndj Dbdw Pfgw 2001-01-01<br>
2<br>
Gxuwgm Gamgvghrbkw Tvqam Sfeal Rseif 2001-01-01<br>
3<br>
Xjlbfkoy Sbonoezy Ietsr Ufmfztotc Vixgedc 2001-01-01<br>
4<br>
Lnudx Kehfknjdig Krxqu Rbjdcydftpixtl Xurifsmop 2001-01-01<br>
[Control-D]<br>
</code></pre>
We gave some torunaments keys as input to awk running the <code>print.awk</code> printing program. For each given tournament, awk printed the tournament's data to the standard output. To print the sub-turnaments too we have to set the <code>tree</code> (external) variable as you can recall:<br>
<pre><code>awk -v tree=yes -f print.awk<br>
42<br>
Szjuzlnftfsik Zndikoc Icvpw Pauhu Prhwqug Jnvsnh 2001-01-01<br>
        Msdjzbuyiczansy Sbalil Jwgtnbgwjzoimbg 2001-01-01<br>
        Jtwi Ebwzpxtgjv Rjkxegcjwdevo Kbfues Qpjwhxputw 2001-01-01<br>
                Eykjxygzfivz Rtlfumibg Xfanqpw Qdgnbfnaswlsbcz 2001-01-01<br>
                Hvaxfhic Xkcywmyo Qztsc Ybjbnh Ihgyhdryxo 2001-01-01<br>
                        Ksdj Txebomyr Ciyzhwx Hgjvuondyxdxpmp Jceqbf 2001-01-01<br>
                        Bmogfhsjsf Elegsjkxdt Cewfgsqfvmqgsl Karewlkbdxowo 2001-01-01<br>
                        Ukybugkzsuhbo Otdimikxkphnmbj Zijlp Cgictq 2001-01-01<br>
                Rdqjzeclcaypc Dctrenwsxfd Ybnbudtybqwq 2001-01-01<br>
                Wivdztciucec Dfxousgd Lnfbzjlulqxqyn 2001-01-01<br>
                Ivskpgfqmjxck Jqtzfqkebd Vghngeoy Ivhayecbqbpmsje 2001-01-01<br>
                Iefs Ykdqifjlqpodaz Usift Aisubzmdq Hgrfhudclqgcsh 2001-01-01<br>
        Axbhw Fjbyt Bsdbwcjeuo Exjqoedeq Ambqeheshdwsctq 2001-01-01<br>
        Xeswxas Zkmjwn Vxuecmowxshvfk Tmbtads Gowhpkdxlqp 2001-01-01<br>
4<br>
Lnudx Kehfknjdig Krxqu Rbjdcydftpixtl Xurifsmop 2001-01-01<br>
        Jsokgp Vnpydlw Buwzjrnh Elpdrpm Hxguylqhyxzt 2001-01-01<br>
        Gkngubrlqftj Clbpieuxaoj Nivtad Gsjfegelqdya 2001-01-01<br>
        Unhrdwi Udgbrdbz Akqhxkif Tiymlwnqbznrj Xqmkbym 2001-01-01<br>
        Kribhxlw Urfywgtxvu Uxuturbewpfcfc Qacemaglbxka 2001-01-01<br>
                Dmdfhetm Knew Icvpw Pwmxal Oxakpgdy Xcageyupki 2001-01-01<br>
                Ihnqjkywowunid Imabwxpmramq Vbfdz Cgrnoygw 2001-01-01<br>
                Hjkemcsnlnpcyj Cupskjztfxtk Pdjenan Cuserdvsj 2001-01-01<br>
                Zqudj Knqesmnvg Rpnbmjby Xrnhsnxyw Ilsoaxn 2001-01-01<br>
                Lkfvjywvzfw Cdzlqbwfiqcdy Gwuzlfninm Vsueruqtfh 2001-01-01<br>
                Kyge Bqmhos Bsrfxoxp Aimdm Wzdjlyove Cyrcvui 2001-01-01<br>
                Jpspblojzgli Wuel Exakgqtwqvlj Pvxpmrsjqfkr 2001-01-01<br>
                Syirln Lpepjztj Olgegmgdqi Utgbaqhlf Ikrzayxrjsbt 2001-01-01<br>
                Jqvizdyjizh Bqodxsnobkv Qxjzx Ptozxdq Kynjoa 2001-01-01<br>
                Fuoraendiwl Cewlnbp Rxfnwsdgbxyvmp Isctbwbfgzkd 2001-01-01<br>
                Lfjfzldf Aqbiah Mygtywsnm 2001-01-01<br>
        Wjuqnxhdiovty Vxuecmowxshvfk Squrcmlqluzpkw 2001-01-01<br>
[Control-D]<br>
</code></pre>
In order to print the games too, we must set the <code>games</code> (external) variable:<br>
<pre><code>awk -v tree=yes -v games=yes -f print.awk<br>
42<br>
        Axbhw Fjbyt Bsdbwcjeuo Exjqoedeq Ambqeheshdwsctq 2001-01-01<br>
                Feznvt Bztdaqd VS Qydaoblba Qgldr (1-0)<br>
                Agyeybwxkyrz Hrmxkt VS Ypmiolndygjguct Ohtadewugbzax (0-1)<br>
                Okwqtqtwpfehbep Npnw VS Pcgjzi Zmcjkbhoytrmvc (1-0)<br>
                Hjicb Hnqwa VS Wrvtruajktq Sulrpxi (0-1)<br>
                Lkpy Rmsq VS Vhlwou Jkwtzeczkod (1/2-1/2)<br>
                Ryutwgwthoh Zmavzjc VS Pcfrkyhzm Xsdmbs (1/2-1/2)<br>
                Orzgsljniayzmi Cilkplf VS Poene Hsibulwszy (1/2-1/2)<br>
                Kdayq Wpxofodjkg VS Wfpia Cmegzt (1-0)<br>
                Etecorj Azhudlusfbvdax VS Ztbdvpqvblo Unaycfvzqn (1/2-1/2)<br>
        Xeswxas Zkmjwn Vxuecmowxshvfk Tmbtads Gowhpkdxlqp 2001-01-01<br>
                Lpclno Apsagnlsoazb VS Dbjfeqegsxgwfy Phlsqfrgoltm (1/2-1/2)<br>
                Pawmemzaehj Hbwhagqug VS Vnyld Mvnitn (1-0)<br>
                Btswlzofdbq Ipufdrtsomvyru VS Edscxtpqnx Qgbzhneksn (1/2-1/2)<br>
                Xuzhwpaqsz Ubkaflk VS Xcwkh Bqhtmzpj (1/2-1/2)<br>
                Xhkaly Mdemprhx VS Wzgnzximfdmk Mgdvbkzbkltkos (0-1)<br>
                Pcbibifzcx Dwnxwhb VS Zuqabr Uvxjcjic (1/2-1/2)<br>
                Zmkdcegkysjdiqc Xvkysrpm VS Whawkrot Nklzdojir (0-1)<br>
                Ytki Nsznfmvshkbzu VS Mskmlabsxrkom Lxlugcxn (1-0)<br>
                Bvasynqpin Yuns VS Hkmruzxvbtmea Encmlsgvcwv (0-1)<br>
                Gkru Rtjuzufwyxwla VS Zxzmlh Fmlmy (1/2-1/2)<br>
4<br>
Lnudx Kehfknjdig Krxqu Rbjdcydftpixtl Xurifsmop 2001-01-01<br>
        Aedltr Rgpnfjueynauhnh VS Gbsxebqgofhmith Rwqf (0-1)<br>
        Xigxanu Kmnwvxcoxholmsa VS Uhuroitij Fcmlzsqvhzabjwn (0-1)<br>
        Sbdkwcxdbrdjl Tafw VS Fspzyaxkfbofiak Ysovwbzfkrpl (1/2-1/2)<br>
        Lupancbv Bqhtmzpj VS Sutbyi Tmfrkxgwkahz (1/2-1/2)<br>
        Jsokgp Vnpydlw Buwzjrnh Elpdrpm Hxguylqhyxzt 2001-01-01<br>
                Mjbcpzafvc Uyxisjimpkgeucb VS Brqhaeq Hwnsz (1-0)<br>
                Qjenkqxkeyqy Lpjy VS Vsxacqjxoidie Orjyzgrjeb (1/2-1/2)<br>
                Xnvbxuavxaqwcg Rbjdcydftpixtl VS Fysyjycvdr Zagidzwfyfc (0-1)<br>
                Dvpzkn Tplkxzcwaudyai VS Bnxgcpiznzrpchp Owhwq (0-1)<br>
                Xzykchofepwm Lijgvxtvflwlxzy VS Fbfliucrpkr Yxjkhzimkjka (1-0)<br>
                Lpjy Uigyvu VS Ksyklweaop Nklywalzpiunyxu (1/2-1/2)<br>
                Gxrg Heytvobpbitl VS Wezbowspvsadsap Nlunzemdplv (1-0)<br>
                Tmdb Hqoc VS Hqnonznilhx Mnhihbehpflbtl (1/2-1/2)<br>
                Yfmtusyp Ecgzxvgce VS Hqoc Zuxgricmurqabqn (1/2-1/2)<br>
                Sjwbotq Modov VS Ptmazhazv Sbalil (0-1)<br>
        Gkngubrlqftj Clbpieuxaoj Nivtad Gsjfegelqdya 2001-01-01<br>
                Odxbsr Ozhgm VS Mvnyubzajfyv Galg (0-1)<br>
                Lumrhibm Sncpubs VS Hmxpefxpb Cqghdrxngyelmnm (1/2-1/2)<br>
                Txwvcrhp Ghruyhwphaq VS Nhcgbecguzeyd Sekgqvhrzebioet (0-1)<br>
                Gpbsxzvc Zlxwmlnokr VS Wfelhjabsgv Eqfopnhgavtp (1/2-1/2)<br>
                Wlmrpu Syiyxf VS Qviulrnv Rdojofsq (0-1)<br>
                Bmvtpvftbo Qgtykyxt VS Esendzazt Gfmkiw (1/2-1/2)<br>
                Rjlonsgkjzbwhti Ymcbavfkowlwmsg VS Mfgfzxucvf Tkaouzywhxd (1/2-1/2)<br>
                Eqgphzfvqkzbc Rypdwjb VS Kanfqiolbuvijg Gazrhzliu (1-0)<br>
        Unhrdwi Udgbrdbz Akqhxkif Tiymlwnqbznrj Xqmkbym 2001-01-01<br>
                Zhmfvqex Mjmdqmbu VS Yaudmlsdxdw Mtbzb (1/2-1/2)<br>
                Njonjuhcrtbf Zuehshu VS Gmtzjybix Dwvwkpqpfclj (1-0)<br>
                Svgfuzqmvfkapnf Xfcrs VS Nrpwghtrv Tledhzrtucpmn (1-0)<br>
                Zodfhnszsyp Rwrzir VS Vmhryeqsnpuiz Nqikbaogpah (1/2-1/2)<br>
                Rojkdq Etnh VS Iozgdixwcq Pqcegwoel (1-0)<br>
                Tgfbao Rzxgl VS Obndigsae Pqkyohwhdbwlae (0-1)<br>
        Kribhxlw Urfywgtxvu Uxuturbewpfcfc Qacemaglbxka 2001-01-01<br>
                Qovolocqszaop Qmasksewrtmd VS Xvxtypdg Byvgmomagxfukcg (0-1)<br>
                Ztbdvpqvblo Unaycfvzqn VS Cstscezkml Ixkgoickiugdq (1-0)<br>
                Fdmofedgbamukg Wuctj VS Fvgsbyui Ykgjvd (0-1)<br>
                Bgtaeukunhbshn Pmnjdgv VS Nfmiwkbfifyfz Kfldqonzozbyicw (0-1)<br>
                Lxqfvdcibyr Xmvxmcmwnrbeno VS Cecxytsrlsdog Hjicb (1-0)<br>
                Bohy Qxqxreoe VS Skyamszmksca Ktlmsqfajmnbwcs (0-1)<br>
                Dmdfhetm Knew Icvpw Pwmxal Oxakpgdy Xcageyupki 2001-01-01<br>
                        Nhnrwvmnzr Gcvg VS Uikqczwp Gdsejvijunckve (1/2-1/2)<br>
                        Ixqbluntrl Pgslweyxmjvwi VS Cwafcfcmvudox Ozoedqjqmqvp (0-1)<br>
                        Ozoedqjqmqvp Xmbfh VS Syiyxf Yjmewupfldvs (0-1)<br>
                        Vdfaealswpvs Kdayq VS Sgkuwtmzgqwt Swercejg (1/2-1/2)<br>
                        Pqsopnjqpmwugid Dzvzdkfm VS Pdbrjnl Hrmxkt (0-1)<br>
                        Fosmovdksk Zcltxnp VS Prxwu Zqlqzqgj (0-1)<br>
                        Kozbtrhv Owqcdw VS Umxapoy Wfnqcx (1-0)<br>
                        Kjzgirblw Bcrsq VS Dcwbwmkj Rxpcudsyfawfm (1/2-1/2)<br>
                        Lpoj Knbxmxtifk VS Usuhkb Ngpmvc (0-1)<br>
                Ihnqjkywowunid Imabwxpmramq Vbfdz Cgrnoygw 2001-01-01<br>
                        Fuervkbhi Ltvdyc VS Xaicwdldofvgcnc Trnfugx (0-1)<br>
                        Wxbe Vcsmctpj VS Tzmcsdbielb Rgqa (1/2-1/2)<br>
                        Gwjrp Dhbor VS Fsqkjr Amgx (1-0)<br>
                        Ntpw Jiydytwuj VS Yxpvrxzbcqfrtm Twgzt (1-0)<br>
                        ...<br>
                        Wpxofodjkg Csjvpefd VS Xyiyrvw Oafh (1-0)<br>
                Lfjfzldf Aqbiah Mygtywsnm 2001-01-01<br>
                        Wpxofodjkg Csjvpefd VS Laic Cmlf (0-1)<br>
                        Rztkiwzj Atsk VS Pewyxzyososoc Xdue (1-0)<br>
                        Sirazrfdvm Qpugwngn VS Almksikpocr Lvlozkyrpu (0-1)<br>
                        Tnzbacz Qrzaipwoh VS Phatbxtlfqjnz Dmybavpq (0-1)<br>
                        Bmogfhsjsf Tjcznbuyzym VS Xpopae Nklzntfvbqjy (1-0)<br>
                        Fdnjegf Lcfuctv VS Autxhz Pwotplqnciowvqc (1/2-1/2)<br>
                        Ipnyvl Tdmqpnavrsczqip VS Vjmxbw Xugrvk (1-0)<br>
                        Reykc Ernvafmeopfo VS Uowlcnsqzjvbthp Rgpyomvi (0-1)<br>
                        Ftnwekxkmk Dueqai VS Kdbfmcphadokqou Xkproknocp (1/2-1/2)<br>
                        Bhaq Szfxyfxqjbz VS Fdvaxklagcfdhph Yapopqufjnybjg (1-0)<br>
                        Vfoladzkx Geoezbzkedthtvh VS Jwqjc Jfckbqmerjpvu (1/2-1/2)<br>
        Wjuqnxhdiovty Vxuecmowxshvfk Squrcmlqluzpkw 2001-01-01<br>
                Tkvy Ydoybvdfgvlq VS Rpubztksgy Euqbtizimivy (1/2-1/2)<br>
                Pvrcmt Xgsamklk VS Ltuilywbzxhx Okbprfezt (1-0)<br>
                Ezjhda Ictinyi VS Yarazk Jilg (0-1)<br>
                Bzezua Gmjpcjk VS Hmnxrluz Qeyiqaxyrmva (1/2-1/2)<br>
[Control-D]<br>
</code></pre>
To print all the games of all tournaments:<br>
<pre><code>mysql -N -A chess &gt;tkeys &lt;&lt;+<br>
SELECT tr_key FROM tournament;<br>
+<br>
awk -v tree=yes -v games=yes -f print.awk tkeys &gt;report<br>
</code></pre>
After creating the <code>tkeys</code> file to contain all the tournaments' primary keys, we run awk with the <code>print.awk</code> reading from the <code>tkeys</code> file and sending the printout to the <code>report</code> file. That's our report for all the tournaments in the database. Another way to produce the same report is to run the print command with the "<code>%</code>" as input and let awk to collect the tournaments:<br>
<pre><code>awk -v tree=yes -v games=yes -f print.awk &gt;report<br>
%<br>
[Control-D]<br>
</code></pre>
It's the same report!<br>
<p>
All of the above programs can of course be hidden in shell scripts which may call awk with the proper variables' settings based on shell scripts' options. That technique have been shawn earlier (tables and columns counts, <code>schematc</code> schell script) and it's not in the scope of that tutorial wiki to write full detailed programs. But it's no harm to tell that the awk <i>chess</i> scripts shall be saved in a system directory, e.g. <code>/apps/chess/awk</code> in order to have smaller shell scripts and easier maintenance. Of course the installation procedure is a little more complicated, but it worths the effort.<br>
</p>

<a href='Tutorial.md'>Top</a>

<u>
<h1>SPAWK Librarian</h1>
</u>
<p>
In the following chapter we're going to develop programs around a library database used in managing books, readers and books' circulation. The schema is named <i>library</i>, of course, and the data dictionary in SQL follows right away:<br>
</p>
<pre><code># Database "library" describes a books, readers and books'<br>
# circulation. There are two major components that constitute<br>
# the database: books and circulation. Books component consists<br>
# of writers, books, authors, subjects and copies. Circulation<br>
# component consists of readers and boroughs.<br>
<br>
CREATE DATABASE `library`;<br>
USE `library`;<br>
<br>
CREATE TABLE `writer` (<br>
        `wr_key`        NUMERIC(9) NOT NULL,<br>
        PRIMARY KEY `wr_key` USING HASH (<br>
                `wr_key`<br>
        ),<br>
        `wr_name`       VARCHAR(60) NOT NULL,<br>
        UNIQUE INDEX `wr_name` USING BTREE (<br>
                `wr_name`<br>
        )<br>
)<br>
ENGINE = "InnoDB";<br>
<br>
CREATE TABLE `book` (<br>
        `bo_key`        NUMERIC(9) NOT NULL,<br>
        PRIMARY KEY `bo_key` USING HASH (<br>
                `bo_key`<br>
        ),<br>
        `bo_title`      VARCHAR(256) NOT NULL,<br>
        UNIQUE INDEX `bo_title` USING BTREE (<br>
                `bo_title`<br>
        )<br>
)<br>
ENGINE = "InnoDB";<br>
<br>
# Every book may have many authors. Those authors<br>
# records refer the writer and the book to which<br>
# the writer contributes.<br>
<br>
CREATE TABLE `author` (<br>
        `au_book`       NUMERIC(9) NOT NULL,<br>
        FOREIGN KEY `au_book` (<br>
                `au_book`<br>
        ) REFERENCES `book` (<br>
                `bo_key`<br>
        ),<br>
        `au_writer` NUMERIC(9) NOT NULL,<br>
        FOREIGN KEY `au_writer` (<br>
                `au_writer`<br>
        ) REFERENCES `writer` (<br>
                `wr_key`<br>
        )<br>
)<br>
ENGINE = "InnoDB";<br>
<br>
# Each book have some related subjects. Those<br>
# subjects are just strings related with each<br>
# specific book.<br>
<br>
CREATE TABLE `subject` (<br>
        `su_book`       NUMERIC(9) NOT NULL,<br>
        FOREIGN KEY `su_book` (<br>
                `su_book`<br>
        ) REFERENCES `book` (<br>
                `bo_key`<br>
        ),<br>
        `su_subject`    VARCHAR(256) NOT NULL,<br>
        INDEX `su_subject` USING BTREE (<br>
                `su_subject`<br>
        )<br>
)<br>
ENGINE = "InnoDB";<br>
<br>
# Every book has many copies in various locations.<br>
# Each copy has each own key (plain number) and<br>
# refers to the related book.<br>
<br>
CREATE TABLE `copy` (<br>
        `co_key`        NUMERIC(9) NOT NULL,<br>
        PRIMARY KEY `co_key` USING HASH (<br>
                `co_key`<br>
        ),<br>
        `co_book`       NUMERIC(9) NOT NULL,<br>
        FOREIGN KEY `co_book` (<br>
                `co_book`<br>
        ) REFERENCES `book` (<br>
                `bo_key`<br>
        ),<br>
        `co_location`   VARCHAR(128) NOT NULL,<br>
        INDEX `co_location` USING BTREE (<br>
                `co_location`<br>
        )<br>
)<br>
ENGINE = "InnoDB";<br>
<br>
CREATE TABLE `reader` (<br>
        `rd_key`        NUMERIC(9) NOT NULL,<br>
        PRIMARY KEY `rd_key` USING HASH (<br>
                `rd_key`<br>
        ),<br>
        `rd_name`       VARCHAR(60) NOT NULL,<br>
        UNIQUE INDEX `rd_name` USING BTREE (<br>
                `rd_name`<br>
        )<br>
)<br>
ENGINE = "InnoDB";<br>
<br>
CREATE TABLE `borough` (<br>
        `br_reader`     NUMERIC(9) NOT NULL,<br>
        FOREIGN KEY `br_reader` (<br>
                `br_reader`<br>
        ) REFERENCES `reader` (<br>
                `rd_key`<br>
        ),<br>
        `br_copy`       NUMERIC(9) NOT NULL,<br>
        FOREIGN KEY `br_copy` (<br>
                `br_copy`<br>
        ) REFERENCES `copy` (<br>
                `co_key`<br>
        ),<br>
<br>
        # The "br_date" column is the date of borough.<br>
        # The "br_until" date is the date for the copy<br>
        # to be returned. The "br_return" columns is<br>
        # the actual return date. That means that if<br>
        # the "br_return" is null and the "br_until"<br>
        # date is greater than the current date, the<br>
        # reader must be informed.<br>
<br>
        `br_date`       DATE NOT NULL,<br>
        `br_until`      DATE,<br>
        INDEX `br_until` USING BTREE (<br>
                `br_until`<br>
        ),<br>
        `br_return`     DATE,<br>
        INDEX `br_return` USING BTREE (<br>
                `br_return`<br>
        )<br>
)<br>
ENGINE = "InnoDB";<br>
</code></pre>
<p>
As you can see, we added the backquote escape mechanism of MySQL in order to avoid keyword conflicts, e.g. the <i>copy</i> table may conflict, etc. That's not a bad thing to do for every MySQL script, but of course it's a cumbersome and tedious procedure as well. However, by using such database objects' names as <i>copy</i>, <i>read</i>, <i>write</i> etc, we have to follow the right procedure.<br>
</p>

<a href='Tutorial.md'>Top</a>

<u>
<h2>Printing Books' Data</h2>
</u>
<p>
Given the above schema (database), the first thing to do is to develop a printing program to print books' data. The program may read book keys (default trivial, but of little use), title patterns, subject patterns, or author patterns, and print the books matching the given criteria. To make the program flexible enough in order to accept more than one type of criteria, we must find a clever way to tell the program what criteria to use. Using awk, it's a trivial method to use some (external) variables to specify such things. In our case we'll use the <code>criteria</code> string for telling the program what criteria to use.<br>
</p>
<p>
If the <code>criteria</code> variable is an empty string or not set at all, then the input will consist of plain book keys (<code>bo_key</code> values). If the <code>criteria</code> has a value, then that value will consist of the <i><b>T</b></i>, <i><b>A</b></i>, or <i><b>S</b></i> letters (upper or lower case) to denote <i>title</i>, <i>author</i> (or <i>writer</i>) and <i>subject</i> criteria respectively, e.g. a <code>criteria</code> value of <b><code>TA</code></b> means that the input lines are consisting of two tab separated fields: the first field is a <i>title</i> pattern while the second field is an <i>author</i> (<i>writer</i>) pattern. If the <code>criteria</code> is valued <b><code>STA</code></b>, then the first field is a <i>subject</i> pattern, the second field is a <i>title</i> pattern and the third field is an <i>author</i> (<i>writer</i>) pattern.<br>
</p>
<p>
Another (external) variable to be used in the <code>bfind</code> program is the <code>sort</code> string variable which is the <i>order by</i> SQL clause. That is of no great use, although, because of the limited number of <i>book</i> columns, so there are not much choices in sorting the books. You can, of course, turn off the sorting by setting that variable to a single space character (not an empty string).<br>
</p>
<pre><code>BEGIN {<br>
        extension("libspawk.so", "dlload")<br>
        SPAWKINFO["program"] = "bfind"<br>
        SPAWKINFO["database"] = "library"<br>
        abort(set_criteria())<br>
        if (!sort)<br>
                sort = "ORDER BY `bo_title`, `bo_key`"<br>
<br>
        FS = "\t"<br>
        OFS = "\t"<br>
}<br>
<br>
NF &gt; 0 {<br>
        spawk_query("SELECT `bo_key`, `bo_title` " \<br>
                "FROM `book` WHERE 1 = 1")<br>
        for (i = 1; i &lt;= n_criteria; i++) {<br>
                if ((criteria_list[i] == "b") &amp;&amp; (NF &gt;= i)) {<br>
                        spawk_query(" AND `bo_key` = " $i)<br>
                        continue<br>
                }<br>
<br>
                if ((criteria_list[i] == "t") &amp;&amp; (NF &gt;= i)) {<br>
                        spawk_query(" AND `bo_title` LIKE '" \<br>
                                spawk_string($i) "'")<br>
                        continue<br>
                }<br>
<br>
                if ((criteria_list[i] == "s") &amp;&amp; (NF &gt;= i)) {<br>
                        spawk_query(" AND `bo_key` IN (SELECT " \<br>
                                "`su_book` FROM `subject` WHERE " \<br>
                                "`su_subject` LIKE '" \<br>
                                spawk_string($i) "')")<br>
                        continue<br>
                }<br>
<br>
                if ((criteria_list[i] == "a") &amp;&amp; (NF &gt;= i)) {<br>
                        spawk_query(" AND `bo_key` IN (SELECT " \<br>
                                "`au_book` FROM `author` WHERE " \<br>
                                "`au_writer` IN (SELECT `wr_key` " \<br>
                                "FROM `writer` WHERE `wr_name` " \<br>
                                "LIKE '" spawk_string($i) "'))")<br>
                        continue<br>
                }<br>
        }<br>
<br>
        if (spawk_update(" " sort)) {<br>
                errmsg("invalid pattern(s) (" spawk_error() ")")<br>
                next<br>
        }<br>
<br>
        spawk_results()<br>
        while (spawk_data(book))<br>
                print_book(book)<br>
}<br>
<br>
function print_book(book,                       data) {<br>
        print book[2]<br>
        spawk_select("SELECT `au_writer` FROM `author` " \<br>
                "WHERE `au_book` = " book[1])<br>
        if (!spawk_data(data))<br>
                return<br>
<br>
        print "Authors:"<br>
        do {<br>
                print OFS writer(data[1])<br>
        } while (spawk_data(data))<br>
<br>
        spawk_select("SELECT `su_subject` FROM `subject` " \<br>
                "WHERE `su_book` = " book[1])<br>
        if (!spawk_data(data))<br>
                return<br>
<br>
        print "Subjects:"<br>
        do {<br>
                print OFS data[1]<br>
        } while (spawk_data(data))<br>
<br>
        spawk_select("SELECT `co_key`, `co_location` FROM `copy` " \<br>
                "WHERE `co_book` = " book[1])<br>
        if (!spawk_data(data))<br>
                return<br>
<br>
        print "Copies:"<br>
        do {<br>
                print OFS data[0]<br>
        } while (spawk_data(data))<br>
}<br>
<br>
function writer(wk,                     data) {<br>
        spawk_select("SELECT `wr_name` FROM `writer` " \<br>
                "WHERE `wr_key` = " wk)<br>
        spawk_first(data)<br>
        return(data[1])<br>
}<br>
<br>
function set_criteria(                          l, i, c, errs) {<br>
        if ((!criteria) || ((l = length(criteria)) &lt;= 0)) {<br>
                criteria_list[++n_criteria] = "b"<br>
                return(0)<br>
        }<br>
<br>
<br>
        for (i = 1; i &lt;= l; i++) {<br>
                if ((c = substr(criteria, i, 1)) ~ /[bB]/) {<br>
                        errs += set_criteria_item(c, "b")<br>
                        continue<br>
                }<br>
<br>
                if (c ~ /[tT]/) {<br>
                        errs += set_criteria_item(c, "t")<br>
                        continue<br>
                }<br>
<br>
                if (c ~ /[sS]/) {<br>
                        errs += set_criteria_item(c, "s")<br>
                        continue<br>
                }<br>
<br>
                if (c ~ /[aAwW]/) {<br>
                        errs += set_criteria_item(c, "a")<br>
                        continue<br>
                }<br>
        }<br>
<br>
        return(errs + 0)<br>
}<br>
<br>
function set_criteria_item(s, c,                        i) {<br>
        for (i in criteria_list) {<br>
                if (criteria_list[i] == c)<br>
                        return(errmsg(criteria \<br>
                                " : multiple criteria (" s ")"))<br>
        }<br>
<br>
        criteria_list[++n_criteria] = c<br>
        return(0)<br>
}<br>
<br>
function errmsg(msg) {<br>
        print SPAWKINFO["program"] ": " msg &gt;"/dev/stderr"<br>
        return(1)<br>
}<br>
<br>
function abort(n) {<br>
        if (n)<br>
                exit(errmsg("program aborted"))<br>
}<br>
</code></pre>
<p>
Given a list of <i>book</i> keys (<code>bo_key</code>s) in file <code>book.keys</code>, we can print the books' data for those books:<br>
</p>
<pre><code>awk -f bfind.awk book.keys<br>
</code></pre>
<p>
To provide writer names' patterns and titles' patterns run:<br>
</p>
<pre><code>awk -v criteria=WT -f bfind.awk<br>
</code></pre>
If we're searching for books written by "<b>Papas</b>" with titles including "<b>wale</b>" the input line must be:<br>
<pre><code>Papas,%-&gt;%wale%<br>
</code></pre>
<p>
where the <code>-&gt;</code> is a tab character.<br>
</p>

<a href='Tutorial.md'>Top</a>

<u>
<h2>Checking Circulation Delays</h2>
</u>
<p>
Here follows a SPAWK script that prints the books that have been delayed to be returned. The core table is <i>borough</i> while the check date can be set from the command line as an (external) awk variable; if not set, the check date is assumed to be the current date. By default, the <i>borough</i> records are raw printed, but we can set the (external) variable <i>detail</i> to print the <i>book</i> and <i>reader</i> data also.<br>
</p>
<pre><code># The following program seeks and prints the delayed boroughed<br>
# books, that is books that must have been returned at specified<br>
# dates (default current date). The "br_date" column is the date<br>
# of boroughing the book, "br_until" is the date when the book<br>
# must be returned to the library and "br_return" is the date<br>
# that the book is actually returned to the library.<br>
#<br>
# The program uses the following external variables, where external<br>
# means variables to set in the command line:<br>
#<br>
#       date    Is the target date. If not set, then the<br>
#               current date is assumed.<br>
#<br>
#       detail  If set to a non empty, non zero valued, then<br>
#               the seeked "borough" records will be printed<br>
#               in detail, that is reader's name and book's<br>
#               tite will be printed. If not set, then raw<br>
#               "borough" data will be printed (usually to<br>
#               post format with other, most sophisticated<br>
#               report processing programs).<br>
<br>
BEGIN {<br>
        extension("libspawk.so", "dlload")<br>
        SPAWKINFO["program"] = "borough"<br>
        SPAWKINFO["database"] = "library"<br>
<br>
        # Now it's time to check/set the target date<br>
<br>
        set_date()<br>
<br>
        # Run the query to select and print the target<br>
        # "borough" records and exit (no input will be<br>
        # processed).<br>
<br>
        borough()<br>
        exit(0)<br>
}<br>
<br>
function borough(                       data) {<br>
        # Here follows the query to select the target "borough"<br>
        # records. The only thing that can produce an error in<br>
        # that query is a wrong "date" value. We use that simple<br>
        # fact in order to run the query and in the same time<br>
        # check the "date" value.<br>
<br>
        if (spawk_update("SELECT `br_reader`, `br_copy`, " \<br>
                "`br_date`, `br_until` FROM borough " \<br>
                "WHERE br_return IS NULL AND br_until &lt; " date)) {<br>
                print date ": invalid date" &gt;"/dev/stderr"<br>
                exit(2)<br>
        }<br>
<br>
        # The query passed the "date" value check, so convert the<br>
        # query from an "update" query to a "select" query and<br>
        # read the results ("borough" records).<br>
<br>
        spawk_results()<br>
        while (spawk_data(data)) {<br>
                # Based on the "detail" variable print the<br>
                # selected "borough" data as raw data ("detail"<br>
                # not set), or as detailed data ("detail" set).<br>
<br>
                if (detail)<br>
                        print_borough(data)<br>
                else<br>
                        print data[0]<br>
        }<br>
}<br>
<br>
# The "print_borough" function accepts a "borough" returned data<br>
# array and prints the reader's name, the book's title and the<br>
# date that the book must have been returned to the library.<br>
<br>
function print_borough(borough) {<br>
        print "Reader: " reader(borough[1])<br>
        print "Book..: " title(borough[2])<br>
        print "Date..: " borough[3] " - " borough[4]<br>
        print ""<br>
}<br>
<br>
# Function "reader" is a utility function that accepts a reader's<br>
# primary key ("rd_key") and returns the reader's name ("rd_name").<br>
<br>
function reader(rk,                             data) {<br>
        spawk_select("SELECT `rd_name` FROM `reader` " \<br>
                "WHERE `rd_key` = " rk)<br>
        spawk_first(data)<br>
        return(data[1])<br>
}<br>
<br>
# Function "title" is a utility function that accepts a book copy's<br>
# primary key ("co_key") and returns the related book's title<br>
# ("bo_title").<br>
<br>
function title(ck,                              data) {<br>
        spawk_select("SELECT `bo_title` FROM `book` " \<br>
                "WHERE `bo_key` = (SELECT `co_book` FROM `copy` " \<br>
                "WHERE `co_key` = " ck ")")<br>
        spawk_first(data)<br>
        return(data[1])<br>
}<br>
<br>
# Function "set_date" is a initialization function to check/set<br>
# the target date. If the "date" (external) value has been set<br>
# (in the command line), then that date will be used as the<br>
# target date, else the current date is assumed.<br>
<br>
function set_date(                                   data) {<br>
        if (!date) {<br>
                spawk_select("SELECT DATE_FORMAT(CURDATE(), '%Y%m%d')")<br>
                spawk_first(data)<br>
                date = data[1]<br>
        }<br>
}<br>
</code></pre>

<a href='Tutorial.md'>Top</a>

<u>
<h2>Using Awk Libraries</h2>
</u>
<p>
We're now going to develop a utilities' functions awk library, that is an awk script that does not form a complete program, in the sense that it lacks a detail or other input pattern section, but it rather consists of awk functions' definitions to be used in other awk programs.<br>
</p>
<pre><code># This is NOT a complete awk program file. There is no detail section,<br>
# nor any pattern matching sections, but rather a set of "library"<br>
# utility functions are defined to be used in other awk programs.<br>
<br>
# Function `witer' accepts a writer's primary key (`wr_key') and<br>
# returns the writer's name (`wr_name'). If the passed writer's<br>
# key is invalid or the writer is not found in the database, then<br>
# an error message is printed and an empty string is returned.<br>
<br>
function writer(wk,                     data) {<br>
        if (wk !~ /^[ \t]*[0-9][0-9]*[ \t]*$/) {<br>
                print SPAWKINFO["program"] ": " wk \<br>
                        ": invalid `wr_key' value" &gt;"/dev/stderr"<br>
                return(data[1])<br>
        }<br>
<br>
        spawk_select("SELECT `wr_name` FROM `library`.`writer` " \<br>
                "WHERE `wr_key` = " wk)<br>
        if (!spawk_first(data))<br>
                print SPAWKINFO["program"] ": " wk \<br>
                        ": writer not found" &gt;"/dev/stderr"<br>
<br>
        return(data[1])<br>
}<br>
<br>
# Function `title' accepts a book's primary key (`bo_key') and<br>
# returns the book's title (`bo_title'). If the passed book's<br>
# key is invalid or the book is not found in the database, then<br>
# an error message is printed and an empty string is returned.<br>
<br>
function title(bk,                      data) {<br>
        if (bk !~ /^[ \t]*[0-9][0-9]*[ \t]*$/) {<br>
                print SPAWKINFO["program"] ": " bk \<br>
                        ": invalid `bo_key' value" &gt;"/dev/stderr"<br>
                return(data[1])<br>
        }<br>
<br>
        spawk_select("SELECT `bo_title` FROM `library`.`book` " \<br>
                "WHERE `bo_key` = " bk)<br>
        if (!spawk_first(data))<br>
                print SPAWKINFO["program"] ": " bk \<br>
                        ": book not found" &gt;"/dev/stderr"<br>
<br>
        return(data[1])<br>
}<br>
<br>
# Function `author' accepts a book's primary key (`bo_key') and<br>
# selects the book's related authors. If the passed book's key<br>
# is invalid, then an error message is printed and a non zero<br>
# value is returned and no results are available, else the<br>
# select query is executed and zero is returned; you must<br>
# fetch the results via repeated "spawk_data" calls, or<br>
# by calling "spawk_first", "spawk_last" or "spawk_clear"<br>
# functions.<br>
<br>
function author(bk) {<br>
        if (bk !~ /^[ \t]*[0-9][0-9]*[ \t]*$/) {<br>
                print SPAWKINFO["program"] ": " bk \<br>
                        ": invalid `bo_key' value" &gt;"/dev/stderr"<br>
                return(1)<br>
        }<br>
<br>
        spawk_select("SELECT `au_writer` FROM `library`.`author` " \<br>
                "WHERE `au_book` = " bk)<br>
        return(0)<br>
}<br>
<br>
# Function `subject' accepts a book's primary key (`bo_key') and<br>
# selects the book's related subjects in alphabetical order. If<br>
# the passed book's key is invalid, then an error message is printed,<br>
# a non zero value is returned and no results are available, else the<br>
# select query is executed and zero is returned; you must<br>
# fetch the results via repeated "spawk_data" calls, or<br>
# by calling "spawk_first", "spawk_last" or "spawk_clear"<br>
# functions.<br>
<br>
function subject(bk) {<br>
        if (bk !~ /^[ \t]*[0-9][0-9]*[ \t]*$/) {<br>
                print SPAWKINFO["program"] ": " bk \<br>
                        ": invalid `bo_key' value" &gt;"/dev/stderr"<br>
                return(1)<br>
        }<br>
<br>
        spawk_select("SELECT `su_subject` FROM `library`.`subject` " \<br>
                "WHERE `su_book` = " bk " ORDER BY su_subject")<br>
        return(0)<br>
}<br>
<br>
# Function `get_copy_data' accepts a copy's primary key (`co_key') as<br>
# first argument and an array as a second argument (optional). It<br>
# returns the copy's data in the array (indices "book" and "location").<br>
# If the passed copy's key is invalid or if the copy is not found,<br>
# the function returns non zero, else zero is returned. We can<br>
# pass a third non empty, non zero parameter to get an extra item<br>
# in the returned array; that item will be indexed "title" and<br>
# it's value will be the book's title.<br>
<br>
function get_copy_data(ck, data, get_title) {<br>
        delete data<br>
        if (ck !~ /^[ \t]*[0-9][0-9]*[ \t]*$/) {<br>
                print SPAWKINFO["program"] ": " ck \<br>
                        ": invalid `co_key' value" &gt;"/dev/stderr"<br>
                return(1)<br>
        }<br>
<br>
        spawk_select("SELECT `co_book`, `co_location` " \<br>
                "FROM `library`.`copy` WHERE `co_key` = " \<br>
                ck, "book,location")<br>
        if (!spawk_first(data)) {<br>
                print SPAWKINFO["program"] ": " ck \<br>
                        ": copy not found" &gt;"/dev/stderr"<br>
                return(1)<br>
        }<br>
<br>
        if (get_title)<br>
                data["title"] = title(data["book"])<br>
<br>
        return(0)<br>
}<br>
<br>
# Function `reader' accepts a reader's primary key (`rd_key') and<br>
# returns the reader's name (`rd_name'). If the passed reader's<br>
# key is invalid or the reader is not found in the database, then<br>
# an error message is printed and an empty string is returned.<br>
<br>
function reader(rk,                     data) {<br>
        if (rk !~ /^[ \t]*[0-9][0-9]*[ \t]*$/) {<br>
                print SPAWKINFO["program"] ": " wk \<br>
                        ": invalid `rd_key' value" &gt;"/dev/stderr"<br>
                return(data[1])<br>
        }<br>
<br>
        spawk_select("SELECT `rd_name` FROM `library`.`reader` " \<br>
                "WHERE `rd_key` = " rk)<br>
        if (!spawk_first(data))<br>
                print SPAWKINFO["program"] ": " rk \<br>
                        ": reader not found" &gt;"/dev/stderr"<br>
<br>
        return(data[1])<br>
}<br>
</code></pre>
<p>
Let's say we saved the above awk library in <code>/apps/library/lib/util.awk</code> file. Then the <code>borough</code> program will become:<br>
</p>
<pre><code># The following program seeks and prints the delayed boroughed<br>
# books, that is books that must have been returned at specified<br>
# dates (default current date). The "br_date" column is the date<br>
# of boroughing the book, "br_until" is the date when the book<br>
# must be returned to the library and "br_return" is the date<br>
# that the book is actually returned to the library.<br>
#<br>
# The program uses the following external variables, where external<br>
# means variables to set in the command line:<br>
#<br>
#       date    Is the target date. If not set, then the<br>
#               current date is assumed.<br>
#<br>
#       detail  If set to a non empty, non zero valued, then<br>
#               the seeked "borough" records will be printed<br>
#               in detail, that is reader's name and book's<br>
#               tite will be printed. If not set, then raw<br>
#               "borough" data will be printed (usually to<br>
#               post format with other, most sophisticated<br>
#               report processing programs).<br>
<br>
BEGIN {<br>
        extension("libspawk.so", "dlload")<br>
        SPAWKINFO["program"] = "borough"<br>
        SPAWKINFO["database"] = "library"<br>
<br>
        # Now it's time to check/set the target date<br>
<br>
        set_date()<br>
<br>
        # Run the query to select and print the target<br>
        # "borough" records and exit (no input will be<br>
        # processed).<br>
<br>
        borough()<br>
        exit(0)<br>
}<br>
<br>
function borough(                       data) {<br>
        # Here follows the query to select the target "borough"<br>
        # records. The only thing that can produce an error in<br>
        # that query is a wrong "date" value. We use that simple<br>
        # fact in order to run the query and in the same time<br>
        # check the "date" value.<br>
<br>
        if (spawk_update("SELECT `br_reader`, `br_copy`, " \<br>
                "`br_date`, `br_until` FROM borough " \<br>
                "WHERE br_return IS NULL AND br_until &lt; " date)) {<br>
                print date ": invalid date" &gt;"/dev/stderr"<br>
                exit(2)<br>
        }<br>
<br>
        # The query passed the "date" value check, so convert the<br>
        # query from an "update" query to a "select" query and<br>
        # read the results ("borough" records).<br>
<br>
        spawk_results()<br>
        while (spawk_data(data)) {<br>
                # Based on the "detail" variable print the<br>
                # selected "borough" data as raw data ("detail"<br>
                # not set), or as detailed data ("detail" set).<br>
<br>
                if (detail)<br>
                        print_borough(data)<br>
                else<br>
                        print data[0]<br>
        }<br>
}<br>
<br>
# The "print_borough" function accepts a "borough" returned data<br>
# array and prints the reader's name, the book's title and the<br>
# date that the book must have been returned to the library.<br>
<br>
function print_borough(borough, data) {<br>
        print "Reader: " reader(borough[1])<br>
        get_copy_data(borough[2], data, 1)<br>
        print "Book..: " data["title"]<br>
        print "Date..: " borough[3] " - " borough[4]<br>
        print ""<br>
}<br>
<br>
# Function "set_date" is a initialization function to check/set<br>
# the target date. If the "date" (external) value has been set<br>
# (in the command line), then that date will be used as the<br>
# target date, else the current date is assumed.<br>
<br>
function set_date(                              data) {<br>
        if (!date) {<br>
                spawk_select("SELECT DATE_FORMAT(CURDATE(), '%Y%m%d')")<br>
                spawk_first(data)<br>
                date = data[1]<br>
        }<br>
}<br>
</code></pre>
<p>
To run the program we must "include" the <code>util.awk</code> file:<br>
</p>
<pre><code>awk -f /apps/library/lib/util.awk -v detail=yes -f borough.awk<br>
</code></pre>
<p>
Following similar techniques we can write very complicated programs in a few lines of code. Using shell scripts and the <b><code>AWKPATH</code></b> environment variable, the code can be shorten even more! You must try really hard to stretch SPAWK...<br>
</p>

<a href='Tutorial.md'>Top</a>
<a href='http://code.google.com/p/spawk/wiki/Tutorial?show=content,meta'>Print</a>

<hr />
<i>To be continued...</i>