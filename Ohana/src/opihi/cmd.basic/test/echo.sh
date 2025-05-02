
list tests
 test1
end

# test that echo actually echoes
macro test1
 $PASS = 1
 exec rm -f test.dat
 output test.dat
 echo foobar
 output stdout
 $line = `cat test.dat`
 # exec rm -f test.dat
 if ($line != foobar)
   $PASS = 0
 end
end

