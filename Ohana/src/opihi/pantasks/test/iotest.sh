
$BLOCK = 0

macro testloop
  queuepush stdout start
  for i 0 5
    echo $i
    queueprint stdout
  end
end

# a basic task which just runs 'echo foobar'
task	       basic
  command      echo foobar
  host         local

  periods      -poll 0.1
  periods      -exec 1.0
  periods      -timeout 20
  active       false
  npending 5
  
  stdout tmp.txt
  stderr tmp.txt

  task.exec
    if ($BLOCK) break
    queuepush stdout start
    echo "command start"
    queuelist
    queueprint stdout
    queueinit stdout
    queuelist
    echo "command stop"
  end

  # success
  task.exit    0
    $BLOCK = 1
    echo "exit start"
    queuelist
    queueprint stdout
    queueinit stdout
    queuelist
    echo "exit stop"
    $BLOCK = 0
  end

  # default exit status
  task.exit    default
    echo       "failure: exit status: $EXIT"
  end

  # operation times out?
  task.exit    timeout
    echo       "timeout"
  end
end

macro show.book
  if ($0 != 2)
   echo "USAGE: show.book (book)"
   break
  end

  book npages $1 -var npages
  for i 0 $npages
    book getpage $1 $i -var pagename
    book listpage $1 $pagename
  end

  echo "npages: $npages"
end

macro load.book
  queueload stdout -x "cat iotest.dat"
  ipptool2book stdout testbook -key chip_id
end
