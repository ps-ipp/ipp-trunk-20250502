## receive.pro: -*- sh -*-

### receivetool -definesource: add new receive source (run by user for setup)
### receivetool -list: get list of sources (for pantasks setup)
### 
### receive_source.pl: launched regularly by pantasks; check source for new filesets
### dsproductls: get available filesets from datastore
### receivetool -addfileset: add new fileset
### receivetool -updatelast: update last fileset for source
### 
### receivetool -pendingfileset: get pending fileset (for pantasks)
### receive_fileset.pl: launched by pantasks; get list of files
### dsfilesetls: get files for fileset
### receivetool -addfiles: add file to receive
### 
### receivetool -pendingfile: get pending file (for pantasks)
### receive_file.pl: launched by pantasks; download file, extract fileset, import
### dsget: get file from datastore
### receivetool -addresult: add result
### 
### receivetool -revert: remove result after something goes wrong
###
### receivetool -toadvance: filesets for which all files have been downloaded and are ready
### to complete processing

# test for required global variables
check.globals

book init receiveSource
book init receiveFileset
book init receiveFile
book init receiveAdvance

macro receive.status
  book listbook receiveSource
  book listbook receiveFileset
  book listbook receiveFile
  book listbook receiveAdvance
end

macro receive.reset
  book init receiveSource
  book init receiveFileset
  book init receiveFile
  book init receiveAdvance
end

macro receive.on
  task receive.source.load
    active true
  end
  task receive.source.run
    active true
  end
  task receive.fileset.load
    active true
  end
  task receive.fileset.run
    active true
  end
  task receive.file.load
    active true
  end
  task receive.file.run
    active true
  end
  task receive.advance.load
    active true
  end
  task receive.advance.run
    active true
  end
end

macro receive.off
  task receive.source.load
    active false
  end
  task receive.source.run
    active false
  end
  task receive.fileset.load
    active false
  end
  task receive.fileset.run
    active false
  end
  task receive.file.load
    active false
  end
  task receive.file.run
    active false
  end
  task receive.advance.load
    active false
  end
  task receive.advance.run
    active false
  end
end


# this variable will cycle through the known database names
$receive_DB = 0
$receive_Advance_DB = 0

# set this to skip extraction of tarfiles into their destination directories
$NO_EXTRACT = ""
macro set.no.extract
    $NO_EXTRACT = "--no-extract"
end
macro clear.no.extract
    $NO_EXTRACT = ""
end
macro show.no.extract
    echo $NO_EXTRACT
end

task	       receive.source.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/receive.source.log

  task.exec
    $run = receivetool -list
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$receive_DB
      $run = $run -dbname $DB:$receive_DB
      $receive_DB ++
      if ($receive_DB >= $DB:n) set receive_DB = 0
    end
    add_poll_args run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout receiveSource -key source_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook receiveSource
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup receiveSource
  end

  # locked list
  task.exit    default
    showcommand failure
  end

  task.exit    crash
    showcommand crash
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
  end
end

task	       receive.source.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60

  task.exec
    book npages receiveSource -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    book getpage receiveSource 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword receiveSource $pageName pantaskState RUN
    book getword receiveSource $pageName source_id -var SOURCE_ID
    book getword receiveSource $pageName source -var SOURCE
    book getword receiveSource $pageName product -var PRODUCT
    book getword receiveSource $pageName fileset_last -var FILESET
    book getword receiveSource $pageName dbname -var DBNAME
    book getword receiveSource $pageName state -var RUN_STATE

    stdout $LOGDIR/receive.source.log
    stderr $LOGDIR/receive.source.log

    $run = receive_source.pl --source_id $SOURCE_ID --source $SOURCE --product $PRODUCT
    if ("$FILESET" != "NULL")
      $run = $run --last_fileset $FILESET
    end
    add_standard_args run

    # save the pageName for future reference below
    options $pageName

    # create the command line
    if ($VERBOSE > 1)
      echo command $run
    end
    command $run
  end

  # default exit status
  task.exit    default
    process_exit receiveSource $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword receiveSource $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword receiveSource $options:0 pantaskState TIMEOUT
  end
end


task	       receive.fileset.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/receive.fileset.log

  task.exec
    $run = receivetool -pendingfileset
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$receive_DB
      $run = $run -dbname $DB:$receive_DB
      $receive_DB ++
      if ($receive_DB >= $DB:n) set receive_DB = 0
    end
    add_poll_args run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout receiveFileset -key fileset_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook receiveFileset
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup receiveFileset
  end

  # locked list
  task.exit    default
    showcommand failure
  end

  task.exit    crash
    showcommand crash
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
  end
end

task	       receive.fileset.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60

  task.exec
    book npages receiveFileset -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    book getpage receiveFileset 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword receiveFileset $pageName pantaskState RUN
    book getword receiveFileset $pageName fileset_id -var FILESET_ID
    book getword receiveFileset $pageName source -var SOURCE
    book getword receiveFileset $pageName product -var PRODUCT
    book getword receiveFileset $pageName fileset -var FILESET
    book getword receiveFileset $pageName dbname -var DBNAME
    book getword receiveFileset $pageName state -var RUN_STATE

    stdout $LOGDIR/receive.fileset.log
    stderr $LOGDIR/receive.fileset.log

    host anyhost

    $run = receive_fileset.pl --fileset_id $FILESET_ID --source $SOURCE --product $PRODUCT --fileset $FILESET
    add_standard_args run

    # save the pageName for future reference below
    options $pageName

    # create the command line
    if ($VERBOSE > 1)
      echo command $run
    end
    command $run
  end

  # default exit status
  task.exit    default
    process_exit receiveFileset $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword receiveFileset $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword receiveFileset $options:0 pantaskState TIMEOUT
  end
end


task	       receive.file.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/receive.file.log

  task.exec
    $run = receivetool -pendingfile
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$receive_DB
      $run = $run -dbname $DB:$receive_DB
      $receive_DB ++
      if ($receive_DB >= $DB:n) set receive_DB = 0
    end
    add_poll_args run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout receiveFile -key file_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook receiveFile
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup receiveFile
  end

  # locked list
  task.exit    default
    showcommand failure
  end

  task.exit    crash
    showcommand crash
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
  end
end

task	       receive.file.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 600

  task.exec
    book npages receiveFile -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    book getpage receiveFile 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword receiveFile $pageName pantaskState RUN
    book getword receiveFile $pageName file_id -var FILE_ID
    book getword receiveFile $pageName source -var SOURCE
    book getword receiveFile $pageName product -var PRODUCT
    book getword receiveFile $pageName fileset -var FILESET
    book getword receiveFile $pageName fileset_id -var FILESET_ID
    book getword receiveFile $pageName file -var FILE
    book getword receiveFile $pageName bytes -var BYTES
    book getword receiveFile $pageName md5sum -var MD5SUM
    book getword receiveFile $pageName component -var COMPONENT
    book getword receiveFile $pageName workdir -var WORKDIR
    book getword receiveFile $pageName dirinfo -var DIRINFO
    book getword receiveFile $pageName dbname -var DBNAME
    book getword receiveFile $pageName state -var RUN_STATE

    stdout $LOGDIR/receive.file.log
    stderr $LOGDIR/receive.file.log

    host anyhost

    $run = receive_file.pl --file_id $FILE_ID --source $SOURCE --product $PRODUCT --fileset $FILESET --fileset_id $FILESET_ID --file $FILE --component $COMPONENT --bytes $BYTES --md5 $MD5SUM --workdir $WORKDIR --dirinfo $DIRINFO $NO_EXTRACT
    add_standard_args run

    # save the pageName for future reference below
    options $pageName

    # create the command line
    if ($VERBOSE > 1)
      echo command $run
    end
    command $run
  end

  # default exit status
  task.exit    default
    process_exit receiveFile $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword receiveFile $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword receiveFile $options:0 pantaskState TIMEOUT
  end
end

task	       receive.advance.load
  host         local

  periods      -poll $LOADPOLL
  periods      -exec $LOADEXEC
  periods      -timeout 30
  npending     1

  stdout NULL
  stderr $LOGDIR/receive.advance.log

  task.exec
    $run = receivetool -toadvance
    if ($DB:n == 0)
      option DEFAULT
    else
      # save the DB name for the exit tasks
      option $DB:$receive_Advance_DB
      $run = $run -dbname $DB:$receive_Advance_DB
      $receive_Advance_DB ++
      if ($receive_Advance_DB >= $DB:n) set receive_Advance_DB = 0
    end
    add_poll_args run
    command $run
  end

  # success
  task.exit    0
    # convert 'stdout' to book format
    ipptool2book stdout receiveAdvance -key fileset_id -uniq -setword dbname $options:0 -setword pantaskState INIT
    if ($VERBOSE > 2)
      book listbook receiveAdvance
    end

    # delete existing entries in the appropriate pantaskStates
    process_cleanup receiveAdvance
  end

  # locked list
  task.exit    default
    showcommand failure
  end

  task.exit    crash
    showcommand crash
  end

  # operation times out?
  task.exit    timeout
    showcommand timeout
  end
end

task	       receive.advance.run
  periods      -poll $RUNPOLL
  periods      -exec $RUNEXEC
  periods      -timeout 60

  task.exec
    book npages receiveAdvance -var N
    if ($N == 0) break
    if ($NETWORK == 0) break
    
    book getpage receiveAdvance 0 -var pageName -key pantaskState INIT
    if ("$pageName" == "NULL") break

    book setword receiveAdvance $pageName pantaskState RUN
    book getword receiveAdvance $pageName fileset_id -var FILESET_ID
    book getword receiveAdvance $pageName fileset -var FILESET
    book getword receiveAdvance $pageName dbinfo -var DBINFO
    book getword receiveAdvance $pageName status_product -var STATUS_PRODUCT
    book getword receiveAdvance $pageName ds_dbname -var DS_DBNAME
    book getword receiveAdvance $pageName ds_dbhost -var DS_DBHOST
    book getword receiveAdvance $pageName dbname -var DBNAME

    stdout $LOGDIR/receive.advance.log
    stderr $LOGDIR/receive.advance.log

    host anyhost

    $run = receive_advance.pl --fileset_id $FILESET_ID --fileset $FILESET --dbinfo $DBINFO --status_product $STATUS_PRODUCT --ds_dbname $DS_DBNAME --ds_dbhost $DS_DBHOST
    add_standard_args run

    # save the pageName for future reference below
    options $pageName

    # create the command line
    if ($VERBOSE > 1)
      echo command $run
    end
    command $run
  end

  # default exit status
  task.exit    default
    process_exit receiveAdvance $options:0 $JOB_STATUS
  end

  # locked list
  task.exit    crash
    showcommand crash
    echo "hostname: $JOB_HOSTNAME"
    book setword receiveAdvance $options:0 pantaskState CRASH
  end

  # operation timed out?
  task.exit    timeout
    showcommand timeout
    book setword receiveAdvance $options:0 pantaskState TIMEOUT
  end
end
