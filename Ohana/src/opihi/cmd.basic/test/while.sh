list tests
 test1
end

# Does while work?
macro test1
 $PASS = 1
 local i
 $i = 0
 while ($i <= 10)
  if ($i == 11)
   $PASS = 0
   echo "While loop failure!"
   echo "i: $i"
   break
  end
  $i++
 end
end
