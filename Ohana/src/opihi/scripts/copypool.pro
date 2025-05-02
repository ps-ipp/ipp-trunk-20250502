
## A test task to demonstrate cycled copying using pending and done pools (directories)
##   and pending queues

queueinit stdout
queueinit stderr
queueinit pending

$indir = dir.01
$outdir = dir.02

# set the test rate to be 1ms
pulse 1000

macro init
  exec rm -r dir.*
  exec mkdir $indir
  exec mkdir $outdir
end

$Nfile = 0
task pool.create
  periods -exec 0.02
  periods -poll 0.02

  task.exec
    sprintf filename "foo.%04d.dat" $Nfile
    $Nfile ++
    host local
    command copypool.sh -create $indir $filename
  end

  task.exit  default
    queueinit stdout
    queueinit stderr
  end
end

task pool.pending
  command copypool.sh -pending $indir
  host local

  periods -exec 5
  periods -poll 1

  # success
  task.exit    0
    local i Nstdout

    # keep only new, unique entries (name is key)
    queuesize stdout -var Nstdout
    for i 0 $Nstdout
      queuepop stdout -var line
      queuepush pending -uniq -key 0 "$line new"
    end
    queueprint pending
  end
end

task pool.copy
  host local

  periods -exec 1
  periods -poll 1

  task.exec
    local Npending
    queuesize pending -var Npending
    if ($Npending == 0) 
      periods -exec 1
      periods -poll 1
      break
    end

    # if data is available, run fast to clear it out
    periods -exec 0.002
    periods -poll 0.002

    # this step grabs and entry by key and updates a field
    # can we do this in one step?
    queuepop pending -var line -key 1 new
    if ("$line" == "NULL") break

    list tmp -split $line
    $name = $tmp:0

    queuepush pending "$name run"

    host local
    command copypool.sh -copy $outdir $name
  end

  # success
  task.exit 0
    echo "done with $taskarg:3"
    queueinit stdout
    queueinit stderr
    queuepop pending -key 0 $taskarg:3 -var line
    # echo "got line: $line"

    if ("$line" == "NULL") 
      echo "missing entry in pending queue?"
      break
    end
  end
end

task pool.list
  host local
  periods -exec 3
  command queueprint pending

  task.exit default
    queueinit stdout
    queueinit stderr
  end
end
