dnl $Id$
dnl config.m4 for extension soaprotobuf

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(soaprotobuf, for soaprotobuf support,
dnl Make sure that the comment is aligned:
dnl [  --with-soaprotobuf             Include soaprotobuf support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(soaprotobuf, whether to enable soaprotobuf support,
[  --enable-soaprotobuf           Enable soaprotobuf support])

if test "$PHP_SOAPROTOBUF" != "no"; then

  PHP_ADD_LIBRARY_WITH_PATH(protobuf-c, "/usr/local/lib", SCC_SHARED_LIBADD)

  PHP_ADD_LIBRARY(m, 1, SCC_SHARED_LIBADD)

  PHP_ADD_LIBRARY(pthread, 1, SCC_SHARED_LIBADD)

  PHP_SUBST(SCC_SHARED_LIBADD)

  PHP_NEW_EXTENSION(soaprotobuf, soaprotobuf.c  soa_proxy_msg.pb-c.c, $ext_shared)
fi
