list tests
 test1
 memtest1
 memtest2
end

# Does scan work?
macro test1
 $PASS = 1
 file test_file.txt fchk
 if ($fchk)
  exec rm test_file.txt
 end
 output test_file.txt
 echo This
 echo is
 echo a
 echo test
 echo file
 output stdout
 scan test_file.txt fscan
 if ("$fscan" != "This")
  $PASS = 0
  echo "Default not working!"
 end
 scan test_file.txt fscan 4
 if ("$fscan" != "test")
  $PASS = 0
  echo "Scan failure!"
 end
 if ($PASS)
  exec rm test_file.txt
 end
end

# Memory test for scan (default)
macro memtest1

 file test_file.txt fchk
 if ($fchk)
  exec rm test_file.txt
 end
 output test_file.txt
 echo This
 echo is
 echo a
 echo test
 echo file
 output stdout

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 1000
  scan test_file.txt fscan
 end    
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 $PASS = 1

 if ($endmem - $startmem > 10)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/1000}"
 end

 if ($PASS)
  exec rm test_file.txt
 end

end


# Memory test for scan (specified)
macro memtest2

 file test_file.txt fchk
 if ($fchk)
  exec rm test_file.txt
 end
 output test_file.txt
 echo This
 echo is
 echo a
 echo test
 echo file
 output stdout

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 1000
  scan test_file.txt fscan 5
 end    
  
 list word -x "ps -p $PID -o rss"
 $endmem = $word:1

 $PASS = 1

 if ($endmem - $startmem > 10)
   $PASS = 0
   echo "growth: {$endmem-$startmem}"
   echo "kB/loop: {($endmem-$startmem)/1000}"
 end

 if ($PASS)
  exec rm test_file.txt
 end

end
