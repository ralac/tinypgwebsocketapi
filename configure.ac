#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([tinypgwebsocketapi], [0.1], [ruicastro.mail@gmail.com])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lpq':
AC_CHECK_LIB([pq],[pg_connect])
AC_CHECK_LIB([fcgi],[FCGI_Accept])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h stdint.h stdio.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_FUNC_MALLOC

AC_PREFIX_DEFAULT([`pwd`])

AC_CONFIG_FILES([
    Makefile
    src/Makefile
])
#AC_CONFIG_SUBDIRS([lighttpd1.4])
AM_INIT_AUTOMAKE([subdir-objects])
AC_OUTPUT
