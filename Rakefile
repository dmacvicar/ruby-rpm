# Rakefile for ruby-rpm -*- ruby -*-
$LOAD_PATH.unshift File.expand_path("../lib", __FILE__)
require 'bundler'
Bundler::GemHelper.install_tasks
require "rpm/version"
require 'rake/testtask'

SPEC_FILE="distro/fedora/ruby-rpm.spec"

task :install => :build do
  system "sudo gem install ruby-rpm-#{RPM::VERSION}.gem"
end

Rake::TestTask.new do |t|
  t.libs << File.expand_path('../test', __FILE__)
  t.libs << File.expand_path('../', __FILE__)
  t.test_files = FileList['test/test*.rb']
  t.verbose = true
end

extra_docs = ['README*', 'TODO*', 'CHANGELOG*']
doc_files = ['lib/**/*.rb', 'ext/rpm/*.c', *extra_docs]


begin
 require 'yard'
  YARD::Rake::YardocTask.new(:doc) do |t|
    t.files   = doc_files
    t.options = ['--no-private']
  end
rescue LoadError
  STDERR.puts "Install yard if you want prettier docs"
  require 'rdoc/task'
  Rake::RDocTask.new(:doc) do |rdoc|
    rdoc.rdoc_dir = "doc"
    rdoc.title = "#{RPM::PKG_NAME} #{RPM::VERSION}"
    doc_files.each { |ex| rdoc.rdoc_files.include ex }
  end
end

task :default => [:compile, :doc, :test]
gem 'rake-compiler', '>= 0.4.1'
require 'rake/extensiontask'
Rake::ExtensionTask.new('rpm')

desc "Build (S)RPM for #{RPM::PKG_NAME}"
task :rpm => [ :package ] do |t|
  system("sed -i -e 's/^Version:.*$/Version: #{RPM::VERSION}/' #{SPEC_FILE}")
  Dir::chdir("pkg") do |dir|
    dir = File::expand_path(".")
    system("rpmbuild --define '_topdir #{dir}' --define '_sourcedir #{dir}' --define '_srcrpmdir #{dir}' --define '_rpmdir #{dir}' -ba ../#{SPEC_FILE} > rpmbuild.log 2>&1")
    if $? != 0
      raise "rpmbuild failed"
    end
  end
end
