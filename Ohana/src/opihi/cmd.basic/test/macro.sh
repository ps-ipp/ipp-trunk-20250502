list tests
 test_prep
 test1
 memtest1
end

# Is the macro working?
macro test_prep
 $test_var1 = check1
 $test_var2 = check2
 $var_count = $0
end

# Is the macro working?
macro test_local
 local test_var test_var2 var_count

 $test_var1 = check1
 $test_var2 = check2
 $var_count = $0
end

macro test1
 $PASS = 1
 $var_count = 0
 $test_var1 = blank
 $test_var2 = blank
 test_prep var1 var2 var3
 if ($var_count != 4)
  $PASS = 0
  echo "Number of parameters (should be 4): $var_count"
 end
 if (("$test_var1" != "check1") || ("$test_var2" != "check2"))
  $PASS = 0
  echo "Paramaters not assigned correctly!: $test_var1 $test_var2"
 end
end

# Memory Test for macro
macro memtest1
 $PASS = 1
 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 10000
  test_prep
 end    
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ($endmem - $startmem > 10)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end
end

# Memory Test for macro
macro memtest2

 $i = 0
 test_prep

 memory check
 for i 0 10000
  test_prep
 end    
 memory check
  
end

# Memory Test for macro
macro memtest3

 $i = 0
 test_local

 memory check
 for i 0 10000
  test_local
 end    
 memory check
  
end
