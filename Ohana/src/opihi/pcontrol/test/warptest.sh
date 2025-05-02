# test script to simulate a pcontrol run with weighted priorities

macro mkjobs
  delete -q warpjobs warptime warpwait warprun
  delete -q stckjobs stcktime stckwait stckrun

  create newjobs 0 100
  set warpjobs = newjobs
  set stckjobs = newjobs + 1000000

  set warptime = zero(warpjobs) + 100
  set stcktime = zero(stckjobs) + 1000

  set warpwait = zero(warpjobs)
  set stckwait = zero(stckjobs)

  set warprun = zero(warpjobs)
  set stckrun = zero(stckjobs)

  delete seq warps stcks

  for i 0 1000
    # make a list of the pending jobs
    subset wpendwait = warpwait if not(warprun)
    subset spendwait = stckwait if not(stckrun)

    subset wpendtime = warptime if not(warprun)
    subset spendtime = stcktime if not(stckrun)

    subset wpendjobs = warpjobs if not(warprun)
    subset spendjobs = stckjobs if not(stckrun)
   
    set wprior = 1 - wpendwait / wpendtime
    set sprior = 1 - spendwait / spendtime

    delete prior pjobs
    concat wprior prior
    concat sprior prior

    concat wpendjobs pjobs
    concat spendjobs pjobs

    sort prior pjobs

    # select 10 jobs
    $Nwarp = 0
    $Nstck = 0
    for j 0 10
      $N = pjobs[$j]
      if ($N < 1000000)
        warprun[$N] = 1
        # echo "warprun: $N : warprun[$N]"
        $Nwarp ++
      else
        stckrun[$N-1000000] = 1
        # echo "stckrun: $N : stckrun[$N-1000000]"
        $Nstck ++
      end
    end

    concat $i seq
    concat $Nwarp warps
    concat $Nstck stcks 

    set warpwait = warpwait + 30
    set stckwait = stckwait + 30

    # vstat -q stckrun; echo "stack: $TOTAL"
    # vstat -q warprun; echo "warp:  $TOTAL"

    if (wpendwait[] < 500) 
      # add new warp jobs
      set warpjobsN = newjobs + warpjobs[-1] + 1
      concat warpjobsN warpjobs
      set warptimeN = zero(warpjobsN) + 100
      concat warptimeN warptime
      set warpwaitN = zero(warpjobsN)
      concat warpwaitN warpwait
      set warprunN = zero(warpjobsN)
      concat warprunN warprun
    end
    if (spendwait[] < 500) 
      # add new stack jobs
      set stckjobsN = newjobs + stckjobs[-1] + 1
      concat stckjobsN stckjobs
      set stcktimeN = zero(stckjobsN) + 100
      concat stcktimeN stcktime
      set stckwaitN = zero(stckjobsN)
      concat stckwaitN stckwait
      set stckrunN = zero(stckjobsN)
      concat stckrunN stckrun
    end
  end

  lim seq -1 11; clear; box
  plot seq warps -c red
  plot seq stcks -c blue
end
