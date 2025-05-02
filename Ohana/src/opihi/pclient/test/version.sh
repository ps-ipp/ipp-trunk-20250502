
list tests
 test1
 test2
 test3
 test4
 test5
 test6
 test7
end

## these tests check that the version command reports the versions
## they do not validate the actual versions themselves

# set up the version output file
macro test1
 $PASS = 1
 break -auto off
 exec rm test.txt
 output -err test.txt
 version
 output -err stderr
 file test.txt found
 if ($found != 1)
   $PASS = 0
 end
end

# test for the correct number of version lines
macro test2
 $PASS = 1
 break -auto off
 
 $line = `wc -l test.txt`

 list word -split $line
 if ($word:0 != 6)
   $PASS = 0
 end
end

# test for pclient version name
macro test3
 $PASS = 1
 break -auto off
 
 list line -x "cat test.txt"
 list word -split $line:0

 if ("$word:0" != "pclient")
   $PASS = 0
 end
end

# test for opihi version name
macro test4
 $PASS = 1
 break -auto off
 
 list line -x "cat test.txt"
 list word -split $line:1

 if ("$word:0" != "opihi")
   $PASS = 0
 end
end

# test for ohana version name
macro test5
 $PASS = 1
 break -auto off
 
 list line -x "cat test.txt"
 list word -split $line:2

 if ("$word:0" != "ohana")
   $PASS = 0
 end
end

# test for gfits version name
macro test6
 $PASS = 1
 break -auto off
 
 list line -x "cat test.txt"
 list word -split $line:3

 if ("$word:0" != "gfits")
   $PASS = 0
 end
end

# test for compilation date/time
macro test7
 $PASS = 1
 break -auto off
 
 list line -x "cat test.txt"
 list word -split $line:4

 if ("$word:0" != "compiled")
   $PASS = 0
 end
end

