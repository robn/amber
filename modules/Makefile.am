pkglib_SCRIPTS =
pkglib_LTLIBRARIES = environment.la Exec.la File.la Thread.la

environment_la_SOURCES = environment.c
environment_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)'

Exec_la_SOURCES = Exec.c
Exec_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)'

File_la_SOURCES = File.c
File_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)'

Thread_la_SOURCES = Thread.c
Thread_la_LDFLAGS = -module -avoid-version -rpath '$(pkglibdir)' -lpthread
