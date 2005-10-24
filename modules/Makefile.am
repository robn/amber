INCLUDES = -I"$(top_builddir)/js/src"

pkglib_SCRIPTS =
pkglib_LTLIBRARIES = environment.la Exec.la

environment_la_SOURCES = environment.c
environment_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)'

Exec_la_SOURCES = Exec.c
Exec_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)'
