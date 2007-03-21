# Rakefile for ruby-rpm -*- ruby -*-
require 'rake/clean'
require 'rake/testtask'
require 'rake/gempackagetask'

PKG_NAME='ruby-rpm'
PKG_VERSION='1.2.2'

EXT_CONF='ext/rpm/extconf.rb'
MAKEFILE="ext/rpm/Makefile"
RPM_MODULE="ext/rpm/rpmmodule.so"
SPEC_FILE="spec/fedora/ruby-rpm.spec"

#
# Additional files for clean/clobber
#

CLEAN.include "**/*~"

CLOBBER.include [ "config.save",
                  "ext/**/*.o", RPM_MODULE,
                  "ext/**/depend", "ext/**/mkmf.log", 
                  MAKEFILE ]

#
# Build locally
#
# FIXME: We can't get rid of install.rb yet, since there's no way
# to pass config options to extconf.rb
file MAKEFILE => EXT_CONF do |t|
    Dir::chdir(File::dirname(EXT_CONF)) do
         unless sh "ruby #{File::basename(EXT_CONF)}"
             $stderr.puts "Failed to run extconf"
             break
         end
    end
end
file RPM_MODULE => MAKEFILE do |t|
    Dir::chdir(File::dirname(EXT_CONF)) do
         unless sh "make"
             $stderr.puts "make failed"
             break
         end
     end
end
desc "Build the native library"
task :build => RPM_MODULE

Rake::TestTask.new(:test) do |t|
    t.test_files = FileList['tests/test*.rb'].exclude("tests/test_ts.rb")
    t.libs = [ 'lib', 'ext/rpm' ]
end
task :test => :build

#
# Package tasks
#

PKG_FILES = FileList[
  "Rakefile", "ChangeLog", "COPYING", "README", 
  "install.rb",
  "doc/refm.rd.ja",
  "lib/**/*.rb",
  "ext/**/*.[ch]", "ext/**/MANIFEST", "ext/**/extconf.rb",
  "tests/**/*",
  "spec/*"
]

SPEC = Gem::Specification.new do |s|
    s.name = PKG_NAME
    s.version = PKG_VERSION
    s.email = "ruby-rpm-devel@rubyforge.org"
    s.homepage = "http://rubyforge.org/projects/ruby-rpm/"
    s.summary = "Ruby bindings for RPM"
    s.files = PKG_FILES
    s.test_file = "tests/runner.rb"
    s.autorequire = "rpm"
    s.required_ruby_version = '>= 1.8.1'
    s.extensions = "ext/rpm/extconf.rb"
    s.description = <<EOF
Bindings for accessing RPM packages and databases from Ruby.
EOF
end

Rake::GemPackageTask.new(SPEC) do |pkg|
    pkg.need_tar = true
end

desc "Build (S)RPM for #{PKG_NAME}"
task :rpm => [ :package ] do |t|
    system("sed -i -e 's/^Version:.*$/Version: #{PKG_VERSION}/' #{SPEC_FILE}")
    Dir::chdir("pkg") do |dir|
        dir = File::expand_path(".")
        system("rpmbuild --define '_topdir #{dir}' --define '_sourcedir #{dir}' --define '_srcrpmdir #{dir}' --define '_rpmdir #{dir}' -ba ../#{SPEC_FILE} > rpmbuild.log 2>&1")
        if $? != 0
            raise "rpmbuild failed"
        end
    end
end

desc "Release a version"
task :dist => [ :rpm, :test ] do |t|
    # Commit changed specfile
    # svn cp svn+ssh://lutter@rubyforge.org/var/svn/ruby-rpm/trunk svn+ssh://lutter@rubyforge.org/var/svn/ruby-rpm/tags/release-#{PKG_VERSION}
    # Upload files
    # Announce on rubyforge and freshmeat
end
