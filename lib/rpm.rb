### -*- mode: ruby; indent-tabs-mode: nil; -*-
### Ruby/RPM
###
### Copyright (C) 2002 Kenta MURATA <muraken2@nifty.com>.

### $Id: rpm.rb 22 2004-03-29 03:42:35Z zaki $

# native
require 'rpm.so'
require 'rpm/version'

module RPM

  def vercmp(ver1, ver2)
    unless String === ver1 and String === ver2 then
      raise TypeError, 'illegal argument type'
    end
    RPM::Version.new(ver1) <=> RPM::Version.new(ver2)
  end # def vercmp(ver1, ver2)
  module_function :vercmp

  class DB

    def DB.each_match(tag, val)
      begin
        db = RPM::DB.new
        db.each_match(tag, val) {|*a| yield a}
      end
      GC.start
    end # def DB.each_match(tag, val)

    def DB.packages
      packages = nil
      begin
        db = RPM::DB.new
        packages = db.packages
      end
      GC.start
      packages
    end # def DB.packages

    def DB.packages(label=nil)
      packages = []
      if label then
        DB.each_match(RPM::DBI_LABEL, label) {|a| packages.push a}
      else
        packages = DB.packages
      end
      packages
    end # def DB.package(label=nil)

  end # class DB

end # module RPM

class RPMdb

  def RPMdb.open(root='')
    RPMdb.new root
  end # def RPMdb.open(root='')

  def close
    @db = nil
    GC.start
  end # def close

  def file(filespec)
    raise RuntimeError, 'closed DB' if @db.nil?
    pkgs = []
    @db.each_match(RPM::TAG_BASENAMES, filespec) do |pkg|
      [pkg.name, pkg.version.v, pkg.version.r]
    end
    pkgs
  end # def file(filespec)

  def group(groupname)
    raise RuntimeError, 'closed DB' if @db.nil?
    pkgs = []
    @db.each_match(RPM::TAG_GROUP, groupname) do |pkg|
      pkgs << [pkg.name, pkg.version.v, pkg.version.r]
    end
    pkgs
  end # def group(groupname)

  def package(packagename)
    raise RuntimeError, 'closed DB' if @db.nil?
    pkgs = []
    @db.each_match(RPM::DBI_LABEL, packagename) do |pkg|
      pkgs << [pkg.name, pkg.version.v, pkg.version.r]
    end
    pkgs
  end # def package(packagename)

  def whatprovides(caps)
    raise RuntimeError, 'closed DB' if @db.nil?
    pkgs = []
    @db.each_match(RPM::TAG_PROVIDENAME, caps) do |pkg|
      pkgs << [pkg.name, pkg.version.v, pkg.version.r]
    end
    pkgs
  end # def whatprovides(caps)

  def whatrequires(caps)
    raise RuntimeError, 'closed DB' if @db.nil?
    pkgs = []
    @db.each_match(RPM::TAG_REQUIRENAME, caps) do |pkg|
      pkgs << [pkg.name, pkg.version.v, pkg.version.r]
    end
    pkgs
  end # def whatrequires(caps)

  def provides(name)
    raise RuntimeError, 'closed DB' if @db.nil?
    pkg = nil
    @db.each_match(RPM::DBI_LABEL, name) {|p| pkg = p}
    if pkg then
      pkg.provides.collect {|prov| prov.name }
    else
      []
    end
  end # def provides(name)

  def requires(name)
    raise RuntimeError, 'closed DB' if @db.nil?
    pkg = nil
    @db.each_match(RPM::DBI_LABEL, name) {|p| pkg = p; break}
    if pkg then
      pkg.requires.collect {|req| [req.name, req.version.v, req.version.r] }
    else
      []
    end
  end # def requires(name)

  def each
    raise RuntimeError, 'closed DB' if @db.nil?
    @db.each do |pkg|
      yield pkg.name, pkg.version.v, pkg.version.r
    end
  end # def each

  def initialize(root='')
    @db = RPM::DB.new false, root
  end # def initialize(root='')

end # class RPMdb
