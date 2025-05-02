# -- sh-mode -- 

# this pantasks script generates jobs in a sequence of stages.  as all
# jobs for a given stage complete, the jobs in the next stage are
# launched.  load the script and start pantasks running.  no jobs will
# execute until the user runs the 'mkhosts' command (this can only be
# run after the initialization stage is run)

# we have 1 initial data source and we want to rsync out to a large number.  this seems like a good use of a binary tree.  

$srcdir = /local/ipp/psconfig

$test = 0

$delete = 
# $delete = --delete
# set backup to 0 to restore on original machine, use $delete to only keep current

if not($?Ntarget) set Ntarget = 0
if not($?stage)   set stage = -1
if not($?Nsrc)    set Nsrc = 0
if not($?Ndone)   set Ndone = 0

if not($?LOGDIR) 
  $today = `date +%Y%m%d`
  $LOGDIR = logs.$today
end

mkdir $LOGDIR

macro mktargets
  local i N

  $target:0 = ippc12
  $N = 1
  for i 1 64
    if ($i == 12) continue
    if ($i ==  6) continue
    sprintf target:$N ippc%02d $i
    $N ++
  end  

  for i 1 104
    if ($i == 71) continue
    if ($i == 37) continue
    if ($i == 38) continue
    if ($i == 39) continue
    if ($i == 40) continue
    if ($i == 41) continue
    if ($i == 42) continue
    if ($i == 43) continue
    if ($i == 44) continue
    sprintf target:$N ippx%03d $i
    $N ++
  end  
  $target:n = $N
end

macro show.targets
  for i 0 $target:n
    echo $target:$i
  end  
end

# just make all connections so we can run 
macro mkhosts
 for i 0 $target:n
   control host add $target:$i
 end
end

# define the current hosts and targets
macro next.stage
  if ($0 != 1)
    echo "USAGE: next.stage"
    break
  end

  # for a given value of stage N, we are rsyncing target:0 through target:2^N-1 over to 2^N to 2^(N+1)-1 
  # eg, stage 0 :  target:0 -> target:1
  #     stage 1 :  target:0 -> target:2
  #     stage 1 :  target:1 -> target:3

  $stage ++

  # each stage, we have 2^stage sources and targets
  $Ntarget = 2^$stage
  $Nsrc = 0
  $Ndone = 0
end

# define the current hosts and targets
macro set.stage
  if ($0 != 2)
    echo "USAGE: set.stage (stage)"
    break
  end

  # for a given value of stage N, we are rsyncing target:0 through target:2^N-1 over to 2^N to 2^(N+1)-1 
  # eg, stage 0 :  target:0 -> target:1
  #     stage 1 :  target:0 -> target:2
  #     stage 1 :  target:1 -> target:3

  $stage = $1

  # each stage, we have 2^stage sources and targets
  $Ntarget = 2^$stage
  $Nsrc = 0
  $Ndone = 0
end

macro show.stage
  echo "stage: $stage"
  echo "Ntarget: $Ntarget"  
  echo "Nsrc: $Nsrc"
  echo "Ndone: $Ndone"
end

task mkjobs
  periods -poll 0.5
  periods -timeout 10
  host none

  task.exec 
    periods -exec 0.2

    if (not($?target:n)) 
      periods -exec 5.0
      break
    end

    if ($Nsrc >= $Ntarget) 
      periods -exec 5.0
      break
    end

    $Ntgt = $Ntarget + $Nsrc

    if ($Ntgt >= $target:n) 
      periods -exec 5.0
      break
    end

    $src = $target:$Nsrc
    $tgt = $target:$Ntgt

    host -required $tgt

    stdout $LOGDIR/$tgt.log
    stderr $LOGDIR/$tgt.log

    $run = rsync --bwlimit=3000 -auv $src\:$srcdir/ $srcdir/

    if ($test)
      command echo $run
    else
      command $run
    end

    $Nsrc ++
  end

  task.exit default
    $Ndone ++
  end
end

task advance.stage
  periods -poll 1.0
  periods -exec 1.0
  periods -timeout 10
  host local

  task.exec 
    if not($?target:n) 
      mktargets
    end

    if ($Ndone >= $Ntarget)
      echo "done with stage $stage, advancing to next stage"
      next.stage
      if ($stage == 0)
        echo "type 'mkhosts' to start processing'"
      end
      periods -exec 5.0
      break
    end      

    periods -exec 1.0
    break    

    command echo advance stage should not actually execute...
  end
end
