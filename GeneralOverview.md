[Print](http://code.google.com/p/spawk/wiki/GeneralOverview?show=content,meta)

  * [Introduction](GeneralOverview#Introduction.md)
  * [Description](GeneralOverview#Description.md)
  * [Function's Reference](GeneralOverview#Function's_Reference.md)
  * [SPAWKINFO Array Elements](GeneralOverview#SPAWKINFO_Array_Elements.md)
    * [SSL Support](GeneralOverview#SSL_Support.md)
  * [Configuration Files](GeneralOverview#Configuration_Files.md)
  * [Initialization Scripts](GeneralOverview#Initialization_Scripts.md)
<u>
<h1>Introduction</h1>
</u>
<p>
<b>S</b>QL <b>P</b>owered <b>AWK</b> is about accessing and manipulating <i>MySQL</i> databases from within <i>awk</i> programs. <i>MySQL</i> is a well known, open source database management system, while <i>awk</i> is the programmer's swiss army knife in UNIX/Linux platforms.<br>
</p>
[Top](GeneralOverview.md)
<u>
<h1>Description</h1>
</u>
<p>
SPAWK is implemented via the GNU awk's <i>extension</i> libraries. GNU awk programs can extend the standard awk function set using some kind of shared libraries, called <i>extension</i> libraries. To use functions from an extension library we must first "load" the library using the standard awk <code>extension</code> function, e.g. if we want to use functions from an extension library called <code>payroll</code>, then somewhere in the awk program (usually in the <code>BEGIN</code> section, but before calling payroll functions, of course) we must call <code>extension</code> to load the library:<br>
</p>
```
BEGIN {
    ...
    extension("payroll", "dlload")
    ...
```
<p>
After calling <code>extension</code> we can call functions included in the specified library just like we call any standard awk function, e.g. <code>index</code>, <code>asorti</code> etc. The <code>"dlload"</code> second argument in the <code>extension</code> call above, is standard, so don't bother of it; just write the extension library code following the rules in <b>Arnold Robbins</b>' "<i>Effective Awk Programming</i>" and you shall not encounter any technical problems.<br>
</p>
<p>
The SPAWK extension library contains the following functions: <code>spawk_program</code>, <code>spawk_query</code>, <code>spawk_select</code>, <code>spawk_update</code>, <code>spawk_results</code>, <code>spawk_data</code>, <code>spawk_first</code>, <code>spawk_last</code>, <code>spawk_clear</code>, <code>spawk_affected</code>, <code>spawk_string</code>, <code>spawk_errno</code>, <code>spawk_error</code>, <code>spawk_server</code> and <code>spawk_debug</code>. Those elements constitute an elegant and very small sized collection of functions enabling the awk programmer to "talk" with MySQL databases. The SPAWK functions are based on the standard <a href='http://dev.mysql.com/doc/refman/5.0/en/c.html'>C API</a> of MySQL, that is the main C function interface for establishing and using connections (clients) to MySQL databases (servers) over normal or encrypted local or network lines.<br>
</p>
<p>
Of major interest in SPAWK is also the <code>SPAWKINFO</code> awk array. We can use <code>SPAWKINFO</code> elements to handle various parameters playing major role in database connections, e.g. the default database (schema) to connect, the default user name and password, etc. The <code>SPAWKINFO</code> elements will be described in detail later.<br>
</p>
<p>
Also the MySQL configuration files (<code>/etc/my.cnf</code> (system wide) and local <code>~/.my.cnf</code> (per user)) play significant role in SPAWK. Actually, the <code>[client]</code> and <code>[spawk]</code> sections will be taken in to account, as any <code>[</code><i><code>program</code></i><code>]</code> section to be used for specific programs. That is we can set default connection parameters using configuration files, just like other MySQL client and server programs do.<br>
</p>
[Top](GeneralOverview.md)
<u>
<h1>Functions' Reference</h1>
</u>
<p>
Here follows a somewhat detailed SPAWK functions' description:<br>
</p>

---

<b>
<code>spawk_program([program])</code>
</b>

---

<p>
The <code>spawk_program</code> function sets the program's name to be used in error messages and such. It's a good thing to call that function just after loading the SPAWK module, e.g.<br>
</p>
```
BEGIN {
    extension("libspawk.so", "dlload")
    SPAWKINFO["host"] = "roadrunner"
    SPAWKINFO["database"] = "forex"
    spawk_program("willex")
    ...
```
<p>
We can call <code>spawk_program</code> without the <code>program</code> argument. In that case the program's name will be set to <code>spawk</code>.<br>
We can also set the program's name using the <code>SPAWKINFO</code> "<code>program</code>" indexed element. The above example would be:<br>
</p>
```
BEGIN {
    extension("libspawk.so", "dlload")
    SPAWKINFO["host"] = "roadrunner"
    SPAWKINFO["database"] = "forex"
    SPAWKINFO["program"] = "willex"
    ...
```
<p>
We can call <code>spawk_progam</code> any number of times, thus changing program's name accordingly. Anyway, <code>spawk_program</code> function is not of much interest and you can ignore its use for a while.<br>
</p>
<u>Return Value</u><br>
<code>spawk_program</code> returns the current program's name, that is the name of the program just before the <code>spawk_program</code> function call.<br>
<hr />
<b>
<code>spawk_query(query)</code>
</b>
<hr />
<p>
Function <code>spawk_query</code> is used to add query portions to the current server's query. In other words, you can call <code>spawk_query</code> repeatedly giving blocks of the <i>select</i> or <i>update</i> query, thus constructing the final query for the current server. After completing the query you can either call <code>spawk_select</code> or <code>spawk_update</code> to execute the query; if the query just selects data, then call <code>spawk_select</code> to execute, else <code>spawk_update</code> must be called. You can also call <code>spawk_update</code> just to check the validity of a <i>select</i> query and then prepare the results' set using <code>spawk_results</code>.<br>
</p>
<p>
The reason not to call <code>spawk_select</code> or <code>spawk_update</code> from the begining is twofold:<br>
<ul><li>The query is huge and is better to give it in pieces.<br>
</li><li>Portions of the query are optional, i.e. the query is not fixed, but we check some conditions before add one portion of the query or another.<br>
</p>
<u>Return Value</u><br>
This function returns the server number of the receiving server, i.e. the current server.<br>
<hr />
<b>
<code>spawk_update([query [, status]])</code>
</b><br>
<b>
<code>spawk_affected()</code>
</b>
<hr />
<p>
Calling <code>spawk_update</code> is one of two ways to execute a query via SPAWK module (the other is <code>spawk_select</code>). The last part of the query to execute may be given as the first argument of <code>spawk_update</code>. You can of course pass the whole query, or pass no query at all. In any case, the query to be executed is the query accumulated by <code>spawk_query</code> previous calls plus the query passed as first argument to <code>spawk_update</code>. The query may not return any results; if the query returns data, then <code>spawk_select</code> must be used instead. However, it's not an error to execute a <i>select</i> query via <code>spawk_update</code> function, and actually this is the only way to check the validity of a <i>select</i> query; after checking the validity of a <i>select</i> query via <code>spawk_update</code> you can prepare the results calling the <code>spawk_results</code> function to convert the already executed query from an <i>update</i> query to a <i>select</i> query.<br>
</p>
<p>
After calling <code>spawk_update</code> you can call <code>spawk_affected</code> to get the number of rows affected due to that update. Rows considered affected in MySQL terms, that is only the rows really changed considered affected. Given, for example, the query: "<code>UPDATE person SET tax = 0 WHERE byear &lt; 1990</code>" the affected number of rows are not all the <code>person</code> rows with <code>byear</code> < 1990, but all of these rows where <code>tax</code> wasn't valued 0, that is all the rows actually changed. We can differentiate that behavior using <code>SPAWKINFO</code> array once again. To specify that <code>spawk_affected</code> must return rather the number of all rows invlolved in updates than just the number of the rows actually changed, we must create a "<code>CFR</code>" indexed element in <code>SPAWKINFO</code> before loading the SPAWK module. Have in mind, however, that changing the default MySQL affected row count policy will have impact in the overall database server efficiency.<br>
</p>
<u>Return Value</u><br>
Function <code>spawk_update</code> returns zero for successful query execution. If the query fails, then the MySQL error code (number) will be returned. If passed a second argument, the program exits on query failure instead of returning the MySQL error code; that second argument (if passed) must be a number which will be used as the program exit status in case of failure; this number may be any integer number (including zero).<br>
<hr />
<b>
<code>spawk_select([query [, index]])</code>
</b>
<hr />
<p>
Function <code>spawk_select</code> is the second of two ways to execute queries via SPAWK module. It's used for <i>select</i> queries only, that is only for queries that produce some data, e.g. "<code>SELECT...</code>", "<code>SHOW...</code>" etc. The first argument is the last part of the query to be executed. This string is concatenated with previous query parts given with previous <code>spawk_query</code> calls and forms a complete query which is executed to produce results. We can, of course, pass a whole query or pass no query at all (or an empty string) in case the query have been given with previous <code>spawk_query</code> calls.<br>
</p>
<p>
The second argument is a bit more complicated. Data produced from SPAWK queries are returned to the calling awk processes in arrays indexed as follows: the whole record is indexed 0, the first field (column) is indexed 1, the second field is indexed 2 etc. That is a number indexing scheme just like the scheme used by awk to number the input line fields. However, this scheme may be altered using a second argument in <code>spawk_select</code> function. If you pass a second argument to <code>spawk_select</code> function, then this argument must be a comma separated string consisting of the indices to be used for the returned fields, e.g. given the "<code>key,lname,fname,pname,zip</code>" and the query "<code>SELECT key, lname, fname, zip_code FROM person WHERE Dept LIKE '02B%'</code>" the returned data array will be indexed as: "<code>key</code>" for the <code>key</code> column, "<code>lname</code>" for the <code>lname</code> column etc. The whole record will be indexed with the empty string ("").<br>
</p>
<u>Return Value</u><br>
The number of fields (columns) of the select query returned lines (rows). A value of 0 signals end of data.<br>
<hr />
<b>
<code>spawk_results([index])</code>
</b>
<hr />
<p>
Function <code>spawk results</code> converts an already executed <i>update</i> query to a <i>select</i> query. Actually the query has already been executed via <code>spawk_update</code> function and you use <code>spawk_results</code> to prepare the data to be returned. You could, of course, had executed the query via <code>spawk_select</code> right from the begining, but there is no way to check the validity of the <i>select</i> query via <code>spawk_select</code>; if the query fails, the program just exits with non zero exit code. For guaranteed hard coded <i>select</i> queries, there is no need to call <code>spawk_results</code> ever; use <code>spawk_select</code> instead. Function <code>spawk_results</code> takes one optional argument which is a comma separated index string for the index scheme of the returned data. For a description on returned data index schemes, take a look on <code>spawk_select</code> description.<br>
</p>
<u>Return Value</u><br>
The number of fields (columns) of the converted query returned lines (rows).<br>
<hr />
<b>
<code>spawk_data([data [, null]])</code>
</b><br>
<b>
<code>spawk_first([data [, null]])</code>
</b><br>
<b>
<code>spawk_last([data [, null]])</code>
</b><br>
<b>
<code>spawk_clear([all])</code>
</b>
<hr />
<p>
Function <code>spawk_data</code> fetches rows returned from SPAWK queries. After initiating <i>select</i> queries via <code>spawk_select</code> or <code>spawk_results</code> function calls, you can (actually you must) fetch the returned rows via repeatitive <code>spawk_data</code> function calls. While returning data, SPAWK servers are considered busy. That means that new servers will pop up for queries given during the process of data return. That's why you must fetch all the returned data rows. Of course there are some other SPAWK functions to discard the rest of the returned rows at any time (<code>spawk_clear</code>), or fetch only the first or last of the returned rows (<code>spawk_first</code> and <code>spawk_last</code> functions respectively) and discard the rest of the returned rows.<br>
</p>
<p>
The <code>spawk_data</code> function accepts two arrays as arguments: the first array will be filled with the returned data column values for each returned row in turn. The indexing scheme is specified via <code>spawk_select</code> or <code>spawk_results</code> function calls and defaults to number indexing, that is 0 for the whole row, 1 for the first returned column, 2 for the second etc. If you specify a comma separated string for a deferred indexing scheme via <code>spawk_select</code> or <code>spawk_results</code> function calls, then the whole returned row will be indexed with the empty string (""), and the returned columns will be indexed according to the given index string values, e.g. the given the index string "<code>key,lname,fname,zip</code>" the indexes will be: "" for the whole returned row, "<code>key</code>" for the first column of the returned row, "<code>lname</code>" for the second etc.<br>
</p>
<p>
Null valued columns will be returned as empty strings or set to the string specified with the <code>SPAWKINFO</code> "<code>null</code>" indexed specified element, e.g. if <code>SPAWKINFO["null"]</code> is set to "<code>NIL</code>", then null valued columns will be returned as "<code>NIL</code>" valued elements in the corresponding elements, and as "<code>NIL</code>" strings embedded in the whole returned row. By the way, awk's default output field separator <code>OFS</code> is used as field separator in the whole returned row, except if <code>SPAWKINFO["OFS"]</code> has been set to another string; in that case that string is used as the returned data field separator. As you can see, <code>SPAWKINFO</code> array plays a major role in SPAWK data return procedure.<br>
</p>
<p>
If passed a second argument, then this is assumed to be another array to hold just the null valued columns. That means that only the null valued columns will be returned in the specified array, using the same indexing scheme as that used for the returned data. Elements in the, so called, null return array have all the value of 1.<br>
</p>
<u>Return Value</u><br>
Functions <code>spawk_data</code>, <code>spawk_first</code> and <code>spawk_last</code> return the number of the returned columns or zero if there are no more rows to fetch. Function <code>spawk_clear</code> returns the number of the current server after the call, i.e. the server to receive the next coming query.<br>
<hr />
<b>
<code>spawk_string(string)</code>
</b>
<hr />
<p>
Function <code>spawk_string</code> is a utility function used to convert strings involved in queries, to safe to use strings. Actually, there are special characters that may cause problems when strings containing such characters are involved in queries. Such characters are double and single quotes, backslashes etc. The string returned by <code>spawk_string</code> is safe to use in queries. Maybe it's a good practice to always call <code>spawk_string</code> when the strings involved are not hard coded, but if there is no reason to do so, don't overdo. However, you can always use other awk functions as well like <code>sub</code>, <code>gsub</code> etc.<br>
</p>
<u>Return Value</u><br>
Function <code>spawk_string</code> returns the converted string.<br>
<hr />
<b>
<code>spawk_errno()</code>
</b><br>
<b>
<code>spawk_error()</code>
</b>
<hr />
<p>
Functions <code>spawk_errno</code> and <code>spawk_error</code> form interfaces to <code>mysql_errno</code> and <code>mysql_error</code> MYSQL C API functions respectively. <code>spawk_errno</code> returns the error code of the last query given to the current server, or zero if no error occurred, whereas <code>spawk_error</code> returns a description of the corresponding error.<br>
</p>
<u>Return Value</u><br>
Function <code>spawk_errno</code> returns a number (error code), whereas <code>spawk_error</code> returns a string (error description).<br>
<hr />
<b>
<code>spawk_server()</code>
</b>
<hr />
<p>
Function <code>spawk_server</code> returns the current server. That is the number of the server currently accepting queries. Usually this number is 0. When this server is busy returning results and a new query is given, this number will be increased to 1 and so on.<br>
</p>
<u>Return Value</u><br>
Function <code>spawk_server</code> returns the current server as a number.<br>
<hr />
<b>
<code>spawk_debug([file [, create]])</code>
</b>
<hr />
<p>
Function <code>spawk_debug</code> is used mainly for query logging. To activate debugging, call <code>spawk_debug</code> with an argument passed, namely the name of a file to be used as query log (or generally debug roll). If passed a second argument too (even empty string or zero number), then the file will be truncated if exists, else debug information will be appended to the file. When calling the function passing no arguments at all, debugging is suspended. We can later reactivate debugging and so on.<br>
</p>
<u>Return Value</u><br>
Always returns zero (number).</li></ul>

<a href='GeneralOverview.md'>Top</a>
<u>
<h1>SPAWKINFO Array Elements</h1>
</u>
<p>
<b><code>SPAWKINFO</code></b> array plays a major role in SPAWK. You can change SPAWK's behavior adding or setting specific elements in the <code>SPAWKINFO</code> array, e.g. to specify which schema to use when establishing a new connection (default database) just set the <code>database</code> indexed element to the desired schema, or if you want to set the default user name to connect just set the <code>user</code> indexed element, etc. Here follows a list of the all the special indices of <code>SPAWKINFO</code> array having to do with SPAWK:<br>
</p>
<u><b>version</b></u><br>
The "version" indesxed <code>SPAWKINFO</code> element contains a string showing the version of the SPAWK module at your local system.<br>
<br>
<u><b>program</b></u><br>
The "program" indesxed <code>SPAWKINFO</code> is the program's name as setted by the <code>spawk_program</code> function call.<br>
<br>
<u><b>host</b></u><br>
The "host" indexed <code>SPAWKINFO</code> element is the domain name of the database host machine, e.g. "roadrunner", "jaguar", "www.mucho.com@apd" etc.<br>
<br>
<u><b>database</b></u><br>
The "database" indexed <code>SPAWKINFO</code> element is the default database (schema) to connect for each new SPAWK server. This is the only <code>SPAWKINFO</code> element that is server specific; if we change the value of the "database" indexed <code>SPAWKINFO</code> element before a new server opens, then this new server will connect to the specified database at that time.<br>
<br>
<u><b>user</b></u><br>
The user name of the user to access the database. That name is the name used by MySQL to identify the user. Usually that name coincides with the user's login name. If not specified (<code>SPAWKINFO</code> "user" indexed element doesn't exist), the login name of the user is assumed.<br>
<br>
<u><b>password</b></u><br>
The database password for the user accessing the database. If not specified and the user needs to supply a password when accessing the database, then the SPAWK user will be prompted for the password needed.<br>
<br>
<u><b>password_prompt</b></u><br>
Used as a prompt when the user is prompted for the database password.<br>
<br>
<u><b>port</b></u><br>
The port number to be used for the connection.<br>
<br>
<u><b>socket</b></u><br>
The name of the socket file to be used for the connection.<br>
<br>
<u><b>OFS</b></u><br>
The column separator string used in returned rows of data. As said before, the servers return data to the awk processes via RDA and RNA. RDA stands or Returned Data Array, while RNA stands for Returned Null Array. The whole row is returned in RDA's 0 or "" indexed element as a string consisting of the columns separated by the <code>SPAWKINFO["OFS"]</code> string. If not defined, then the OFS awk's default output field separator is being used instead.<br>
<br>
<u><b>null</b></u><br>
Null valued columns are returned as empty strings in the RDA. We can define an arbitrary string to be used for null valued columns using <code>SPAWKINFO["null"]</code> element.<br>
<br>
<u><b>max_query_len</b></u><br>
Is the maximum query length allowed for SPAWK queries. The default value is large enough to hold huge queries. To handle even larger queries we have to increase that value.<br>
<br>
<u><b>max_row_len</b></u><br>
The maximum length of the returned data rows. If the returned rows are of huge length, then you must increase that value.<br>
<br>
<u><b>CFR</b></u><br>
If there is a "CFR" indexed element in <code>SPAWKINFO</code> at the time SPAWK module is loaded, then the affected number of rows in updates is considered to be the number of all the rows involved in the updates, else the default (in MySQL terms) number of actually changed rows is considered as affected number of rows.<br>
<br>
<a href='GeneralOverview.md'>Top</a>
<u>
<h2>SSL Support</h2>
</u>
SPAWK supports SSL connections, that is secure line connections. The <code>SPAWKINFO</code> elements affecting SSL are indexed as follows:<br>
<br>
<u><b>ssl-key</b></u><br>
The name of the SSL key file to use for establishing a secure database connection.<br>
<br>
<u><b>ssl-ca</b></u><br>
The path to a file that contains a list of trusted SSL CAs.<br>
<br>
<u><b>ssl-capath</b></u><br>
The path to a directory that contains trusted SSL CA certificates in PEM format.<br>
<br>
<u><b>ssl-cert</b></u><br>
The name of the SSL certificate file to use for establishing a secure connection.<br>
<br>
<u><b>ssl-cipher</b></u><br>
A colon separated list of allowable ciphers to use for SSL encryption.<br>
<br>
<u><b>ssl</b></u><br>
If set to zero (0), then connect over a non SSL connection. If set to one (1) use SSL. There is no need to set this variable. When other SSL variables are set, then SSL is used,  else SSL is inactive.<br>
<br>
<u><b>skip-ssl</b></u><br>
If set (no value needed), then SSL is turned off, either if other SSL variables have been set.<br>
<br>
<a href='GeneralOverview.md'>Top</a>
<u>
<h1>Configuration Files</h1>
</u>
<p>
You can also configure SPAWK's behavior globally using the standard MySQL configuration files. MySQL uses the file <code>/etc/my.cnf</code> for global settings and <code>~/.my.cnf</code> files for per user settings (<code>~</code> stands for the users' home directories), e.g. if your login name is <code>panos</code>, then the files examined will be: <code>/etc/my.cnf</code> and <code>/home/panos/.my.cnf</code> in that order; that is if a parameter is set in <code>/etc/my.cnf</code> and also set in <code>/home/panos/.my.cnf</code>, then the user's setting is used.<br>
</p>
<p>
MySQL configuration files are structured in <i>sections</i>. Each section affects a group of programs or specific programs, e.g. the <code>[client]</code> section affects all the client programs, while the <code>[payroll]</code> section affects only the <code>payroll</code> program (if, of course, such a program exists). The sections interaction in SPAWK are <code>[client]</code> and <code>[spawk]</code> sections in that order. If you name your program using <code>spawk_program</code> function or the <code>program</code> indexed element of the <code>SPAWKINFO</code> array, then you can add a section with the same name and this section takes higher priority, e.g. if you run a SPAWK program named "payroll", then you can add a <code>[payroll]</code> section in your configuration files in order to set various parameters for that specific program. Here follows an example configuration file:<br>
</p>
<pre><code>...<br>
# The following options will be passed to all MySQL clients<br>
[client]<br>
host            = "aplha"<br>
port            = 3306<br>
socket          = /var/lib/mysql/mysql.sock<br>
...<br>
[mysql]<br>
no-auto-rehash<br>
...<br>
# The following options will be passed to all SPAWK programs.<br>
[spawk]<br>
host            = "spock"<br>
password_prompt = "Enter SPAWK password "<br>
...<br>
[payroll]<br>
host            = "roadrunner"<br>
database        = "payroll"<br>
password_prompt = "Please enter your payroll password "<br>
...<br>
</code></pre>
<p>
Given the above configuration file, the user will be connected to the host "alpha" when running any MySQL client program. However, when that program is a SPAWK program, then the password prompt will be "Enter SPAWK password" and the user will be connected to the "spock" host (let's say a machine for testing and developing the SPAWK module itself). Moreover, if the SPAWK program is named "payroll", then the password prompt will become "Please enter your payroll password" and the default database to connect will be the "payroll" schema at the "roadrunner" host.<br>
</p>
<p>
If a parameter is set literally in the configuration files and the same parameter is also set in the <code>SPAWKINFO</code> array, then the array value is used. Also, all of the parameters set via the configuration files will be inserted in the <code>SPAWKINFO</code> array. All those are of great importance in SPAWK, so you have to experiment a lot to understand the SPAWK interaction with the configuration files and <code>SPAWKINFO</code> array.<br>
</p>
<a href='GeneralOverview.md'>Top</a>
<u>
<h1>Initialization Scripts</h1>
</u>
<p>
When connecting to a MySQL database via SPAWK you can run initialization SQL commands. The commands can be written in global (<code>/etc/spawkrc</code>) and per user (<code>~/.spawkrc</code>) initialization scipts (files). The contents of those initialization files are plain SQL queries separated with "<code>;</code>" characters, just like <code>mysql</code> client scripts. Every time a new SPAWK server (actually a SPAWK server is a database connection) is opened, those commands will be executed in order; first the <code>/etc/spawkrc</code> command are executed, then the <code>~/.spawkrc</code> commands. You can include almost any SQL commands in the initialization scripts, but be aware of the fact that initialization scripts shall not contain queries that produce any results! An example initialization script follows:<br>
</p>
<pre><code>USE payroll;<br>
SET @max_sal = 100000;<br>
SET @max_tax = 2000;<br>
</code></pre>
<p>
Whenever a new SPAWK server comes into life (recall that a SPAWK server is just a database connection), the above commands will be executed first. Thus, the default schema will become "payroll", whereas the two SQL variables <code>max_sal</code> and <code>max_tax</code> will be set to <code>100000</code> and <code>2000</code> respectively. <u>If any of the commands of an initialization script fails to execute, then an error message is printed and the awk process running SPAWK exits with a non zero exit status</u>.<br>
</p>

<a href='GeneralOverview.md'>Top</a>
<a href='http://code.google.com/p/spawk/wiki/GeneralOverview?show=content,meta'>Print</a>
<hr />

<i>To be continued...</i>