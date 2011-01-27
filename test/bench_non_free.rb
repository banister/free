require './bench_base'

puts "non free #{Benchmark.measure { TIMES.times { j = STR.dup; j << rand(100).to_s } }}"

