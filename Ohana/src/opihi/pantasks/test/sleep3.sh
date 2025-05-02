
$hostname = `hostname`

controller exit true
controller host add po02
controller host add po03
controller host add po04
controller host add po05
controller host add po06
controller host add po07
controller host add po08
controller host add po09
controller host add po10

# a basic task which just runs 'sleep'
task	       basic
  command      sleep 1
  host         anyhost

  periods      -poll 0.01
  periods      -exec 0.02
  periods      -timeout 20
  npending 100
  
  stdout tmp.txt
  stderr tmp.txt

  task.exec
    # echo "create command"
  end

  # success
  task.exit    0
    $Npass ++
    # echo done sleep
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
