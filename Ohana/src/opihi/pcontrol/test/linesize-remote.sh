# we are having trouble with pcontrol -> pclient comms with very long lines (> 512)
# this seems to be a problem with readline buffering

# generate jobs with very long lines
# XXX this must be run as the only pass in pcontrol (to get the JobIDs right)

macro test
  if ($0 != 2)
    echo "USAGE: test (remotehost)"
    break
  end

  host add $1

  $inline1 = abcdefghijklmnopqrstuvwxyz
  $inline2 = $inline1+$inline1
  $inline3 = $inline2-$inline2
  $inline4 = $inline3=$inline3
  $inline5 = $inline4*$inline4
  $inline6 = $inline5^$inline5
  $inline7 = $inline6%$inline6

  strlen "inline1: $inline1"
  strlen "inline2: $inline2"
  strlen "inline3: $inline3"
  strlen "inline4: $inline4"
  strlen "inline5: $inline5"
  strlen "inline6: $inline6"
  strlen "inline7: $inline7"

  for i 1 10
    job echo $inline7
  end

  sleep 5

  for i 1 10
    stdout $i -var outline
    strlen "$inline7" Nin
    strlen "$outline" Nout

    echo Nin: $Nin, Nout: $Nout

    if ($Nin != $Nout - 1)
      echo "mismatch in length!"
    end
  end
end
