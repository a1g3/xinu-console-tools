# XINU Console Daemon needs a uid and gid.
%define xinu_uid    95
%define xinu_gid    95


Name:      xinu-console
Version:   2.07
Release:   1
Summary:   Remote console tools for XINU development.
Group:     Applications/Internet
License:   Purdue Research Foundation
Source0:   %{name}-%{version}.tar.gz
BuildRoot: /var/tmp/buildroot-%{name}
BuildRequires: tcp_wrappers-devel
Packager:  Dennis Brylow <brylow at mscs.mu.edu>
Requires: expect

%package clients
Summary:   Remote console tools for XINU development.
Group: Applications/Internet

%package server
Summary: The XINU Console daemon.
Group: System Environment/Daemons

%package powerd
Summary: The XINU Console Power daemon
Group: System Environment/Daemons


%description
The xinu-consoled daemon and associated tools allow users
to reserve and communicate with backend machines in an
experimental operating system testbed where a single
server aggregates the backend consoles.

%description clients
This package provides the xinu-console and xinu-status utilities,
as well as default profile settings.

%description server
This package provides the daemon and various associated utilities
for providing network clients with connectivity to backend consoles
that are really only connected directly to the daemon host.

%description powerd
This package provides the daemon and client utilities for remotely 
controlling the power on a pool of backends.

%prep
%setup

%configure --prefix=%{_prefix}

%build
make

%install
make install DESTDIR=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%pre server
%{_sbindir}/groupadd -r -g %{xinu_gid} xinu 2>/dev/null || :
%{_sbindir}/useradd -d /var/empty/xinu -s /bin/false -u %{xinu_uid} \
        -g xinu -M -r xinu 2>/dev/null || :
touch /var/log/xinu-consoled
chown xinu:xinu /var/log/xinu-consoled
chmod 0660 /var/log/xinu-consoled

%post server
/sbin/chkconfig --add xinu-consoled

%postun server
/sbin/service xinu-consoled condrestart > /dev/null 2>&1 || :

%preun server
if [ "$1" = 0 ]
then
        /sbin/service xinu-consoled stop > /dev/null 2>&1 || :
        /sbin/chkconfig --del xinu-consoled
fi

%post powerd
/sbin/chkconfig --add xinu-powerd

%postun powerd
/sbin/service xinu-powerd condrestart > /dev/null 2>&1 || :

%preun powerd
if [ "$1" = 0 ]
then
        /sbin/service xinu-powerd stop > /dev/null 2>&1 || :
        /sbin/chkconfig --del xinu-powerd
fi

%files clients
%defattr(-,root,root)
%attr(0755,root,root) %config /etc/profile.d/xinu.csh
%attr(0755,root,root) %config /etc/profile.d/xinu.sh
%attr(0755,root,root) %{_bindir}/xinu-console
%attr(0755,root,root) %{_bindir}/xinu-status
%attr(0755,root,root) %{_bindir}/mips-console
%attr(0644,root,root) %{_mandir}/man1/xinu-console.1.gz
%attr(0644,root,root) %{_mandir}/man1/xinu-status.1.gz
%doc AUTHORS ChangeLog COPYING INSTALL README*

%files server
%defattr(-,root,root)
%attr(0755,root,root) %{_sbindir}/xinu-consoled
%attr(0755,root,xinu) %{_sbindir}/cp-download
%attr(0755,root,xinu) %{_sbindir}/tty-connect
%attr(0755,root,root) %config /etc/rc.d/init.d/xinu-consoled
%attr(0644,root,xinu) %config(noreplace) %{_sysconfdir}/xinu-consoled.conf
%attr(0644,root,root) %config %{_sysconfdir}/logrotate.d/xinu-consoled
%attr(0644,root,root) %{_mandir}/man1/xinu-consoled.1.gz
%doc AUTHORS ChangeLog COPYING INSTALL README*

%files powerd
%defattr(-,root,root)
%attr(0755,root,root) %{_sbindir}/xinu-powerd
%attr(0755,root,xinu) %{_bindir}/xinu-power
%attr(0755,root,root) %config /etc/rc.d/init.d/xinu-powerd

%changelog
* Thu Jan 15 2009   Adam T. Koehler 2.07-1
- added -2 option to grab second serial port owned by your user
- modified xinu-status to not wait if any XINU_SERVERS are specified

* Fri Jun 20 2008   Adam T. Koehler and Michael J. Schultz 2.06-1
- Randomized connection backend via xinu-console (atk)
- Removed ability to remotely restart xinu-consoled (atk)
- Stripped default xinu.conf of Marquette specific entries (mjs)

* Tue Apr 01 2008	Dennis W. Brylow <brylow at mscs.mu.edu> 2.05-3
- RC script fix for reclaiming pidfile.

* Tue Apr 01 2008       Paul T. Hinze  2.05-2
- A little bugfixing.

* Tue Apr 01 2008       Paul T. Hinze  2.05-1
- Optimizations for powercycling.

* Mon Mar 31 2008       Paul T. Hinze  2.04-2
- Added xinu-powerd, the XINU Console Power daemon.
- Added xinu-power, the associated client.

* Mon Aug 06 2007       Aaron R. Gember <agember at mscs.mu.edu> 2.04-1
- Changed xinu-console to power down backend on quit.
- Added expect scripts for power off, power on, and reboot.
- Added mips-console expect script.
- Moved to default port mentioned in /etc/services for XINU.
- TCP Wrapper portability for 64-bit arch.

* Wed May 30 2007       Dennis W. Brylow <brylow at mscs.mu.edu> 2.03-1
- Significant audit eliminated several 64-bit portability bugs.

* Wed Nov 08 2006	Dennis W. Brylow <brylow at mscs.mu.edu>
- Updated list of known baudrates in lib/ttyutils.c.

* Mon Oct 23 2006	Dennis W. Brylow <brylow at mscs.mu.edu>
- Initial package. 






