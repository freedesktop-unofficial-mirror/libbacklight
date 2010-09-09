AC_PREREQ(2.63)

AC_INIT([libbacklight], 0.01, [mjg@redhat.com])
AC_CONFIG_SRCDIR([libbacklight.c])
AC_CONFIG_MACRO_DIR(m4)

AM_INIT_AUTOMAKE([foreign 1.11 -Wall])

AC_CANONICAL_HOST
AM_SILENT_RULES([yes])

AC_PROG_CC
AC_PROG_LIBTOOL
PKG_CHECK_MODULES([PCIACCESS], [pciaccess >= 0.10.0])
AC_CONFIG_FILES([Makefile libbacklight.pc])
AC_OUTPUT