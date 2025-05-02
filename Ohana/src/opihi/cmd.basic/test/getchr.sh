
list tests
 test1
 test2
end

# index test
macro test1
 $PASS = 1
 getchr "a long string.string" . var
 if ($var != 13)
   $PASS = 0
 end
end

# null test
macro test2
 $PASS = 1
 getchr "a long string.string" x var
 if ($var != -1)
   $PASS = 0
 end
end
