
list tests
 test1
end

# Test if the $MODULES list is available
macro test1

 $PASS = 1

 if ($?MODULES:0 != 1)
  $PASS = 0
  echo "Modules list not loaded!"
 end

end
