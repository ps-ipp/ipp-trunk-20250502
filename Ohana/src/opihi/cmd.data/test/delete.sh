
list tests
 test1
 memtest1
end

# Test delete
macro test1

 $PASS = 1

 $v = 7

# create v 0 10

 delete v

 if ($?v != 0)
  $PASS = 0
  echo "Variable not deleted!"
 end

end


# Memory test
macro memtest1

 local i

 list word -x "ps -p $PID -o rss"
 $startmem = $word:1

 for i 0 1000
  $u = testing
  delete u
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

# Test delete
macro test2

 $PASS = 1

 for i 0 1000
   create v 0 200
   delete v
 end
end
