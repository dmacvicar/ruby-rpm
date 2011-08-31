require File.join(File.dirname(__FILE__), 'helper')
require 'fileutils'
require 'tmpdir'

def rpm_version(ver=[])
  ver = `LANG=C rpm --version| cut -d' ' -f 3`.split(/\./) if ver.empty?
  return (ver[0].to_i<<16) + (ver[1].to_i<<8) + (ver[2].to_i<<0)
end

class RPM_DB_Tests < Test::Unit::TestCase

  def with_tmp_db
    Dir.mktmpdir do |root_dir|
      RPM::DB.init(root_dir)
      yield root_dir
    end
  end

  def setup
    @version = rpm_version
  end

  def teardown
  end

  def test_init
    with_tmp_db do |root_dir|
      assert File.exist?(File.join(root_dir, RPM['_dbpath'], 'Packages'))
    end
  end

  def test_rebuild
    # We add the "/" to work around an obscure bug in older
    # RPM versions
    with_tmp_db do |root_dir|
      RPM::DB.rebuild(root_dir + "/")
      assert File.exist?(File.join(root_dir, RPM['_dbpath'], 'Packages'))
    end
  end

  def test_open

    with_tmp_db do |root_dir|
      db = RPM::DB.open(true, root_dir)
      assert db

      assert_equal(db.root, root_dir) if @version < rpm_version([4,6,0])
      if @version >= rpm_version([4,6,0])
        assert_raise(NoMethodError) { db.root }
      end

      assert_equal( db.home, RPM['_dbpath'] ) if @version < rpm_version([4,6,0])
      if @version >= rpm_version([4,6,0])
        assert_raise(NoMethodError) { db.home }
      end

      db.close
    end
  end

end
