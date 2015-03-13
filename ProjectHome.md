[SPAWK](http://sites.google.com/site/spawkinfo) is an elegant collection of functions for accessing and manipulating [MySQL](http://mysql.com) databases from within [awk](http://awk.info) programs. SPAWK is based on MySQL's [C API](http://dev.mysql.com/doc/refman/5.0/en/c.html) and awk's sources. It's all about a dozen of functions grouped into a GNU awk (gawk) "extension" library. The following example shows how to use SPAWK for printing all tables and columns of all schemas in your MySQL database:
```
BEGIN {
    extension("libspawk.so", "dlload")
    SPAWKINFO["database"] = "information_schema"
    SPAWKINFO["OFS"] = "."
    spawk_query("SELECT TABLE_SCHEMA, TABLE_NAME FROM TABLES")
    spawk_select()
    while (spawk_data(table)) {
        print table[0]
        spawk_query("SELECT COLUMN_NAME FROM COLUMNS " \
            "WHERE TABLE_SCHEMA = '" table[1] \
            "' AND TABLE_NAME = '" table[2] "'")
            spawk_select()
            while (spawk_data(column))
                print "\t" column[1]
    }

    exit(0)
}
```