list tests
 test1
 test2
 test3
 test4
 memtest2
 memtest3
 memtest4
end

list check
  this
  is
  a
  list
end

# Does list work?
macro test1
 $PASS = 1
 if ($check:n != 4)
  $PASS = 0
  echo "Number of list elements: $check:n"
 end
 if (("$check:0" != "this") || ("$check:1" != "is") || ("$check:2" != "a") || ("$check:3" != "list"))
  $PASS = 0
  echo "List element does not return correctly!"
 end
end

# Test split option
macro test2
 $PASS = 1
 list check2 -split This is a list
 if ($check2:n != 4)
  $PASS = 0
  echo "Number of list elements: $check2:n"
 end
 if (("$check2:0" != "This") || ("$check2:1" != "is") || ("$check2:2" != "a") || ("$check2:3" != "list"))
  $PASS = 0
  echo "List element does not return correctly!"
 end
end

# Test -x option for the ls command
macro test3
 $PASS = 1
 list check3 -x "ls /dev/null"
 if ($check3:n != 1)
  $PASS = 0
  echo "Number of list elements: $check3:n"
 end
 if ("$check3:0" != "/dev/null")
  $PASS = 0
  echo "List element does not return correctly!"
 end
end

# Test -x for files
macro test4
 $PASS = 1
 file list_test.txt fchk
 if ($fchk)
  exec rm list_test.txt
 end
 output list_test.txt
 echo This
 echo is
 echo a
 echo list
 output stdout
 list check4 -x "cat list_test.txt"
 if ($check4:n != 4)
  $PASS = 0
  echo "Number of list elements: $check4:n"
 end
 if (("$check4:0" != "This") || ("$check4:1" != "is") || ("$check4:2" != "a") || ("$check4:3" != "list"))
  $PASS = 0
  echo "List element does not return correctly!"
 end
 if ($PASS)
  exec rm list_test.txt
 end
end

# Memory test for list -split
macro memtest2

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 1000
  list check2 -split This is a list
 end    
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 $PASS = 1

 if ($endmem - $startmem > 10)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/1000}"
 end

end

# Memory test for list -x
macro memtest3

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 100
  list check3 -x "ls /dev/null"
 end    
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 $PASS = 1

 if ($endmem - $startmem > 10)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/100}"
 end

end

# Memory test for list -x
macro memtest4

 file list_test.txt fchk
 if ($fchk)
  exec rm list_test.txt
 end
 output list_test.txt
 echo This
 echo is
 echo a
 echo list
 output stdout

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 100
  list check4 -x "cat list_test.txt"
 end    
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 $PASS = 1

 if ($endmem - $startmem > 10)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/100}"
 end

 if ($PASS)
  exec rm list_test.txt
 end

end
