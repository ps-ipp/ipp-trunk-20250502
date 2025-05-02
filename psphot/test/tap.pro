# -*-sh-*-

macro tapOK
  if ($0 != 3) 
    echo "USAGE: tapOK (condition) (message)"
    break
  end

  if ($1)
    echo "ok : $2"
    $TAP_LAST = 1
  else
    echo "not ok : $2"
    $TAP_NFAIL ++
    $TAP_LAST = 0
    if ($TAP_BREAK)
     break
    end
  end
  $TAP_NDONE ++
end

macro tapPLAN
  if ($0 != 2) 
    echo "USAGE: tapPLAN (Ntests)"
    break
  end

  $TAP_NTEST = $1
  $TAP_NFAIL = 0
  $TAP_NDONE = 0
  if (not($?TAP_BREAK)) set TAP_BREAK = 0
end

macro tapDONE
  if ($0 != 1) 
    echo "USAGE: tapDONE"
    break
  end

  if ($TAP_NDONE != $TAP_NTEST) 
    echo "planned tests ($TAP_NTEST) not equal to done tests ($TAP_NDONE)"
  end

  if ($TAP_NFAIL) 
    echo "failed $TAP_NFAIL of $TAP_NDONE"
  end

  if ($TAP_NFAIL == 0) 
    echo "passed $TAP_NDONE tests"
  end
end
