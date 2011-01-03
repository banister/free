# free.rb
# (C) John Mair (banisterfiend); MIT license

direc = File.dirname(__FILE__)

require "#{direc}/free/version"

# @author John Mair (banisterfiend)
module Free

  # Free multiple objects at once.
  # @param [Array] args The objects to free.
  # @return [nil] Returns nil.
  # @example
  #   h = "hello"
  #   g = "goodbye"
  #   m = "good morning"
  #
  #   Free.free h, g, m
  def self.free(*args) end
    
  # Force garbage collection on an object and free its internal structures.
  # @return [nil, Object] Return value of \_\_destruct\_\_ method (if
  #   defined) or nil (if no \_\_destruct\_\_ method)
  # @example
  #   h = "hello"
  #   def h.__destruct__
  #     :killed
  #   end
  #   
  #   h.free #=> :killed
  def free() end
end

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
