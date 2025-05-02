
# a basic task which just runs 'echo'
task	       basic
  host         local

  periods      -poll 0.005
  periods      -exec 0.005
  periods      -timeout 2
  command      ls -l /usr/bin /tmp/foobar
  
  stdout tmp.txt
  stderr tmp.txt

  # success
  task.exit    0
    $Npass ++
  end

  # default exit status
  task.exit    default
    echo       "basic: exit status: $JOB_STATUS"
  end

  # operation times out?
  task.exit    timeout
    echo       "basic: timeout"
  end
end

