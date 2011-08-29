require 'rubygems'
require 'test/unit'

$: << File.join(File.dirname(__FILE__), "..", "lib")
require 'rpm'

def test_data(name)
  File.join(File.dirname(__FILE__), "data", name)
end
