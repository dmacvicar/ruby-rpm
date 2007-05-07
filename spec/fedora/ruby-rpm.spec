%{!?ruby_sitelib: %define ruby_sitelib %(ruby -rrbconfig -e "puts Config::CONFIG['sitelibdir']")}
%{!?ruby_sitearch: %define ruby_sitearch %(ruby -rrbconfig -e "puts Config::CONFIG['sitearchdir']")}

Summary: Ruby bindings for RPM
Name: ruby-rpm

Version: 1.2.3
Release: 1%{?dist}
Group: Development/Languages
License: GPL
URL: http://rubyforge.org/projects/ruby-rpm/
Source0: %{name}-%{version}.tgz
BuildRoot: %{_tmppath}/%{name}-%{version}-root
Requires: ruby >= 1.8.1
BuildRequires: ruby >= 1.8.1
BuildRequires: ruby-devel >= 1.8.1
BuildRequires: rpm-devel >= 4.2.1
BuildRequires: db4-devel

%description
Provides bindings for accessing RPM packages and databases from Ruby. It
includes the low-level C API to talk to rpm as well as Ruby classes to
model the various objects that RPM deals with (such as packages,
dependencies, and files).

%prep
%setup -q

%build
ruby install.rb config \
    --bin-dir=%{_bindir} \
    --rb-dir=%{ruby_sitelib} \
    --so-dir=%{ruby_sitearch} \
    --data-dir=%{_datadir}

ruby install.rb setup

%install
[ "%{buildroot}" != "/" ] && %__rm -rf %{buildroot}
ruby install.rb config \
    --bin-dir=%{buildroot}%{_bindir} \
    --rb-dir=%{buildroot}%{ruby_sitelib} \
    --so-dir=%{buildroot}%{ruby_sitearch} \
    --data-dir=%{buildroot}%{_datadir}
ruby install.rb install
chmod 0755 %{buildroot}%{ruby_sitearch}/rpmmodule.so

%clean
[ "%{buildroot}" != "/" ] && rm -rf %{buildroot}

%files
%defattr(-, root, root)
%doc NEWS README COPYING ChangeLog doc
%{ruby_sitelib}/rpm.rb
%{ruby_sitearch}/rpmmodule.so

%changelog
* Tue Nov 14 2006 David Lutterkort <dlutter@redhat.com> - 1.2.1-1
- Adapted for Fedora

* Fri Jul 02 2004 Pascal Terjan <pterjan@mandrake.org> 1.2.0-1mdk
- Adapt for Mandrakelinux 
- Patch for a segfault when TMP or TEMP or TMPDIR exist but does not contain =

* Sun Mar 14 2004 Toru Hoshina <t@momonga-linux.org>
- (1.2.0-1m)
- for rpm 4.2.1.

* Tue Jan 20 2004 YAMAZAKI Makoto <zaki@zakky.org>
- (1.1.12-2m)
- add patch to fix SEGV if TMP or TEMP or TMPDIR environment variable exists. [bug:#19]

* Sun Jan 11 2004 Kenta MURATA <muraken2@nifty.com>
- (1.1.12-1m)
- version up.

* Sat Nov 01 2003 Kenta MURATA <muraken2@nifty.com>
- (1.1.11-1m)
- version up.

* Mon Aug 04 2003 Kenta MURATA <muraken2@nifty.com>
- (1.1.10-3m)
- merge from ruby-1_8-branch.

* Sun Aug 03 2003 Kenta MURATA <muraken2@nifty.com>
- (1.1.10-2m)
- rebuild against ruby-1.8.0.

* Sun May  4 2003 Kenta MURATA <muraken2@nifty.com>
- (1.1.10-1m)
- version up.

* Tue Mar  4 2003 Shingo Akagaki <dora@kitty.dnsalias.org>
- (1.1.9-1m)
- version up.

* Sat Jan 18 2003 Shingo Akagaki <dora@kitty.dnsalias.org>
- (1.1.8-1m)
- version up.

* Wed Dec 25 2002 Shingo Akagaki <dora@kitty.dnsalias.org>
- (1.1.7-1m)
- version up.

* Tue Dec 17 2002 Shingo Akagaki <dora@kitty.dnsalias.org>
- (1.1.6-1m)
- version up.

* Tue Dec 10 2002 Shingo Akagaki <dora@kitty.dnsalias.org>
- (1.1.5-1m)
- version up.

* Fri Dec  6 2002 Shingo Akagaki <dora@kitty.dnsalias.org>
- (1.1.4-2m)
- bug fix.

* Fri Dec  6 2002 Shingo Akagaki <dora@kitty.dnsalias.org>
- (1.1.4-1m)
- version up.

* Thu Dec  5 2002 Shingo Akagaki <dora@kitty.dnsalias.org>
- (1.1.3-2m)
- bug fix.

* Thu Dec  5 2002 Shingo Akagaki <dora@kitty.dnsalias.org>
- (1.1.3-1m)
- version up.

* Thu Dec  5 2002 Shingo Akagaki <dora@kitty.dnsalias.org>
- (1.1.2-1m)
- version up.

* Fri May 31 2002 Kenta MURATA <muraken@kondara.org>
- (1.1.1-2k)
- version up.

* Fri May 31 2002 Kenta MURATA <muraken@kondara.org>
- (1.1.0-2k)
- version up.

* Tue May  7 2002 Kenta MURATA <muraken@kondara.org>
- (1.0.1-2k)
- version up.

* Fri May  3 2002 Kenta MURATA <muraken@kondara.org>
- (1.0.0-2k)
- version up.

* Wed May  1 2002 Kenta MURATA <muraken@kondara.org>
- (0.3.13-2k)
- version up.

* Mon Apr 29 2002 Kenta MURATA <muraken@kondara.org>
- (0.3.12-2k)
- version up.

* Sun Apr 28 2002 Kenta MURATA <muraken@kondara.org>
- (0.3.11-2k)
- version up.

* Wed Apr 24 2002 Kenta MURATA <muraken@kondara.org>
- (0.3.10-2k)
- version up.

* Sun Apr 21 2002 Toru Hoshina <t@kondara.org>
- (0.3.9-6k)
- i586tega... copytega... 

* Sun Apr 21 2002 Kenta MURATA <muraken@kondara.org>
- (0.3.9-4k)
- applyed patch basenames and dirindexes.

* Sun Apr 21 2002 Kenta MURATA <muraken@kondara.org>
- (0.3.9-2k)
- version up.

* Sun Apr 21 2002 Kenta MURATA <muraken@kondara.org>
- (0.3.8-4k)
- ukekeke.

* Fri Apr 19 2002 Kenta MURATA <muraken@kondara.org>
- (0.3.8-2k)
- version up.
- devel merge to main.

* Fri Apr 19 2002 Kenta MURATA <muraken@kondara.org>
- (0.3.7-2k)
- version up.
- append package devel for neomph. fufufu.

* Fri Apr 19 2002 Kenta MURATA <muraken@kondara.org>
- (0.3.6-2k)
- version up.

* Fri Apr 19 2002 Kenta MURATA <muraken@kondara.org>
- (0.3.5-4k)
- defattr

* Thu Apr 18 2002 Kenta MURATA <muraken@kondara.org>
- (0.3.5-2k)
- version up.

* Thu Apr 18 2002 Kenta MURATA <muraken@kondara.org>
- (0.3.4-2k)
- version up.

* Thu Apr 18 2002 Kenta MURATA <muraken@kondara.org>
- (0.3.3-4k)
- apply patch to DB#each_match

* Thu Apr 18 2002 Kenta MURATA <muraken@kondara.org>
- (0.3.3-2k)
- version up.

* Wed Apr 17 2002 Kenta MURATA <muraken@kondara.org>
- (0.3.2-2k)
- version up.

* Wed Apr 17 2002 Kenta MURATA <muraken@kondara.org>
- (0.3.1-6k)
- apply patch version_parse.

* Wed Apr 17 2002 Kenta MURATA <muraken@kondara.org>
- (0.3.1-4k)
- apply patch Init_hoge.

* Wed Apr 17 2002 Kenta MURATA <muraken@kondara.org>
- (0.3.1-2k)
- first release.
