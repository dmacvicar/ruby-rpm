#!/usr/bin/env ruby
require 'test/unit'

rootdir = "#{File::dirname($0)}/.."
$:.unshift( "#{rootdir}/tests", "#{rootdir}/lib", "#{rootdir}/ext/rpm" )

require 'test_db'
require 'test_rpm'
require 'test_source'
#require 'test_ts'
require 'test_version'
require 'test_file'

exit Test::Unit::AutoRunner.run(false, File.dirname($0))
