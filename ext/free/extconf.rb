require 'mkmf'

$CFLAGS += " -std=c99"
$CFLAGS += " -I./ruby_headers/"

create_makefile('free')
