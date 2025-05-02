
$Npass = 0

## DO NOT DO THIS: if pantasks removes pantasks.stderr.log and pantasks.stdout.log, the
## output from the tasks disappears
# exec rm -f output.txt pantasks.stdout.log pantasks.stderr.log
# echo "Npass: $Npass"

# a basic task which just runs 'echo' with an error
task	       basic
  host         local

  periods      -poll 0.5
# periods      -exec 0.0005
  periods      -exec 0.002
# periods      -exec 0.5
  periods      -timeout 2
  
  stdout output.txt
  stderr output.txt

  task.exec
    command echo "test line"
  end

  # success
  task.exit    0
    $Npass ++
    # echo "Npass: $Npass"
    if ($Npass % 1000 == 0)
      date -seconds -reftime 1611900000 -var Nsec
      echo "Npass $Npass : $Nsec"
    end
  end

  # default exit status
  task.exit    default
    echo       "basic: exit status: $EXIT"
  end

  # operation times out?
  task.exit    timeout
    echo       "basic: timeout"
  end
end

