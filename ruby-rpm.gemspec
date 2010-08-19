# -*- encoding: utf-8 -*-
lib = File.expand_path('../lib/', __FILE__)
$:.unshift lib unless $:.include?(lib)
require "rpm/version"

Gem::Specification.new do |s|
    s.name = RPM::PKG_NAME
    s.version = RPM::VERSION
    s.email = "ruby-rpm-devel@rubyforge.org"
    s.homepage = "http://gitorious.org/ruby-rpm"
    s.summary = "Ruby bindings for RPM"
    s.test_file = "tests/runner.rb"
    s.autorequire = "rpm"
    s.required_ruby_version = '>= 1.8.1'
    s.extensions = "ext/rpm/extconf.rb"
    s.description = <<EOF
Provides bindings for accessing RPM packages and databases from Ruby. It
includes the low-level C API to talk to rpm as well as Ruby classes to
model the various objects that RPM deals with (such as packages,
dependencies, and files).
EOF
    s.files        = Dir.glob("lib/**/*.rb") + Dir.glob("ext/**/*.{rb,h,c}") + %w(CHANGELOG.rdoc README.rdoc)

  s.required_rubygems_version = ">= 1.3.6"
  s.add_development_dependency("rake-compiler", [">= 0"])
  s.require_path = 'lib'

end
