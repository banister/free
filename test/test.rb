direc = File.dirname(__FILE__)

require 'rubygems'
require "#{direc}/../lib/free"
require 'bacon'

puts "Testing free version #{Free::VERSION}..." 
puts "Ruby version: #{RUBY_VERSION}"

describe Free do

  ["hello", Object.new, Class.new, /hello/, [], {}, 10.5].each do |v|
    it "should free a #{v.class}" do
      v
      # make sure test doesn't pass as a result of normal garbage
      # collection, so keep a reference to v
      c = v

      # grab the object id since referring to object directly (through
      # c) will likely result in segfault, which can't be rescued.
      id = v.object_id

      ObjectSpace._id2ref(id).should == v

      # free it
      v.free

      # Two things may happen if it is properly freed:
      # 1. The _id2ref resolves to another object (the free slot is
      #   replace by a new object)
      # 2. Calling _id2ref raises an exception, typically a RangeError
      (ObjectSpace._id2ref(id)  != v || lambda { ObjectSpace._id2ref(id) } rescue true).should == true
    end
  end
end
