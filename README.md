free
=====

(C) John Mair (banisterfiend) 2010

_force immediate garbage collection of Ruby objects_

free provides the `Object#free` method enabling a user to garbage
collect an object on demand and free all its internal structures.

* Install the [gem](https://rubygems.org/gems/free): `gem install free`
* Read the [documentation](http://rdoc.info/github/banister/free/master/file/README.md)
* See the [source code](http://github.com/banister/free)

`free` also has `rubygems-test` support; to participate, first install
free, then:

1. Install rubygems-test: `gem install rubygems-test`
2. Run the test: `gem test free`
3. Finally choose 'Yes' to upload the results. 

Example: Freeing a String
-------------------------

Let's free a String:

    hello = "hello world"
    id = hello.object_id

    hello.free

    # Note we go through the id as accessing the freed object directly
    # may cause a segfault
    ObjectSpace._id2ref(id) #=> RangeError: _id2ref': 0x1c1a63c is recycled object 


Example: Destructors
--------------------

Free also supports object destructors. If the `__destruct__` method is
implemented on the object being freed it will be called before freeing
takes place; and the return value of `free` will be the return value of `__destruct__`:

    o = Object.new
    def o.__destruct__
      :killed
    end

    o.free #=> :killed

Features and limitations
-------------------------

### Features

* Can free any object (but be careful, see below)
* Works in both Ruby 1.8 and 1.9.
* Some protection from freeing critical objects and immediate values.
* Supports object destructors
* Can free multiple objects at same time, e.g: `Free.free a, b, c`

### Limitations

* Beta software, beware.
* Supports MRI and YARV only.
* Not complete protection from freeing silly things, e.g core classes. Be sensible :)
* Can be dangerous - `free` will force garbage collection on an object
even if references to it still exist. Trying to access an already freed object may result in unexpected behaviour or segfaults.

### Caveats

Benchmarks have shown that `free` can significantly improve performance, but only
when the objects you free are very large (approximately > 10K
in size).

It is not recommended you free small or medium-sized objects as you will
actually negatively impact performance - as the `free` process itself
incurs some overhead.

In general you should benchmark your application with and without
`free` before you decide to use it.
  
Contact
-------

Problems or questions contact me at [github](http://github.com/banister)

Special Thanks
--------------

[Coderrr](http://coderrr.wordpress.com) for the idea


License
-------

(The MIT License) 

Copyright (c) 2011 (John Mair)

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
'Software'), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
