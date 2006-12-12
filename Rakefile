# Rakefile for ruby-rpm -*- ruby -*-
require 'rake/clean'
require 'rake/testtask'
require 'rake/packagetask'

PKG_NAME='ruby-rpm'
PKG_VERSION='1.2.1'

# FIXME: Doesn't work
#Rake::TestTask.new(:test) do |t|
#  t.test_files = FileList['tests/test*.rb']
#end

# Package tasks

PKG_FILES = FileList[
  "Rakefile", "ChangeLog", "COPYING", "README", 
  "install.rb",
  "doc/refm.rd.ja",
  "lib/**/*.rb",
  "ext/**/*.[ch]", "ext/**/MANIFEST", "ext/**/extconf.rb",
  "tests/**/*",
  "spec/*"
]

Rake::PackageTask.new("package") do |p|
  p.name = PKG_NAME
  p.version = PKG_VERSION
  p.need_tar = true
  p.need_zip = true
  p.package_files = PKG_FILES
end

