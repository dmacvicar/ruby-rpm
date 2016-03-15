
>This project has been obsoleted by [ruby-rpm-ffi](https://github.com/dmacvicar/ruby-rpm-ffi). This version is not longer maintained and I keep this repo for archiving purposes. 

[maintained](https://img.shields.io/maintenance/yes/2012.svg)

# RPM bindings for ruby

* http://www.gitorious.org/ruby-rpm

# Introduction

Ruby/RPM is an interface to access RPM database for Ruby.
  
## Requirements

* ruby 1.8+
* gcc
* rpm 4.0.0 or newer (tested on 4.0.4 and 4.2.1)

## Installation

```console
gem install ruby-rpm
```

## Building

If you want to automatically install required gems, make sure to
have bundler 1.x installed

* gem instal bundler
* gem build ruby-rpm.gemspec

## Usage

```ruby
require 'rpm'
```  
  
== License

GPLv2. See COPYING file.

== Credits

* Kenta MURATA <muraken@kondara.org>
* David Lutterkort <dlutter@redhat.com>
* Duncan Mac-Vicar P. <dmacvicar@suse.de>

