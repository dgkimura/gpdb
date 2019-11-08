#
# Makefile for upgrade regress tests
#

subdir = contrib/pg_upgrade/test/integration
top_builddir = ../../../..
include $(top_builddir)/src/Makefile.global

NAME = upgrade_regress
#OBJS = upgrade_regress.o
include $(top_srcdir)/src/Makefile.shlib

# where to find psql for testing an existing installation
PSQLDIR = $(bindir)

ifeq ($(PORTNAME), win32)
LDLIBS += -lws2_32
endif

override CPPFLAGS := -I$(srcdir) -I$(libpq_srcdir) -I$(top_srcdir)/src/test/regress $(CPPFLAGS)
override LDLIBS := $(libpq_pgport) $(LDLIBS)

all: pg_upgrade_regress$(X) all-lib data scripts/gpdb5-cluster scripts/gpdb6-cluster scripts/upgrade-cluster scripts/reset-cluster

pg_regress.o:
	$(MAKE) -C $(top_builddir)/src/test/regress pg_regress.o
	rm -f $@ && $(LN_S) $(top_builddir)/src/test/regress/pg_regress.o .

gpstringsubs.pl:
	rm -f $@ && $(LN_S) $(top_builddir)/src/test/regress/gpstringsubs.pl

gpdiff.pl: atmsort.pm explain.pm GPTest.pm
	rm -f $@ && $(LN_S) $(top_builddir)/src/test/regress/gpdiff.pl

GPTest.pm:
	rm -f $@ && $(LN_S) $(top_builddir)/src/test/regress/GPTest.pm

atmsort.pm:
	rm -f $@ && $(LN_S) $(top_builddir)/src/test/regress/atmsort.pm

explain.pm:
	rm -f $@ && $(LN_S) $(top_builddir)/src/test/regress/explain.pm

data:
	rm -f $@ && $(LN_S) $(top_builddir)/src/test/regress/data

pg_upgrade_regress$(X): upgrade_main.o pg_regress.o submake-libpq submake-libpgport
	$(CC) $(CFLAGS) $(filter %.o,$^) $(libpq_pgport) $(LDFLAGS) $(LDFLAGS_EX) $(LIBS) -o $@

clean distclean:
	rm -f upgrade_regress.so pg_upgrade_regress$(X) $(OBJS) isolation2_main.o *.o
	rm -f pg_regress.o
	rm -f gpstringsubs.pl gpdiff.pl atmsort.pm explain.pm
	rm -f data
	rm -rf $(pg_regress_clean_files)

install: all gpdiff.pl gpstringsubs.pl

gpdb5-cluster: scripts/gpdb5-cluster.o
gpdb6-cluster: scripts/gpdb6-cluster.o
upgrade-cluster: scripts/upgrade-cluster.o
reset-cluster: scripts/reset-cluster.o

installcheck: install
	# create state on gpdb5 cluster
	./scripts/reset-cluster
	./scripts/gpdb5-cluster start
	./pg_upgrade_regress --init-file=$(top_builddir)/src/test/regress/init_file --psqldir='$(PSQLDIR)' --inputdir=$(srcdir) --schedule=$(srcdir)/gpdb5_schedule --port=50000
	./scripts/gpdb5-cluster stop
	# upgrade the cluster
	./scripts/upgrade-cluster
	# validate state on gpdb6 cluster
	./scripts/gpdb6-cluster start
	./pg_upgrade_regress --init-file=$(top_builddir)/src/test/regress/init_file --psqldir='$(PSQLDIR)' --inputdir=$(srcdir) --schedule=$(srcdir)/gpdb6_schedule --port=60000 --use-existing
	./scripts/gpdb6-cluster stop
