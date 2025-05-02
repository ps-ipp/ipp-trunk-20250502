
list tests
 test1
 test2
end

# check status without any jobs
macro test1
 $PASS = 1
 break -auto off

 output -err /dev/null
 reset
 exec rm -f test.txt
 output -err stderr

 output test.txt
 status
 output stdout

 # this test requires a specific set of output
 list line -x "cat test.txt"
 if ($line:n != 4)
   $PASS = 0
 end
 if ("$line:0" != "STATUS NONE")
   $PASS = 0
 end
 if ("$line:1" != "EXITST 0")
   $PASS = 0
 end
 if ("$line:2" != "STDOUT 0")
   $PASS = 0
 end
 if ("$line:3" != "STDERR 0")
   $PASS = 0
 end
end

# check status with a basic job
macro test2
 $PASS = 1
 break -auto off

 output -err /dev/null
 reset
 exec rm -f test.txt
 job ls 
 usleep 500000
 output -err stderr

 output test.txt
 status
 output stdout

 # this test requires a specific set of output
 list line -x "cat test.txt"
 if ($line:n != 4)
   $PASS = 0
 end
 if ("$line:0" != "STATUS EXIT")
   $PASS = 0
 end
end
