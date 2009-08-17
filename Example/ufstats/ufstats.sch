CREATE DATABASE ufstats;
USE ufstats   			;

-- Table "cuser" stands for [c]computer [user] and will hold
-- a record for each user found in the password file. If a
-- user is not found during a files count cycle (scan), then
-- that user will be marked as deleted by means of the "cu_delete"
-- column; that column will be left NULL for users found in the
-- password file, while for users not found in the password file
-- the date/time of scan will be filled in "cu_delete" column. The
-- "cu_info" column contains the corresponding password file info field.

CREATE TABLE cuser (
    cu_login CHARACTER(32) NOT NULL PRIMARY KEY,
    cu_info CHARACTER(128) NOT NULL,
    cu_delete DATE
) ENGINE = "INNODB";

-- Table "fscan" holds a record for each file scan executed. The
-- "fs_start" column is the begin date/time of the scan and will
-- be used as a primary key as there will be references to that
-- table, whereas "fs_end" holds the date/time of the scan end.
-- Column "fs_end" may accept NULL values while the scan is in
-- progress.


CREATE TABLE fscan (
    fs_start DATETIME NOT NULL,
    PRIMARY KEY USING BTREE (
        fs_start
    ),
    fs_end DATETIME
) ENGINE = "INNODB";

-- Table "fcount" holds the per user files' count statistics for
-- each executed scan. The table has only three columns, namely
-- the "fc_scan" column which is the scan's date/time (primary
-- key of the "fscan" table), the "fc_user" column which is,
-- of course, the login name of each user (primary key of the
-- "cuser" table) and, finally, the file count for that scan
-- and user.

CREATE TABLE fcount (
        fc_scan DATETIME NOT NULL, 
        FOREIGN KEY (
                fc_scan
        ) REFERENCES fscan (
                fs_start
        ),
        fc_user CHARACTER(32) NOT NULL,
        FOREIGN KEY (
                fc_user
        ) REFERENCES cuser (
                cu_login
        ),
        fc_files NUMERIC(9) NOT NULL
) ENGINE = "INNODB";
