direc = File.dirname(__FILE__)

require 'rubygems'
require "#{direc}/../lib/free"
require 'bacon'

puts "Testing free version #{Free::VERSION}..." 
puts "Ruby version: #{RUBY_VERSION}"

describe Free do
end

