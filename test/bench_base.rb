require 'benchmark'
require '../lib/free'

TIMES = 900_00
SIZE_IN_BYTES = 10_000

STR = "x" * SIZE_IN_BYTES
