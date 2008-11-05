### -*- mode: ruby; indent-tabs-mode: nil; -*-
### Ruby/RPM
###
### Copyright (C) 2002 Kenta MURATA <muraken2@nifty.com>.

### $Id: extconf.rb 19 2004-03-28 10:01:23Z zaki $

# To build against an rpm installed in a nonstandard location, you need to
# set PKG_CONFIG_PATH and make sure the corresponding rpm executable is on
# PATH, e.g. if rpm was configured with --prefix=/dir you want
# PKG_CONFIG_PATH=/dir/lib/pkgconfig
# PATH=/dir/bin:$PATH

require 'mkmf'

dir_config('popt')

def check_popt
  if have_header('popt.h') and have_library('popt') then
    true
  else
    STDERR.puts "libpopt not found"
    false
  end
end

def check_db
  dir_config('db')
  if have_library("db-4.2","db_version")
    return true
  end
  4.downto(2) do |i|
    if have_library("db-#{i}.0", "db_version") or
        have_library("db#{i}", "db_version") then
      return true
    end
  end
  if have_library("db", "db_version") then
    true
  else
    STDERR.puts "db not found"
  end
end

def check_rpm
  return false unless check_db
  # Newer rpm supports pkg-config. If detected, compat mode for now...
  if pkg_config('rpm') then
     $CFLAGS="#{$CFLAGS} -D_RPM_4_4_COMPAT"
     return true
  end

  # Set things up manually
  dir_config("rpm")
  $libs = append_library($libs, 'rpmdb')
  $libs = append_library($libs, 'rpm')
  if have_header('rpm/rpmlib.h') and
      have_library('rpmio') and
      have_library('rpmbuild', 'getBuildTime') then
    true
  else
    STDERR.puts "rpm library not found"
    false
  end
end

def check_rpm_version
  version_string=`LANG=C rpm --version| cut -d' ' -f 3`
  ver=version_string.split(/\./)
  # TODO: zaki: strict checking is requires
  $CFLAGS="#{$CFLAGS} -DRPM_VERSION_CODE=#{(ver[0].to_i<<16) + (ver[1].to_i<<8) + (ver[2].to_i<<0)}"
end

def check_debug
  if ENV["RUBYRPM_DEBUG"] then
    puts "debug mode\n"
    $CFLAGS="#{$CFLAGS} -O0 -g -ggdb"
  else
    puts "non-debug mode\n"
  end
end

exit unless check_popt
exit unless check_rpm
exit unless check_rpm_version

check_debug

HEADERS = [ "rpmlog", "rpmps", "rpmts", "rpmds" ]
HEADERS.each { |hdr| have_header("rpm/#{hdr}.h") }

$CFLAGS="#{$CFLAGS} -Werror -Wno-deprecated-declarations"

system 'gcc -MM *.c >depend 2>/dev/null'

create_header
create_makefile('rpmmodule')
