require 'test/unit'
require 'fileutils'
require 'tmpdir'
require 'rpm'

def rpm_version(ver=[])
  ver = `LANG=C rpm --version| cut -d' ' -f 3`.split(/\./) if ver.empty?
  return (ver[0].to_i<<16) + (ver[1].to_i<<8) + (ver[2].to_i<<0)
end

class RPM_DB_Tests < Test::Unit::TestCase
  def setup
    @version = rpm_version
    @work_dir = "#{Dir.tmpdir}/.ruby-rpm-test"
    unless File.directory?(@work_dir) then
      Dir.mkdir(@work_dir, 0777)
    end
    ## create database directory
    FileUtils.mkdir_p( "#{@work_dir}#{RPM['_dbpath']}" )
    RPM::DB.init( @work_dir )
    @a = RPM::DB.open( true, @work_dir )
  end # def setup

  def teardown
    @a.close
    FileUtils.rm_rf(@work_dir)
  end # def teardown

  def test_init
    assert( File.exist?( "#{@tmppath}/#{RPM['_dbpath']}/Packages" ) )
  end # def test_initdb

  def test_rebuild
    # We add the "/" to work around an obscure bug in older
    # RPM versions
    RPM::DB.rebuild( @work_dir + "/")
    assert( File.exist?( "#{@tmppath}/#{RPM['_dbpath']}/Packages" ) )
  end

  def test_open
    assert( @a )
  end

  def test_root    
    assert_equal( @a.root, @work_dir ) if @version < rpm_version([4,6,0])
    if @version >= rpm_version([4,6,0])
      assert_raise(NoMethodError) { @a.root }      
    end
  end

  def test_home
    assert_equal( @a.home, RPM['_dbpath'] ) if @version < rpm_version([4,6,0])
    if @version >= rpm_version([4,6,0])
      assert_raise(NoMethodError) { @a.home }
    end
  end

end # class RPM_DB_Tests < Test::Unit::TestCase
