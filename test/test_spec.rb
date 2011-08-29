require File.join(File.dirname(__FILE__), 'helper')


SIMPLE_SPEC =<<EOF

EOF

class RPM_Spec_Tests < Test::Unit::TestCase

  def setup

  end

  def test_parsing
    @spec = RPM::Spec.new(test_data('a.spec'))
    assert_equal 2, @spec.packages.size
    assert @spec.packages.collect(&:name).include?('a')
    assert @spec.packages.collect(&:name).include?('a-devel')

    assert @spec.buildrequires.collect(&:name).include?('c')
    assert @spec.buildrequires.collect(&:name).include?('d')
  end

  def test_build
    @spec = RPM::Spec.new(test_data('simple.spec'))
    assert_equal 0, @spec.build(RPM::BUILD_BUILD, true)
  end
#Dir.mktmpdir

end
