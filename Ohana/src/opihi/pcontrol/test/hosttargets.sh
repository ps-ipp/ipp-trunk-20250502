
# generate a bunch of jobs which are one of the three states NEED, WANT, (ANY?)
# check the rate at which they are placed on the appropriate type of host.

# ANY : is there a valid test for ANY?
# NEED must always be sent to the NEED host
# WANT : report the fraction of time : if job time is << timeout -> WANT host rate should be high
# WANT : report the fraction of time : if job time is >> timeout -> WANT host rate should be low
# WANT : report the fraction of time : if job time is  ~ timeout -> WANT host rate should be ~NN%

macro hosttargets

  if ($0 != 1) 
    echo "USAGE: hosttargets"
    break
  end

  if ($?hostlist:n == 0)
    echo "create a list of target hosts called 'hostlist'"
    break
  end

  stop

  for i 0 $hostlist:n
    # generate a set of jobs with desired targets
    for j 0 10
      job -host $hostlist:$i sleep 1
    end
    host add $hostlist:$i
  end
  
  run

  # wait 'till jobs complete
  sleep 12

  # compare desired with actual host
  # for i 1 20
  #   check job $i
  # end

end

## NOTE : dtime in 'status' is not working
