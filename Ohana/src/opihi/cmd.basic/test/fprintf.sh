
list tests
 test1
 test2
 test3
 test4
 test5
end

macro test1
 exec rm -f test.dat
 output test.dat
 fprintf "test %03d" 50
 output stdout
 $line = `cat test.dat`
 if ("$line" == "test 050")
   $PASS = 1
 else
   $PASS = 0
 end
end

macro test2
 exec rm -f test.dat
 output test.dat
 fprintf "test %6.3f" 123.45678
 output stdout
 $line = `cat test.dat`
 if ("$line" == "test 123.457")
   $PASS = 1
 else
   $PASS = 0
 end
end

macro test3
 exec rm -f test.dat
 output test.dat
 fprintf "test %x" 32
 output stdout
 $line = `cat test.dat`
 if ("$line" == "test 20")
   $PASS = 1
 else
   $PASS = 0
 end
end

macro test4
 exec rm -f test.dat
 output test.dat
 fprintf "test %10s" foobar
 output stdout
 $line = `cat test.dat`
 if ("$line" == "test     foobar")
   $PASS = 1
 else
   $PASS = 0
 end
end

# check for memory leaks
macro test5

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 output /dev/null
 for i 0 1000
   fprintf "test %10s" foobar
 end
 output stdout

 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ($endmem - $startmem < 10)
   $PASS = 1
 else
   $PASS = 0
   echo growth: {$endmem - $startmem}
   echo kB/loop: {($endmem - $startmem)/1000}
 end
end
