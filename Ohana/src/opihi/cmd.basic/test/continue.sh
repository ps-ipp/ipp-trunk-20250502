
list tests
 test1
end

# test that continue skips within a loop
macro test1
 $PASS = 1
 for i 0 10
   if ($i > 5)
      continue
   end
   $j = $i
 end
 if ($j != 5)
   $PASS = 0
 end
end
