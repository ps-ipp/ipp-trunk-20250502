
macro testloop1
 local i
 for i 0 10
  echo "1: $i"
    end
end

macro testloop2
 local i
 for i 0 5
   echo "2: $i"
   testloop1
end
end

macro testlocal1
 local -static foo
 echo "1 - foo: $foo"
 $foo = local.1
 echo "1 - foo: $foo"
end

macro testlocal2
 local foo
 $foo = local.2
 echo "2 - foo: $foo"
 testlocal1
 echo "2 - foo: $foo"
 testlocal1
 echo "2 - foo: $foo"
end

macro test1
 # echo step $1
 if ($1 == 1)
  echo "break here"
  break
 end
 if ($1 == 2)
  continue
 end
end

# continue

macro test2
 echo "input test"
 break
 echo "error, can't get here"
end

macro testm
 break -auto off
 test1 0
 # echo $STATUS
 test1 1
 # echo $STATUS
 test1 2
 # echo $STATUS
 if ($1 == 10) 
  test1 1
  if ($STATUS == 0)
   echo "breaking it"
   break
  end
 end
 echo done
end

echo "done loading"

macro test3
 for i 0 12
  echo $i
  testm $i
  echo "status: $STATUS"
 end
end

macro testL
 for i 0 10
  for j 0 10
   for k 0 10
    echo $i $j $k
    mcreate a 1024 1024   
   end
  end
 end
end
