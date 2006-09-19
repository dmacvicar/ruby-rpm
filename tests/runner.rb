#!/usr/bin/env ruby
require 'test/unit'

rootdir = "#{File::dirname($0)}/.."
$:.unshift( "#{rootdir}/lib", "#{rootdir}/ext/rpm" )

exit Test::Unit::AutoRunner.run(false, File.dirname($0))
