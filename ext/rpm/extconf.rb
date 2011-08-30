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
  if rpm_version < rpm_version([4,1,0])
    STDERR.puts "ruby-rpm requires rpm 4.1 or newer"
    return false
  end
  # Set things up manually
  dir_config("rpm")
  $libs = append_library($libs, 'rpmdb') if rpm_version < rpm_version([4,6,0])
  $libs = append_library($libs, 'rpm')
  have_library('rpmbuild', 'getBuildTime')
  if rpm_version >= rpm_version([4,6,0])
    $defs << "-D_RPM_4_4_COMPAT"
    return true
  end
  if have_header('rpm/rpmlib.h') and
      check_db and
      have_library('rpmio') then
    true
  else
    STDERR.puts "rpm library not found"
    false
  end
end

# if no parameters, returns the installed rpm
# version, or the one specified by the array
# ie: [4,1,0]
def rpm_version(ver=[])
  ver = `LANG=C rpm --version| cut -d' ' -f 3`.split(/\./) if ver.empty?
  return (ver[0].to_i<<16) + (ver[1].to_i<<8) + (ver[2].to_i<<0)
end

def check_rpm_version
  # TODO: zaki: strict checking is requires
  verflag = "-DRPM_VERSION_CODE=#{rpm_version}"
  $defs << verflag
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
create_makefile('rpm')
