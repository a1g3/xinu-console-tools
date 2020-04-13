EXTRA_DIST = xinu.spec configure scripts/xinu.conf man/xinu-consoled.1.man

dist_sbin_SCRIPTS = scripts/cp-download

dist_bin_SCRIPTS = scripts/mips-console

mandir = $(datadir)/man
dist_man1_MANS = man/xinu-console.1  man/xinu-consoled.1  man/xinu-status.1

man/xinu-consoled.1:	man/xinu-consoled.1.man
	sed -e "s|%LOGFILE%|$(logdir)/@cserver@|" < $< \
	| sed -e "s|%CONFIGFILE%|$(sysconfdir)/@configfile@|" > $@

confdir      = $(sysconfdir)
conf_DATA    = $(configfile)

$(configfile):	scripts/xinu.conf
	sed -e "s|%SERVERNAME%|@servername@|" < $< \
	| sed -e "s|%SBINDIR%|$(sbindir)/|" > $@

MOSTLYCLEANFILES = $(configfile) man/xinu-consoled.1

SUBDIRS = lib src scripts
