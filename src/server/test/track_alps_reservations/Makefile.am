
PROG_ROOT = ../..

AM_CFLAGS = -g -DTEST_FUNCTION -I${PROG_ROOT}/ -I$(PROG_ROOT)/../include --coverage `xml2-config --cflags` 
AM_LIBS = `xml2-config --libs`

lib_LTLIBRARIES = libtest_track_alps_reservations.la

AM_LDFLAGS = @CHECK_LIBS@ $(lib_LTLIBRARIES) $(AM_LIBS)

check_PROGRAMS = test_track_alps_reservations

libtest_track_alps_reservations_la_SOURCES = scaffolding.c $(PROG_ROOT)/track_alps_reservations.c $(PROG_ROOT)/../lib/Libutils/u_constants.c
libtest_track_alps_reservations_la_LDFLAGS = @CHECK_LIBS@ $(AM_LIBS) -shared -lgcov

test_track_alps_reservations_SOURCES = test_track_alps_reservations.c

check_SCRIPTS = ${PROG_ROOT}/../test/coverage_run.sh

TESTS = $(check_PROGRAMS) ${check_SCRIPTS}

CLEANFILES = *.gcno *.gcda *.gcov core *.lo
