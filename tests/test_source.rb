require 'test/unit'
require 'rpm'

class RPM_Source_Tests < Test::Unit::TestCase
  def setup
    @a = RPM::Source.new( 'http://example.com/hoge/hoge.tar.bz2', 0 )
    @b = RPM::Source.new( 'http://example.com/fuga/fuga.tar.gz', 1, true )
  end

  def test_fullname
    assert_equal( 'http://example.com/hoge/hoge.tar.bz2', @a.fullname )
  end

  def test_to_s
    assert_equal( 'http://example.com/hoge/hoge.tar.bz2', @a.to_s )
  end

  def test_filename
    assert_equal( 'hoge.tar.bz2', @a.filename )
  end

  def test_num
    assert_equal( 0, @a.num )
    assert_equal( 1, @b.num )
  end

  def test_no?
    assert_equal(false, @a.no?)
    assert( @b.no?)
  end
end
