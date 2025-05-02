
# a basic task which just runs 'echo' with an error
# this should show sensible behavior for Nskip (use status -taskinfo to check)
task	       basic
  host         local

  periods      -poll 0.5
  periods      -exec 0.1
  periods      -timeout 2
  
  stdout tmp.txt
  stderr tmp.txt

  task.exec
    $N = 1
    if ($N == 0) break
    command echo "$Npass : test line"
  end

  # success
  task.exit    0
    $Npass ++
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

