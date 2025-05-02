
task test
  command ls
  periods -poll 1.0
  periods -exec 2.0
  periods -timeout 10.0
  # trange 07:09 07:10
  # trange -exclude 07:09:30 07:09:45
  nmax 5
  host local
  # host localhost
  # local is default 

  task.exec
    # echo "starting job sched.test"
  end

  # stdout / stderr lines on named queues
  task.exit 0
#    echo "task exit 0"
#    queuesize stdout
#    queuesize stderr
    queuedelete stdout
    queuedelete stderr
    memory leaks
  end

  task.exit 1
    echo "task exit 1"
    queuesize stdout
    queuesize stderr
  end

  task.exit crash
    echo "crashed job"
  end

  task.exit timeout
    output timeout.log
    echo $stdout
    output stdout
  end
end
