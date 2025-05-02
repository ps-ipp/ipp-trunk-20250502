# Elixir test config file

CONNECT         /usr/bin/ssh
MACHINE		localhost
MACHINE		localhost
elixir		test

# FIFO files
global.success	$elixir.success
global.failure	$elixir.failure
global.source	$elixir.source
global.msg	$elixir.msg
global.end	$elixir.end
global.pid	$elixir.pid

# process definition
global.Nargs	1
global.logfile  0 %s $elixir.log
global.pending	test1
global.timeout	100

# test1: ls (will ls &0 user home dir)
process	test1 
test1.arg 0 ls
test1.arg 0 %s  &0
test1.arg 0 ;
test1.arg 0 echo
test1.arg 0 SUCCESS
test1.success test2
test1.failure global

# test2 sleep 5
process	test2 
test2.arg 0 sleep
test2.arg 0 5
test2.arg 0 ;
test2.arg 0 echo
test2.arg 0 SUCCESS
test2.success test3
test2.failure global

# test3 echo &0
process	test3 
test3.arg 0 echo 
test3.arg 0 %s  &0
test3.arg 0 ;
test3.arg 0 echo
test3.arg 0 SUCCESS
test3.success global
test3.failure global

# this is a very simple elixir script to test that elixir is working
# correctly.  See the README file for instructions.
 
