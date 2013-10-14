Keyword Centric Network (KCN) implementations

What is KCN?
================================================================================
    Keyword Centric Network (KCN) is a kind of Information Centric Network
(ICN).  ICN is now well-known among network researchers as a major paradigm
toward Future Internet.  ICN, however, is still just a concept, and has not yet
been defined what ICN is, and its implementation or applications are not
unclear.  We are now proposing KCN in order to present an actual implementation
or applications of ICN.

    One of basic concepts of KCN is naming information, which is similar to (but
not exactly same as) one of an original concenpt of ICN.  KCN separates a name
of information into an identifier and a locator as same as ICN.  An identifier
of a name of information in KCN is, however, keywords combined with end users'
attributes (e.g., users' locations or nationalities) while one in ICN is a
URI-like address of information.  On the other hand, a locator of a name of
information in KCN is URL (or URI) while one in ICN is an IP address.  That is,
KCN separate a name of information into keywords and URLs.

    This package demonstrates how KCN works and what can be applications of KCN
by providing an acutual implementation.  We hope that this package could present
an actual example of ICN in order to make ICN being in the wild.

Quick start:
================================================================================
    First of all, build KCN binaries, and install them as follows.

	1. install curl on *BSD* or libcurl on Linux.
	2. install jansson.
	3. extract kcn-x.x.x.tar.gz, and move to extracted directry.
	  % tar xfz kcn-x.x.x.tar.gz
	  % cd kcn-x.x.x
	4. If you check out source codes by git, please read
	   ``Tips for build failures due to autoconf tools'' section in this
	   document.
	5. run ``configure'' script.
	  % ./configure
	6. build KCN binaries.
	  % make
	7. install KCN binaries.
	  % sudo make install

When you need to specify other options for ``configure'' script, please consult
with ``INSTALL'' file.

    After installation succeeds, you can demonstrate to resolve locators from
identifier in KCN as follows:

	  % key2loc keyword1 keyword2 keyword3
	  result1
	  result2
	  result3
	  ...

    You can then try some applications using KCN framework as follows:

	Example 1. ping
	  % ping `key2loc/key2loc 'Kyushu Univ.'`
	  PING www.kyushu-u.ac.jp (133.5.5.119): 48 data bytes
	  64 bytes from 133.5.5.119: icmp_seq=0 ttl=56 time=29.140 ms
	  64 bytes from 133.5.5.119: icmp_seq=1 ttl=56 time=29.939 ms
	  64 bytes from 133.5.5.119: icmp_seq=2 ttl=56 time=29.826 ms
	  ^C
	  ----www.kyushu-u.ac.jp PING Statistics----
	  3 packets transmitted, 3 packets received, 0.0% packet loss
	  round-trip min/avg/max/stddev = 29.140/29.635/29.939/0.433 ms


	Example 2. traceroute
	  % traceroute `key2loc/key2loc 'Kyushu Univ.'`
	  traceroute to www.kyushu-u.ac.jp (133.5.5.119), 64 hops max, 40 byte packets
	    1  10.15.5.1 (10.15.5.1)  3.463 ms  1.592 ms  0.780 ms
	    2  *^C


	Example 3. wget
	  % wget `key2loc/key2loc -t uri Curry Tottori`
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

These errors results from autoconf tools' checks to see if related files need
to be regenerated or not.  These checks are not necessary on building-only
environment (i.e., non-developing environment), and should be skipped.  These
checks may, however, unnecessarily run when you check out source codes of KCN
by git since git does not consider time stamps of related files.  In order to
skip these unnecessary checks, you need to change time stamps of related files
in following order:

	configure.ac	(oldest timestamp)
	aclocal.m4		^
	config.h.in		|
	Makefile.in
	key2loc/Makefile.in	|
	lib/Makefile.in		V
	configure	(newest timestamp)

To this end, you may open (or copy) each file in order, and just overwrite
(or copy back) with no modifications.

Required libraries and tips on build failures:
================================================================================
    This KCN implementation depends directly upon following libraries:

	- curl on *BSD* (or libcurl on Linux)
	- jansson

    Please make sure that these and related libraries are properly installed
onto your environment.  You can examine the installation of curl as follows:

	  % curl-config --cflags --libs

If curl is properly installed, above command may produce below output:

	  -I/usr/pkg/include
	  -L/usr/pkg/lib -L/usr/pkg/lib -Wl,-R/usr/pkg/lib		\
	  -L/usr/pkg/lib -lcurl -lidn -lssl -lcrypto -lgssapi		\
	  -lheimntlm -lkrb5 -lhx509 -lcom_err -lcrypto -lasn1 -lwind	\
	  -lroken -lcrypt -lpthread -lz

If you cannot obtain above output (note that shown libraries depend upon your
environment), please make sure that curl is properly installed.

When curl is properly installed but still ``configure'' script fails, please
make sure that pkg-config is properly installed and works well since
``configure'' script uses ``pkg-config'' to obtain necessary information about
libraries.  Generally speaking, ``pkg-config'' may be properly installed onto
your system because it is widely used in many cases.

    When pkg-config cannot be installed or cannot work, you can manually specify
necessary information about curl and jansson to the ``configure'' script as
follows:

	Example 1.
	  % ./configure							\
	    CURL_CFLAGS=-I${ABS_CURL_DIR}/include			\
            CURL_LIBS="-L${ABS_CULR_DIR}/lib/ -lcurl"			\
            JANSSON_CFLAGS=-I${ABS_JANSSON_DIR}/include			\
            JANSSON_LIBS="-L${ABS_JANSSON_DIR}/lib/ -ljansson"

	Example 2.
	  % ./configure							\
	    CURL_CFLAGS=-I${ABS_CURL_DIR}/include			\
            CURL_LIBS="-L${ABS_CULR_DIR}/lib/ -lcurl"			\
            JANSSON_CFLAGS=-I${ABS_JANSSON_DIR}/include			\
            JANSSON_LIBS=${ABS_JANSSON_DIR}/lib/libjansson.a

Note that ABS_CURL_DIR and ABS_JANSSON_DIR denote *absolute* paths into which
curl and jansson are installed, respectively.

Tested environment and tips
================================================================================
NetBSD current 6.99.3
	pkg-config:
		pkgsrc/devel/pkg-config
	curl:
		pkgsrc/www/curl
	jansson:
		jansson-2.5 obtained from http://www.digip.org/jansson/
		jansson-2.4 seems to be imported to pkgsrc/textproc/jansson
		on 2013/7/13, and it may work but has not been tested yet.

CentOS 6.4 (x86_64)
	pkg-config (pkgconfig):
		installed by default as a base tool (0.23)
	curl (libcurl):
		installed by default (7.19.7-37.el6_4 or later)
	jansson:
		jansson-2.5 obtained from http://www.digip.org/jansson/
