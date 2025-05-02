
list tests
 test1
end

# test that config does not return an error
macro test1
 $PASS = 1
 config
 if ($STATUS == 0)
   $PASS = 0
 end
end

