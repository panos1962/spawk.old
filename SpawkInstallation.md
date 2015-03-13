[Print](http://code.google.com/p/spawk/wiki/SpawkInstallation?show=content,meta)

  * [Introduction](SpawkInstallation#Introduction.md)
  * [Installation Examples](SpawkInstallation#Installation_Examples.md)
    * [CentOS5.3](SpawkInstallation#CentOS5.3.md)
    * [Fedora11](SpawkInstallation#Fedora11.md)
    * [Ubuntu9.04](SpawkInstallation#Ubuntu9.04.md)

<u>
<h1>Introduction</h1>
</u>

[SPAWK](http://sites.google.com/site/spawkinfo) module can be installed to your Linux/UNIX machine by following some simple steps. You can just download the binaries (`libspawk.so` and `libspawk_r.so`) given for various famous Linux distributions (32-bit, x86), but in order to avoid version conflicts ([gawk](http://awk.info) and [MySQL](http://mysql.com) are involved), it's better to clone the current source version and relink the libraries in your local machine. To do so you must have [GCC](http://gcc.gnu.org) (the GNU Compiler Collection) installed in your local machine. You must also have MySQL client libraries and include files installed as well as the [gawk](http://ftp.gnu.org/gnu/gawk/) (GNU awk) sources of your local awk version installed somewhere in your local machine. All this may sounds like too much work for you but, believe me, it's not. Give it a try!

<u>
<h1>SPAWK Installation Instructions</h1>
</u>

<p>
The easiest way for downloading SPAWK module is downloading the binaries. Actually there is only one binary, namely a shared gawk <i>extension</i> library named <code>libspawk.so</code>, but there is a threads safe companion named <code>libspawk_r.so</code> provided aswell. These two binaries are grouped in tarballs for various Linux distributions. You can download these tarballs, marked as <font color='maroon'><b>BT</b></font> (<font color='maroon'><b>B</b></font>inary <font color='maroon'><b>T</b></font>arball), from the <a href='http://code.google.com/p/spawk/downloads/list'>Downloads</a> tab. If the binaries don't match your Linux distribution, MySQL or gawk versions, then it's recommended to download the SPAWK sources and build the SPAWK modules. This is somewhat more complicated, but it's still a simple step by step procedure.<br>
</p>

<u>
<h2>Downloading Binaries</h2>
</u>

If you choose to download the binaries given in [SPAWK's Downloads](http://code.google.com/p/spawk/downloads/list), then you must check your Linux distribution to match the closest tarball. Run:
```
mysql --version
gawk --version
```
and choose the right SPAWK binary tarball, e.g. `fedora.10.tar.gz`. After downloading the tarball (just click it), login as root (or run `sudo -i`) and seek the downloaded tarball. Let's say you find the tarball in `/home/panos/Downloads`. Run:
```
tar xxPf /home/panos/Downloads/fedora.tar.gz
```
and the SPAWK module is ready for use! The tarball is no longer needed, so you can remove it.

First thing is to download the SPAWK source files and directories. There are two methods of downloading the SPAWK sources to your local machine:
  * Clone the project using _mercurial_. However, if you're not a VCS expert, then choose another method!
  * Download the SPAWK source tarball and extract the files. That's the easy way!

<u>
<h2>Cloning the SPAWK project</h2>
</u>

In order to install the SPAWK module from the source you have to clone the SPAWK project from the [GoogleCode](http://code.google.com/p/spawk) repository. Just login as root in your local machine (assuming that you have a good, functional internet connection, of course) and position yourself in a non-system directory; `/root` is a good place. Then run the following command:
```
hg clone https://spawk.googlecode.com/hg/ spawk
```
This is the right way to clone the whole SPAWK project to your local machine. It's possible not to have `hg` program installed in your local system. That's the [mercurial](http://mercurial.selenic.com) VCS client program. If that's the case, you have to install mercurial in your local machine. That can be done either by `yum`, or by `apt-get`, or via GUI tools like `synaptic`. Let's say we use `yum` in _CentOS_, or _Fedora_:
```
yum install mercurial
```
After installing the mercurial package, test the `hg` program:
```
hg
```
If you get any error messages about missing python libraries etc, then run:
```
yum istall python-setuptools
easy_install mercurial
```
In _Ubuntu_ you can use `apt-get`:
```
apt-get install mercurial
```
After installing `hg` in your local machine, run the clone command above for a second time. If, however, you encounter problems downloading `mercurial`, then you can always download the spawk tarball from http://code.google.com/p/spawk repository.

<u>
<h2>Downloading the SPAWK tarball</h2>
</u>
Login as `root` to your local machine. Using your favorite browser, enter http://code.google.com/p/spawk and download the tarball to your local machine. Let's say you download the file as `/root/Desktop/spawk-2.4.1.tar.gz`. Move yourself to a non-system directory (`/root` is a good place) and extract the SPAWK source files and directories from the tarball you've just downloaded, e.g.:
```
cd /root
tar xzf /root/Desktop/spawk-2.4.1.tar.gz
```
The SPAWK source files and directories must be there!

<u>
<h2>Verifying SPAWK files and directories</h2>
</u>

You must have succeeded by now and, whatever downloading method you've chosen, there must exist a `spawk` directory in your local machine, let's say `/root/spawk-2.4.1`. That's your local `spawk` project clone. Position yourself into that directory and run `ls`:
```
cd /root/spawk-2.4.1
ls -l
```
You'll see something like:
```
drwxr-xr-x 2 panos panos 4096 2009-08-09 09:59 bin
-rw-r--r-- 1 panos panos 1992 2009-08-08 12:49 configure
drwxr-xr-x 2 panos panos 4096 2009-08-09 09:59 lib
-rw-r--r-- 1 panos panos 6516 2009-08-05 19:09 README
drwxr-xr-x 2 panos panos 4096 2009-08-09 09:59 Sample
drwxr-xr-x 2 panos panos 4096 2009-08-09 09:59 src
drwxr-xr-x 2 panos panos 4096 2009-08-09 09:59 src.stable
drwxr-xr-x 2 panos panos 4096 2009-08-09 09:59 Test
drwxr-xr-x 2 panos panos 4096 2009-08-09 09:59 tools
```
You are in the SPAWK project main directory and all (or most) of the files you see here are needed to construct the desired SPAWK libraries. To construct (compile and link) the libraries you have to follow the following steps:

<u>
<h2>Constructing the <code>Makefile</code></h2>
</u>

First thing in the SPAWK module compilation and linkage procedure is to check your local system and construct a `Makefile`. That job will be carried out by the `configure` shell script:
```
sh configure
```
This will run `configure` shell script to check your system for all the prerequisites in order to construct a `Makefile` which will be used later to construct the libraries. `Makefile`s are just rules to be followed by the `make` GCC tool, which is the standard method for compiling sources and linking objects in Linux/UNIX platforms.

After running `configure`, you may get some error messages, like _library not found_, or _file not found_ etc. If that's the case, you probably have to install some extra packages in your local machine:

<u>
<h3><code>mysql.h: file not found</code></h3>
</u>

There are some files used in the compilation process named _header_ files. Those files contain various statements and other program items (variables' declarations, objects' definitions etc) to be used in the sources being compiled. For SPAWK to be compiled the `mysql.h` of MySQL C API is needed. This file belongs to the MySQL development component. The above message means that you have not that package installed in your local system, so you have to install the package following the usual procedures, e.g. in a _CentOS_ system use `yum`:
```
yum install mysql-devel
```
That must do the job and the `mysql.h` header file will be in the correct place for SPAWK to be compiled with.

<u>
<h3><code>awk.h: file not found</code></h3>
</u>
That's another header file that contains various gawk object definitions, declarations and such. In order to get that file we have to download the sources of our local gawk version. Run:
```
gawk --version
```
to get your local gawk's version. Let's say that our local gawk version is 3.1.7. First move yourself out of the spawk directory; again, `/root` is a good place. Download the corresponding gawk sources:
```
wget --quiet http://ftp.gnu.org/gnu/gawk/gawk-3.1.7.tar.gz
```
After the `wget` command finishes there must be a zipped _tarball_ file in your current directory, e.g. `gawk-3.1.7.tar.gz`. _Tarballs_ are special archives used in Linux/UNIX systems. Extract the contents of the the archive using the `tar` program:
```
tar xzf gawk-3.1.7.tar.gz
```
You'll notice a new `gawk-3.1.7` directory. The `gawk-3.1.7.tar.gz` will no longer be needed, so remove it:
```
rm -f gawk-3.1.7.tar.gz
```
Now move yourself in the gawk directory and run the `configure` shell script:
```
cd gawk-3.1.7
sh configure
```
A lot of messages will appear in your terminal's screen. Ignore them. After `configure` finishes its job, you may notice some "`Now please type '`**`make`**`' to compile. Good luck.`" or such messages on your terminal screen; don't run make if you don't want to reconstruct your gawk executable! Just check for the wanted `awk.h` header file to be present:
```
ls -ld awk.h
```
If the file isn't there, then you have a problem: you can't compile nor link the SPAWK libraries! Abort the job, or try again with some expert friend of yours.

<u>
<h3><code>libmysqlcient.so: library not found</code></h3>
</u>
That kind of message means that the standard client MySQL dynamic (shared) library is missing. You have to download the corresponding package using the usual methods, e.g. in a _Fedora_ system:
```
yum install mysql-libs
```
or, in _Ubuntu_:
```
apt-get install mysql-client
```
The same procedure must be followed for the threads-safe library (`libmysqlclient_r.so`).

All of the prerequisites must now be in place and you're ready to run the SPAWK `configure` command once again:
```
cd /root/spawk
sh configure
```
This time there must be no error messages and the `Makefile` must be present:
```
ls -ld Makefile
```

<u>
<h2>Compiling and Linking the SPAWK Libraries</h2>
</u>

Now run `make` to construct a first version of the SPAWK libraries in the current directory:
```
make
```
The compilation process must finish without errors and the libraries (`libspawk.so` and `libspawk_r.so`) must be present in the current directory.

<u>
<h2>Testing the SPAWK module</h2>
</u>

To test the libraries locally you must have the MySQL server package installed in your local machine. If you're connecting to a remote database, then there's no need to install the server package. Just edit your MySQL `[client]` section in MySQL configuration files and try:
```
make test
```
You must see a cascade of messages on your terminal's screen. That means that the (local) library drafts are functional.

<u>
<h2>Installing and Testing the SPAWK module</h2>
</u>

Now install the libraries in a more "global" place:
```
make install
make cleanup
```
The libraries will be moved in the `/usr/lib` directory of your local system and the local objects will be removed. Now test the final libraries:
```
awk -f Test/test99.awk
```
You must see a cascade of tables' and columns' names coming up to your terminal screen. That's a good sign and it's time for further cleanup:
```
cd /root
rm -rf spawk-2.4.1 gawk-3.1.5
```
<p>
If you know what you're doing, you can remove the MySQL packages as well; However, you must NOT  remove the MySQL client libraries (<code>libmysqlclient.so</code> and/or <code>libmysqlclient_r.so</code> files), because those are dynamically linked (shared) libaries that contain MySQL C API functions to be called from SPAWK functions during runtime!<br>
</p>

[Top](SpawkInstallation.md)

<u>
<h1>Installation Examples</h1>
</u>
<p>
In this chapter we're going to give some example on how to install SPAWK in machines running under Linux operating system. In all installations we assume that no awk sources, nor MySQL packages have been installed; if some of the packages are already installed, then skip the relative steps. Here follows some example installation procedures for various Linux distros:<br>
</p>

[Top](SpawkInstallation.md)

<u>
<h2>CentOS5.3</h2>
</u>
<p>
I've just installed the <i>CentOS5.3</i> Linux distro without installing any <i>MySQL</i> components (server, libs, client etc). I followed the following procedure and managed to install the SPAWK module without a single error:<br>
</p>
  * Login as root.
  * Enter Mozilla Firefox and go to the http://code.google.com/p/spawk URL (SPAWK project).
  * Download the spawk-_2_._4_._1_.tar.gz SPAWK archive and open with the _Archive Manager_ (`tar`).
  * Open a terminal and search the directory where the archive have been extracted. Found it in `/tmp`
    * Position yourself to the SPAWK directory (`cd /tmp/spawk-2.4.1`) and run `sh configure`
  * I got error messages about not found files (`awk.h`, `mysql.h`, etc).
  * Run `yum install -y mysql-devel` to install the _MySQL_ development package. That command installed the missing `mysql.h` file as well as the `libmysqlclient.so` and `libmysqlclient_r.so` files also.
  * Checked the gawk's version (`gawk --version`). Got _3.1.7_.
  * Move to `/tmp` and download gawk sources for found gawk version: `wget http://ftp.gnu.org/gnu/gawk/gawk-3.1.7.tar.gz`
  * Extract the gawk sources: `tar xzf gawk-3.1.7.tar.gz`
  * Move to gawk-3.1.7 (`cd gawk-3.1.7`) and run: `sh configure`
  * Checked for the `awk.h` file: `ls -l awk.h`. Found it!
  * Move again to the SPAWK build directory (`cd /tmp/spawk-2.4.1`) and run: `sh configure`
  * Local versions of SPAWK libraries have been created. Try to test: `make test`. Got error messages about not found MySQL database in my system. In order to try the SPAWK module, I installed the _server_ MySQL component: `yum install -y mysql-server` and run `service mysqld start`.
  * Retry the test: `make test` and succeed!
  * Install the SPAWK libraries to a global place: `make install`
  * Do some cleanup: `cd /tmp` and then: `rm -rf spawk-2.4.1* gawk-3.1.7*`.
  * Finished!

[Top](SpawkInstallation.md)

<u>
<h2>Fedora11</h2>
</u>
<p>
I've just installed the <i>Fedora11</i> Linux distro without installing any <i>MySQL</i> components (server, libs, client etc). I followed the following procedure and managed to install the SPAWK module without a single error:<br>
</p>
  * Login as panos.
  * Enter Mozilla Firefox and go to the http://code.google.com/p/spawk URL (SPAWK project).
  * Download the spawk-_2_._4_._1_.tar.gz SPAWK archive and open with the _Archive Manager_ (`tar`).
  * Open a terminal and search the directory where the archive have been extracted. Found it in `/home/panos`
  * Execute `sudo -i` to gain root permissions (if not a sudoer, then be one by editing the `/etc/sudoers` files as root).
  * Position yourself to the SPAWK directory (`cd /home/panos/spawk-2.4.1`) and run `sh configure`
  * I got error messages about not found files (`awk.h`, `mysql.h`, etc).
  * Run `yum install -y mysql-devel` to install the _MySQL_ development package. That command installed the missing `mysql.h` file as well as the `libmysqlclient.so` and `libmysqlclient_r.so` files also.
  * Checked the gawk's version (`gawk --version`). Got _3.1.6a_.
  * Move to `/home/panos` and download gawk sources for found gawk version: `wget http://ftp.gnu.org/gnu/gawk/gawk-3.1.6.tar.gz`
  * Extract the gawk sources: `tar xzf gawk-3.1.6.tar.gz`
  * Move to gawk-3.1.6 (`cd gawk-3.1.6`) and run: `sh configure`
  * Checked for the `awk.h` file: `ls -l awk.h`. Found it!
  * Move again to the SPAWK build directory (`cd /home/panos/spawk-2.4.1`) and run: `sh configure`
  * If everything is ok, then run: `make`. Local versions of SPAWK libraries have been created. Try to test: `make test`. Got error messages about not found MySQL database in my system. In order to try the SPAWK module, I installed the _server_ MySQL component: `yum install -y mysql-server` and run `service mysqld restart`.
  * Retry the test: `make test` and succeed!
  * Install the SPAWK libraries to a global place: `make install`
  * Do some cleanup: `cd /home/panos` and then: `rm -rf spawk-2.4.1 gawk-3.1.6`. Also remove `/tmp/spawk-2.4.1.tar.gz` left there by Firefox.
  * Finished!

[Top](SpawkInstallation.md)

<u>
<h2>Ubuntu9.04</h2>
</u>
<p>
I've just installed the <i>Ubuntu9.04</i> Linux distro. I followed the following procedure and managed to install the SPAWK module without a single error:<br>
</p>
  * Login as panos.
  * Enter Mozilla Firefox and go to the http://code.google.com/p/spawk URL (SPAWK project).
  * Download the spawk-_2_._4_._1_.tar.gz SPAWK archive and open with the _Archive Manager_ (`tar`).
  * Open a terminal and search the directory where the archive have been extracted. Found it in `/tmp`
  * Execute `sudo -i` to gain root permissions (if not a sudoer, then be one by editing the `/etc/sudoers` files as root).
  * Position yourself to the SPAWK directory (`cd /home/panos/spawk-2.4.1`) and run `sh configure`
  * I got error messages about not found files (`awk.h`, `mysql.h`).
  * Run `apt-get install libmysqlclient16-dev` to install the _MySQL_ development package. That command installed the missing `mysql.h`.
  * Checked the gawk's version (`gawk --version`). Got an error message about not found gawk. Install gawk using `apt-get install gawk`. Run again `gawk --version`. Got _3.1.6_.
  * Move to `/tmp` and download gawk sources for found gawk version: `wget http://ftp.gnu.org/gnu/gawk/gawk-3.1.6.tar.gz`
  * Extract the gawk sources: `tar xzf gawk-3.1.6.tar.gz`
  * Move to gawk-3.1.6 (`cd gawk-3.1.6`) and run: `sh configure`
  * Checked for the `awk.h` file: `ls -l awk.h`. Found it!
  * Move again to the SPAWK build directory (`cd /home/panos/spawk-2.4.1`) and run: `sh configure`
  * Local versions of SPAWK libraries have been created. Try to test: `make test`. Got error messages about not found MySQL database in my system. In order to try the SPAWK module, I installed the _server_ MySQL component: `apt-get install mysql-server`
  * Retry the test: `make test` and succeed!
  * Install the SPAWK libraries to a global place: `make install`
  * Do some cleanup: `cd /tmp` and then: `rm -rf spawk-2.4.1* gawk-3.1.6*`.
  * Finished!

[Top](SpawkInstallation.md)
[Print](http://code.google.com/p/spawk/wiki/SpawkInstallation?show=content,meta)