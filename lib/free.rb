# free.rb
# (C) John Mair (banisterfiend); MIT license

direc = File.dirname(__FILE__)

require "#{direc}/free/version"

begin
  if RUBY_VERSION =~ /1.9/
    require "#{direc}/1.9/free"
  else
    require "#{direc}/1.8/free"
  end
rescue LoadError => e
  require "rbconfig"
  dlext = Config::CONFIG['DLEXT']
  require "#{direc}/free.#{dlext}"
end

class Object
  include Free
end
