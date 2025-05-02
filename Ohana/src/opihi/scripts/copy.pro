
# identify the images ready for copy 
task	       new.images
  command      new.images
  host         local

  periods      -poll 1
  periods      -exec 30
  periods      -timeout 2

  # success
  task.exit    0
    push new.images $stdout
    $new.image.failure = 0
  end

  # locked list
  task.exit    1
    echo       "new.images: exec failure"
    $new.image.failure ++
  end

  # default exit status
  task.exit    -
    echo       "new.images: unknown exit status: $EXIT"
    $new.image.failure ++
  end

  # operation times out?
  task.exit    timeout
    echo       "new.images: timeout"
    $new.image.failure ++
  end
end

# copy specific images from the summit
task           copy.images

  # these define task properties which are fixed

  period       -exec 2
  period       -poll 1
  period       -timeout 50

  # these commands are executed at the start of a new task
  task.exec
    local        RemoteName
    local        FileID
    local        Host

    queuesize  NewImages -var N
    if ($N > 0) break
    if ($network == 0) break
    if ($filesystem == 1) break

    pop NewImages -var line
    list tmp -split $line
    $RemoteName = $tmp:0
    $FileID     = $tmp:1
    $Host       = $tmp:2
    $Ntry       = $tmp:3

    stderr       /data/logfiles/copy/$FileID.log
    stdout       -queue $FileIDlog

    echo $TaskID
    echo $TaskName
    spawn copy.images $RemoteName $FileID $Host -host $Host
  end

  # success
  task.exit      0
    $new.image.failure --
  end

  # summmit connection failed
  task.exit      1
    echo         "copy.images: copy failed $RemoteName $FileID"
    push         NewImages "$RemoteName $FileID $Host 0"
    $copy.image.failure ++
  end

  # target disk full
  task.exit      2
    echo         "copy.images: disk full"
    push         NewImages "$RemoteName $FileID $Host 0"
    $copy.image.failure ++
  end

  # file not found
  task.exit      3
    echo         "copy.images: missing file"
    # send a message to OTIS?
  end

  # task timed out
  task.exit      timeout
    echo        "copy.images: timeout"
    push        NewImages "$RemoteName $FileID $Host 0"
    $copy.image.failure ++
  end
end

# identify the images ready for copy 
TASK	       new.images
  COMMAND      new.images
  HOST         -
  STDERR       /data/logfiles/new.images.log
  STDOUT       $stdout
  EXEC_PERIOD  30
  POLL_PERIOD  1

  # success
  EXIT         0
    STDOUT     @new.images
    $new.image.failure = 0
  END

  # locked list
  EXIT         1
    MESSAGE    "new.images: exec failure"
    $new.image.failure ++
  END

  # default exit status
  EXIT         -
    MESSAGE    "new.images: unknown exit status: $EXIT"
    $new.image.failure ++
  END

  # operation times out?
  TIMEOUT      2
    MESSAGE    "new.images: timeout"
    $new.image.failure ++
  END
END
