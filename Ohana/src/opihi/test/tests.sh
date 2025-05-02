
if ($?VERBOSE == 0)
 $VERBOSE = 0
end

$failtest:n = 0
$failfile:n = 0
$faildirs:n = 0
$currentdir = .

macro fulltests
  $Npass = 0
  $Nfail = 0
  $Ntest = 0
  break -auto off

  $failtest:n = 0
  $failfile:n = 0
  $faildirs:n = 0

  for Ti 0 $testdir:n
    if ($VERBOSE > 0)
       echo "directory $testdir:$Ti"
    end
    runtestdir $testdir:$Ti
  end

  echo "completed $Ntest tests"
  echo "$Npass tests passed"
  echo "$Nfail tests failed"
  echo "examined $testdir:n directories"

  if ($Nfail > 0) 
    echo " ** failed tests **"
    for i 0 $Nfail
      echo $faildirs:$i $failfile:$i $failtest:$i
    end
  end
end

macro runtestdir
  if ($0 != 2)
    echo "USAGE: runtestdir (testdir)"
    break -auto on
    break
  end
  
  pwd -var startdir
  cd -q $1
  list testscripts -x "ls *.sh"

  $currentdir = $1
  for Tj 0 $testscripts:n
    if ($VERBOSE > 1)
      echo " ----- running $testscripts:$Tj -----"
    end
    runtests $testscripts:$Tj      
  end
  
  cd -q $startdir
end

macro runtests
  if ($0 != 2)
    echo "USAGE: runtests (script.sh)"
    break -auto on
    break
  end
  
  local Tk

  input $1
  for Tk 0 $tests:n
    if ($VERBOSE > 2)
      echo "   running $tests:$Tk"
    end
    $PASS = 1
    $tests:$Tk
    if ($PASS == 0)
      echo "   ** failed $tests:$Tk $1"
      $Nfail ++
      $n = $failtest:n
      $failtest:$n = $tests:$Tk
      $failfile:$n = $1
      $faildirs:$n = $currentdir
      $failtest:n ++
      $failfile:n ++
      $faildirs:n ++
    else
      $Npass ++
      if ($VERBOSE > 2)
        echo " $tests:$Tk : success"
      end
    end
    $Ntest ++
    # echo "finished $tests:$Tk"
  end
end

echo "USAGE:"
echo "runtests (script.sh)"
echo "runtestdir (testdir)"
echo "set VERBOSE to 0 through 3"
