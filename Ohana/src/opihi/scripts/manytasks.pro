
controller exit true
controller host add kiawe

$Ntest = 0
pulse 1000
controller pulse 1000

macro load.machines
  if ($0 != 2)
    echo "load.machines (nmach)"
    break
  end

  for i 0 $1
    $n = $i + 1
    sprintf host "po%02d" $n
    controller host add $host
  end
end

macro mktask
  if ($0 != 2)
    echo "USAGE: mktask (n)"
    break
  end

  $N = $1

  task test.$N
    command partest
    # polling period is no longer valid: we check for completed controller tasks
    # correction: still valid for local tasks
    periods -poll 0.2
    periods -exec 0.01
    periods -timeout 10.0
    nmax 20
    host anyhost

    # stdout / stderr lines on named queues
    task.exit 0
      # echo "task exit 0"
      queuedelete stdout
      queuedelete stderr
      date date
      queuepush done "$date"
      $Ntest ++
    end

    task.exit 1
      # echo "task exit 1"
      queuesize stdout
      queuesize stderr
    end

    task.exit crash
      echo "crashed job"
    end

    task.exit timeout
      output timeout.log
      echo $stdout
      output stdout
    end
  end
end

macro mktasks
 for i 0 $1
  mktask $i
 end
end
