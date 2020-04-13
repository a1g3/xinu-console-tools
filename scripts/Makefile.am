# Hook for  logrotate daemon.
LOGROT         = xinu-consoled.logrot
logdir         = @logdir@
logrotdir      = $(sysconfdir)/logrotate.d
logrot_DATA    = $(LOGROT)

$(LOGROT):	logrotate.d/$(LOGROT)
	sed -e "s|%LOGFILE%|$(logdir)/@cserver@|" < $< > $@

# Hook for SysV daemon start/stop scripts.
INITDAEMON     = xinu-consoled.rc
INITPDAEMON    = xinu-powerd.rc

initddir       = $(sysconfdir)/rc.d/init.d
initd_DATA     = $(INITDAEMON)

$(INITDAEMON):	init.d/$(INITDAEMON)
	sed -e "s|%DAEMON%|$(sbindir)/@cserver@|" < $< > $@
$(INITPDAEMON): init.d/$(INITPDAEMON)
	sed -e "s|%DAEMON%|$(sbindir)/@pserver@|" < $< > $@

# Hook for profile shell scripts
PROFILESH      = xinu.sh
PROFILECSH     = xinu.csh

profiledir     = $(sysconfdir)/profile.d
profile_DATA   = $(PROFILESH) $(PROFILECSH)

$(PROFILESH):	profile.d/$(PROFILESH)
	sed -e "s|%SERVERNAME%|@servername@|" < $< > $@

$(PROFILECSH):	profile.d/$(PROFILECSH)
	sed -e "s|%SERVERNAME%|@servername@|" < $< > $@

EXTRA_DIST = logrotate.d/$(LOGROT) init.d/$(INITDAEMON) \
		profile.d/$(PROFILESH) profile.d/$(PROFILECSH) \
		init.d/$(INITPDAEMON)

MOSTLYCLEANFILES = $(INITDAEMON) $(INITPDAEMON) \
				   $(LOGROT) $(PROFILESH) $(PROFILECSH)

install: $(LOGROT) $(INITDAEMON) $(INITPDAEMON) $(PROFILESH) $(PROFILECSH)
	mkdir -p $(DESTDIR)$(logrotdir)
	mkdir -p $(DESTDIR)$(initddir)
	mkdir -p $(DESTDIR)$(profiledir)
	$(INSTALL) -m 0644 $(LOGROT) $(DESTDIR)$(logrotdir)/xinu-consoled
	$(INSTALL) -m 0755 $(INITDAEMON) $(DESTDIR)$(initddir)/xinu-consoled
	$(INSTALL) -m 0755 $(INITPDAEMON) $(DESTDIR)$(initddir)/xinu-powerd
	$(INSTALL) -m 0755 $(PROFILESH)  $(DESTDIR)$(profiledir)/$(PROFILESH)
	$(INSTALL) -m 0755 $(PROFILECSH) $(DESTDIR)$(profiledir)/$(PROFILECSH)
