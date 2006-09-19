raise NotImplementedError, "not implemented yet..."

require 'test/unit'
require 'tmpdir'
require 'rpm'

class TC_TransactionSet < Test::Unit::TestCase
  SPEC_A = <<END_SPEC_A
Summary: Spec A for Testing Ruby/RPM.
Name: spec-a
Version: 1.0
Release: 1m
Group: Documentation
License: GPL

BuildArchitectures: noarch

%description
Spec A for Testing Ruby/RPM.

%files
%defattr(-, root, root)

%changelog
* Sun May 23 2004 Kenta MURATA <muraken2@nifty.com>
- (1.0-1m)
- initial release.
END_SPEC_A

  SPEC_C = <<END_SPEC_C
Summary: Spec C for Testing Ruby/RPM.
Name: spec-c
Version: 1.0
Release: 1m
Group: Documentation
License: GPL

BuildArchitectures: noarch

Requires: spec-b

%description
Spec C for Testing Ruby/RPM.

%files
%defattr(-, root, root)

%changelog
* Sun May 23 2004 Kenta MURATA <muraken2@nifty.com>
- (1.0-1m)
- initial release.
END_SPEC_C

  def init_rpmmacros(path)
    File.open(path, "w") do |io|
      io.puts("%_topdir #{@rpm_dir}")
    end
  end

  def init_rpmrc(path)
    File.open(path, "w") do |io|
      io.puts("macrofiles: #{@rpm_dir}/macros")
    end
  end

  def build_spec(rpmrc, path)
    log = `rpmbuild --rcfile=/usr/lib/rpm/rpmrc:#{rpmrc} -ba #{path}`
    if $?.exitstatus != 0 then
      raise RuntimeError, log
    end
  end

  def setup
    @work_dir = Dir.tmpdir + "/.ruby-rpm-test"

    @rpm_dir = "#{@work_dir}/rpm"
    %W(SPECS SRPMS SOURCES RPMS/#{RPM['_arch']} RPMS/noarch BUILD SRPMS).each do |dir|
      FileUtils.mkdir_p("#{@rpm_dir}/#{dir}")
    end

    @root_dir = "#{@work_dir}/root"
    FileUtils.mkdir_p("#{@root_dir}#{RPM['_dbpath']}")

    ## build spec file for testing
    init_rpmrc("#{@rpm_dir}/rpmrc") unless File.file?("#{@rpm_dir}/rpmrc")
    init_rpmmacros("#{@rpm_dir}/macros") unless File.file?("#{@rpm_dir}/macros")
    self.class.constants.grep(/^SPEC_/).each do |name|
      filename = name.downcase + ".spec"
      next if File.file?("#{@rpm_dir}/RPMS/noarch/#{filename}")
      File.open("#{@rpm_dir}/SPECS/#{filename}", "w") do |io|
        io.print(self.class.const_get(name))
      end
      build_spec("#{@rpm_dir}/rpmrc", "#{@rpm_dir}/SPECS/#{filename}")
    end

    @ts = RPM::TransactionSet.new(@root_dir)
  end

  def teardown
    @ts.empty!
    @ts.close_db

    # If available following code, rpmbuild process is forked by for
    # each test_* methods...
    ## FileUtils.rm_rf(@work_dir)
  end

  def test_00_init_db
    assert_nothing_raised { @ts.init_db }
    assert(File.exist?("#{@root_dir}/#{RPM[:_dbpath]}/Packages"))
  end

  def test_rebuild_db
    assert_nothing_raised { @ts.rebuild_db }
  end

  def test_verify_db
    assert_nothing_raised { @ts.verify_db }
  end

  def test_add_install
    n = @ts.element_count
    @ts.add_install() # XXX
    assert_equal(n + 1, @ts.element_count)
  end

  def test_add_upgrade
    n = @ts.element_count
    @ts.add_upgrade() # XXX
    assert_equal(n + 1, @ts.element_count)
  end

  def test_add_available
    n = @ts.element_count
    @ts.add_available() # XXX
    assert_equal(n + 1, @ts.element_count)
  end

  def test_add_erase
    n = @ts.element_count
    @ts.add_erase() # XXX
    assert_equal(n + 1, @ts.element_count)
  end

  def test_check
    # not implemented yet
  end

  def test_order
    # not implemented yet
  end

  def test_set_flags
    # not implemented yet
  end

  def test_run
    # not implemented yet
  end
end

#--
# Local Variables:
# mode: ruby
# ruby-indent-level: 2
# indent-tabs-mode: nil
# End:
#++
