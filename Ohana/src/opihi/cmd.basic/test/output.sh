
list tests
 test1
 testmem1
end

# test subtraction
macro test1

 $PASS = 1

 output testout.txt

 file testout.txt fchk
 output stdout
 if ($fchk != 1)
  $PASS = 0
  echo "Output did not create test file!"
 end

 output testout.txt
 echo "This is a test."
 output stdout
 $line = `cat testout.txt`
 if ("$line" == "This is a test.")
   $PASS = 1
 else
   $PASS = 0
   echo "Output: $line"
 end

 exec rm testout.txt

end


# check memleaks
macro testmem1

 $PASS = 1
 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 output /dev/null
 for i 0 100
  output testout.txt
  echo "This is a test."
  output stdout
  exec rm testout.txt
 end    
 output stdout
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 if ({$endmem - $startmem} > 10)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/100}"
 end
end
