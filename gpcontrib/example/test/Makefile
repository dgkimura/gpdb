top_builddir=../../..
include $(top_builddir)/src/Makefile.global

TARGETS= get_best_database

# mock.mk is gpdb specific and it will use a pattern to express all the
# dependencies of all the other mock files in the database as well as the test
# file.  It also takes care of linking correctly to CMockery and swapping out the
# real code for the mocks.
include $(top_builddir)/src/backend/mock.mk

get_best_database.t: \
	query_mock.o \
