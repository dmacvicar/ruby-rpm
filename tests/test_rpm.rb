require 'test/unit'
require 'rpm'

class RPM_RPM_Tests < Test::Unit::TestCase
  def test_macro_read
    assert_equal( RPM['_usr'], '/usr' )
  end # def test_macro_read

  def test_macro_write
    RPM['hoge'] = 'hoge'
    assert_equal( RPM['hoge'], 'hoge' )
  end # def test_macro_write
end # class RPM_RPM_Tests < Test::Unit::TestCase
