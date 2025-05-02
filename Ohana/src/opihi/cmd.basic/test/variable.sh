
list tests
 test1
 test2
 test3
 test4
 test5
 test6
 test7
 test8
 test9
 test10
 test11
 test12
 test13
 test14
 testmem1
 testmem2
 testmem3
 testmem4
 testmem5
 testmem6
 testmem7
 testmem8
 testmem9
 testmem10
end

# do we set variables correctly?
macro test1

 local i

 $i = 99
  
 if ($i == 99)
   $PASS = 1
 else
   $PASS = 0
 end

end

# do math expressions assign the correct value (test2 -> test8)
macro test2

  local i

  $i = 99 - 98
  
 if ($i == 1)
   $PASS = 1
 else
   $PASS = 0
   echo "i : $i"
 end

end

macro test3

  local i

  $i = 2*3
  
 if ($i == 6)
   $PASS = 1
 else
   $PASS = 0
   echo "i: $i"
 end

end

macro test4

  local i

  $i = 2^3

 if ($i == 8)
   $PASS = 1
 else
   $PASS = 0
   echo "i: $i"
 end

end

macro test5

  local i

  $i = 2+3

 if ($i == 5)
   $PASS = 1
 else
   $PASS = 0
   echo "i: $i"
 end

end

macro test6

  local i

  $i = 6/3

 if ($i == 2)
   $PASS = 1
 else
   $PASS = 0
   echo "i: $i"
 end

end

# testing operation priority
macro test7

  local i j

  $i = 2 - 3*2
  $j = 10/2*5+5

 if (($i == -4) && ($j == 30))
   $PASS = 1
 else
   $PASS = 0
   echo "i: $i\ j: $j"
 end

end

# testing math on negative numbers
macro test8

  local i

  $i = -2 - -3

 if ($i == 1)
   $PASS = 1
 else
   $PASS = 0
   echo "i: $i"
 end

end

# testing the existance of variables
macro test9

  local i

  $i = 0

 if ($?i == 1)
   $PASS = 1
 else
   $PASS = 0
   echo "i: $i"
 end
end

# test increment
macro test10

  local i N

  $N = 0
  for i 0 100
    $N ++
  end

  if ($N == 100)
   $PASS = 1
  else
   $PASS = 0
   echo "N: $N"
 end
end

# test decrement
macro test11

  local i N

  $N = 100
  for i 0 100
    $N --
  end

  if ($N == 0)
   $PASS = 1
  else
   $PASS = 0
   echo "N: $N"
 end
end

# test command assign
macro test12

  local i N

  $N = `ls -dF /etc`

  if ("$N" == "/etc/")
   $PASS = 1
  else
   $PASS = 0
   echo "N: $N"
 end
end

# test vector assign
macro test13

  local i N

  create v1 0 100
  v1[5] = 10
  set v2 = v1 + 20

  if (v2[5] == 30)
   $PASS = 1
  else
   $PASS = 0
   echo "v1\[5\]: v1[5]"
   echo "v2\[5\]: v2[5]"
 end
end

# test local variables
macro test14

 $PASS = 1

 # a slightly weak test: depends on no prior macros defining variables
 # with the names below
 if (($?very_obscure_name == 1) || ($?another_obscure_name == 1))
  $PASS = 0
 end
end

# check memleaks (set global)
macro testmem1

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 10000
   $Nvar = 10
 end    
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} < 10)
   $PASS = 1
 else
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end
end

# check memleaks (set string)
macro testmem2

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 output /dev/null
 for i 0 10000
   $Nvar = test line
 end    
 output stdout
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} < 10)
   $PASS = 1
 else
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end
end

# check memleaks (set double)
macro testmem3

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 10000
   $Nvar = 5.212
 end    
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} < 10)
   $PASS = 1
 else
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end
end

# check memleaks (set local)
macro testmem4

 local i N

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 10000
   $N = 10
 end    
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} < 10)
   $PASS = 1
 else
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end
end

# check memleaks (existence)
macro testmem5

 local i N

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 output /dev/null
 for i 0 10000
   echo $?N
 end    
 output stdout
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} < 10)
   $PASS = 1
 else
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end
end

# check memleaks (get variable)
macro testmem6

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 $Nvar = 5
 output /dev/null
 for i 0 10000
   echo $Nvar
 end    
 output stdout
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} < 10)
   $PASS = 1
 else
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end
end

# check memleaks (increment)
macro testmem7

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 $Nvar = 0
 output /dev/null
 for i 0 10000
   $Nvar ++
 end    
 output stdout
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} < 10)
   $PASS = 1
 else
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end
end

# check memleaks (decrement)
macro testmem8

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 $Nvar = 10000
 output /dev/null
 for i 0 10000
   $Nvar --
 end    
 output stdout
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} < 10)
   $PASS = 1
 else
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end
end

# check memleaks (command)
macro testmem9

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 output /dev/null
 for i 0 100
   $Nvar = `ls -d /etc`
 end    
 output stdout
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} < 10)
   $PASS = 1
 else
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/100}"
 end
end

# check memleaks (vector assign)
macro testmem10

 local i

 create v1 0 100

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 output /dev/null
 for i 0 10000
   v1[5] = $i
 end    
 output stdout
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} < 10)
   $PASS = 1
 else
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/10000}"
 end
end
