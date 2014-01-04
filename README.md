Keyword Centric Network (KCN) implementations
================================================================================

What is KCN?
================================================================================
  Keyword Centric Network (KCN) is a kind of Information Centric Network (ICN).
ICN is now well-known among network researchers as a major paradigm
toward Future Internet.  ICN, however, is still just a concept, and has not yet
been defined what ICN is, and its implementation or applications are not
unclear.  We are now proposing KCN in order to present an actual implementation
or applications of ICN.

  One of basic concepts of KCN is naming information, which is similar to (but
not exactly same as) one of an original concenpt of ICN.  KCN separates a name
of information into an identifier and a locator as same as ICN.  An identifier
of a name of information in KCN is, however, keywords combined with end users'
attributes (e.g., current time, users' locations or nationalities) while one in
ICN is a URI-like address of information.  On the other hand, a locator of a
name of information in KCN is URL (or URI) while one in ICN is an IP address.
That is, KCN separate a name of information into keywords and URLs.

  This package demonstrates how KCN works and what can be applications of KCN
by providing an acutual implementation.  We hope that this package could present
an actual example of ICN in order to make ICN being in the wild.

Quick start
================================================================================
Installation
--------------------------------------------------------------------------------
  First of all, build KCN binaries, and install them as follows:

	1. install libevent, libcurl and jansson.
	2. extract kcn-x.x.x.tar.gz, and move to extracted directry.
	  % tar xfz kcn-x.x.x.tar.gz
	  % cd kcn-x.x.x
	3. If you check out source codes by git, please read *Tips for build
	   failures due to autoconf tools* section in this document.
	4. run *configure* script.
	  % ./configure
	5. build KCN binaries.
	  % make
	6. install KCN binaries.
	  % sudo make install
	7. run KCN database server, kcndbd.
	  % sudo /usr/local/sbin/kcndbd

Please see *Installation examples* section for actual examples in this document.
When you need to specify other options for *configure* script, please consult
with *INSTALL* file.

KCN operation test
--------------------------------------------------------------------------------
  After installation succeeds, you can demonstrate to resolve locators from
identifier in KCN, i.e. keywords,  as follows:

	  % key2loc keyword1 keyword2 keyword3 keyword4 ...
	  result1
	  result2
	  result3
	  ...

  Current KCN library supports two database engines, KCN database engine and
google searching engine.  Database engine is automatically selected in
accordance with input keywords.  For examples,

          % key2loc latency lt 100

this may choose KCN database, and search for a FQDN of a host that can be
reached with a latency, 100 msec or less.

  On the other hand,

	  % key2loc Kyushu Univ.

this may choose google database, and search for the URI of the Web page that
matches keywords, Kyuhsu and Univ.

  You can then try some applications using KCN framework as follows:

	Example 1. ping
	  % ping `key2loc 'Kyushu Univ.'`
	  PING www.kyushu-u.ac.jp (133.5.5.119): 48 data bytes
	  64 bytes from 133.5.5.119: icmp_seq=0 ttl=56 time=29.140 ms
	  64 bytes from 133.5.5.119: icmp_seq=1 ttl=56 time=29.939 ms
	  64 bytes from 133.5.5.119: icmp_seq=2 ttl=56 time=29.826 ms
	  ^C
	  ----www.kyushu-u.ac.jp PING Statistics----
	  3 packets transmitted, 3 packets received, 0.0% packet loss
	  round-trip min/avg/max/stddev = 29.140/29.635/29.939/0.433 ms


	Example 2. traceroute
	  % traceroute `key2loc 'Kyushu Univ.'`
	  traceroute to www.kyushu-u.ac.jp (133.5.5.119), 64 hops max, 40 byte packets
	    1  10.15.5.1 (10.15.5.1)  3.463 ms  1.592 ms  0.780 ms
	    2  *^C


	Example 3. wget
	  % wget `key2loc -t uri Curry Tottori`
	  --2013-10-13 10:04:41--  http://tokyotombaker.wordpress.com/2013/09/07/around-japan-in-47-curries-tottori-nashi-pear/
	  Resolving tokyotombaker.wordpress.com (tokyotombaker.wordpress.com)... 192.0.80.250, 192.0.81.250, 66.155.9.238, ...
	  Connecting to tokyotombaker.wordpress.com (tokyotombaker.wordpress.com)|192.0.80.250|:80... connected.
	  HTTP request sent, awaiting response... 200 OK
	  Length: unspecified [text/html]
	  Saving to: 'index.html'

	    [  <=>                   ] 51,707       146KB/s   in 0.3s   

	      2013-10-13 10:04:42 (146 KB/s) - 'index.html' saved [51707]

Tips for build failures due to autoconf tools
================================================================================
  This KCN package uses autoconf tools.  Autoconf tools are very useful from
the viewpoint of developers.  Autoconf tools may, however, cause build failures
like below when you check out source codes of KCN by git:

	CDPATH="${ZSH_VERSION+.}:" && cd . && /bin/sh /home/kcn/missing aclocal-1.13 
	/home/kcn/missing: line 81: aclocal-1.13: command not found
	WARNING: 'aclocal-1.13' is missing on your system.
	         You should only need it if you modified 'acinclude.m4' or
	         'configure.ac' or m4 files included by 'configure.ac'.
	         The 'aclocal' program is part of the GNU Automake package:
	         <http://www.gnu.org/software/automake>
	         It also requires GNU Autoconf, GNU m4 and Perl in order to run:
	         <http://www.gnu.org/software/autoconf>
	         <http://www.gnu.org/software/m4/>
	         <http://www.perl.org/>
	make: *** [aclocal.m4] Error 127

These errors result from checks of autoconf tools to see if related files need
to be regenerated or not.  These checks are not necessary on building-only
environment (i.e., non-developing environment), and should be skipped.  These
checks may, however, unnecessarily run when you check out source codes of KCN
by git since git does not consider time stamps of related files.  In order to
skip these unnecessary checks, you need to change time stamps of related files
in following order:

	configure.ac	(oldest timestamp)
	acinclude.m4		^
	aclocal.m4		|
	config.h.in		|
	Makefile.in		|
	kcndbd/Makefile.in	|
	kcndbctl/Makefile.in	|
	key2loc/Makefile.in	|
	lib/Makefile.in		V
	configure	(newest timestamp)

To this end, you may open (or copy) each file in order, and just overwrite
(or copy back) with no modifications.

Required libraries and tips on build failures:
================================================================================
  This KCN implementation depends directly upon following libraries:

	- pthread
	- libevent
	- libcurl
	- jansson

  Please make sure that these and related libraries are properly installed
onto your environment.

  Since jansson is quite new implementation of JSON parser at this moment,
most of build errors may be brought by jansson installation.  If you manually
installed jansson into a directory, say /usr/local/, please try to run
the *configure* script as follows:

    	Example 1.
    	  % ./configure	PKG_CONFIG_PATH=/usr/local/lib/pkgconfig

  If you still have some errors about others but jansson, you can examine the
installation of curl as follows:

    	  % curl-config --cflags --libs

If curl is properly installed, above command may produce below output:

    	  -I/usr/pkg/include
    	  -L/usr/pkg/lib -L/usr/pkg/lib -Wl,-R/usr/pkg/lib		\
    	  -L/usr/pkg/lib -lcurl -lidn -lssl -lcrypto -lgssapi		\
    	  -lheimntlm -lkrb5 -lhx509 -lcom_err -lcrypto -lasn1 -lwind	\
    	  -lroken -lcrypt -lpthread -lz

If you cannot obtain above output (note that shown libraries depend upon your
environment), please make sure that curl is properly installed.

When curl is properly installed but still the *configure* script fails, please
make sure that pkg-config is properly installed and works well since the
*configure* script uses *pkg-config* to obtain necessary information about
libraries.  Generally speaking, *pkg-config* may be properly installed onto
your system because it is widely used in many cases.

  When pkg-config cannot be installed or cannot work, you can manually specify
necessary information about curl and jansson to the *configure* script as
follows:

    	Example 2.
    	  % ./configure							\
    	    CURL_CFLAGS=-I${ABS_CURL_DIR}/include			\
    	    CURL_LIBS="-L${ABS_CULR_DIR}/lib/ -lcurl"			\
    	    JANSSON_CFLAGS=-I${ABS_JANSSON_DIR}/include			\
    	    JANSSON_LIBS="-L${ABS_JANSSON_DIR}/lib/ -ljansson"

    	Example 3.
    	  % ./configure							\
    	    CURL_CFLAGS=-I${ABS_CURL_DIR}/include			\
    	    CURL_LIBS="-L${ABS_CULR_DIR}/lib/ -lcurl"			\
    	    JANSSON_CFLAGS=-I${ABS_JANSSON_DIR}/include			\
    	    JANSSON_LIBS=${ABS_JANSSON_DIR}/lib/libjansson.a

Note that ABS_CURL_DIR and ABS_JANSSON_DIR denote *absolute* paths into which
curl and jansson are installed, respectively.

Tested environment and tips
================================================================================
NetBSD current 6.99.3 or later
--------------------------------------------------------------------------------
	pthread:
		base system.
	libevent:
		1.4.12-stable already merged into base system.
	pkg-config:
		pkgsrc/devel/pkg-config
	curl:
		pkgsrc/www/curl
	jansson:
		pkgsrc/textproc/jansson
		jansson has been firstly imported to pkgsrc on 2013/7/13.
		If your pkgsrc does not have jansson, you may obtain the
		source code of jansson-2.5 obtained:
			http://www.digip.org/jansson/

CentOS 6.4 or later (x86_64)
--------------------------------------------------------------------------------
	pthread:
		base system.
	libevent:
		libevent-devel (1.4.13-stable)
	pkg-config:
		pkgconfig (1:0.23-9.1.el6): installed by default as a base tool
	curl:
		libcurl-devel (7.19.7-37.el6_4)
	jansson:
		jansson-2.5 obtained from http://www.digip.org/jansson/

Installation examples
================================================================================
NetBSD current 6.99.3 or later
--------------------------------------------------------------------------------
	  % cd /usr/pkgsrc/devel/pkg-config && make update
	  % cd /usr/pkgsrc/www/curl && make update
	  % cd /usr/pkgsrc/textproc/jansson && make update
	  % sudo mkdir -p /usr/local/src
	  % sudo chown yourusername /usr/local/src
	  % cd /usr/local/src
	  % git clone https://github.com/ohmori7/kcn.git
	  % cd /usr/local/src/kcn
	  % ./configure
	  % make
	  % sudo make install

CentOS 6.4 or later (x86_64)
--------------------------------------------------------------------------------
	  % sudo yum install libevent-devel
	  % sudo yum install pkgconfig
	  % sudo yum install libcurl-devel
	  % sudo mkdir -p /usr/local/src
	  % sudo chown yourusername /usr/local/src
	  % cd /usr/local/src 
	  % wget http://www.digip.org/jansson/releases/jansson-2.5.tar.gz
	  % tar xvfz jansson-2.5.tar.gz
	  % cd /usr/local/src/jansson-2.5
	  % ./configure
	  % make
	  % sudo make install
	  % cd /usr/local/src
	  % git clone https://github.com/ohmori7/kcn.git
	  % cd /usr/local/src/kcn
    	  % ./configure	PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
	  % make
	  % sudo make install
