
verbose on
queueinit newImages
exec echo 0 > new.last
exec cp -f raw.list new.list

controller host add po01
controller host add po02
controller host add po03
controller host add po04

# identify the images ready for copy 
# new entries are added to queue newImages
# need to compare the new list with the ones already being processed
task	       new.images
  command      new.images
  host         local

  periods      -poll 1
  periods      -exec 5
  periods      -timeout 5

  # success
  task.exit    0
    local i j Nstdout Nimages
    # compare output with new.image queue
    # keep only new entries
    queuesize stdout -var Nstdout
    for i 0 $Nstdout
      queuepop stdout -var line
      queuepush newImages -uniq -key 0 "$line"
    end
  end

  # locked list
  task.exit    1
    echo       "new.images: exec failure"
    $new.image.failure ++
  end

  # default exit status
  task.exit    default
    echo       "new.images: unknown exit status: $EXIT"
    $new.image.failure ++
  end

  # operation times out?
  task.exit    timeout
    echo       "new.images: timeout"
    $new.image.failure ++
  end
end

# copy new images, sending job to desired host
task	       copy.images
  periods      -poll 0.2
  periods      -exec 1
  periods      -timeout 5

  task.exec
    queuesize  newImages -var N
    if ($N == 0) break
    # if ($network == 0) break
    # if ($filesystem == 1) break
    
    queuepop newImages -var line
    list tmp -split $line
    $filename   = $tmp:0
    $chip       = $tmp:1
    $state      = $tmp:2
    if ($state == new) 
      # copy this image
      queuepush newImages -replace -key 0 "$filename $chip run"
    else
      # ignore this image
      queuepush newImages -replace -key 0 "$filename $chip $state"
      break
    end
    # echo $chip
    $host = `chip.host $chip`
    # echo $host
    host $host
    # echo "starting copy for $filename on $host..."
    command copy.image $filename $chip
  end

  # can I have access to argc,argv?

  # success
  task.exit    0
    echo "done copy..."
    queuepop stdout -var line
    list tmp -split $line
    $filename   = $tmp:0
    $chip       = $tmp:1
    exec mark.image $filename
    queuepush newImages -replace -key 0 "$filename $chip copy"
  end

  # default exit status
  task.exit    default
    echo       "new.images: unknown exit status: $EXIT"
    $new.image.failure ++
  end

  # operation times out?
  task.exit    timeout
    echo       "new.images: timeout"
    $new.image.failure ++
  end
end
