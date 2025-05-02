## pantasks.pro : globals and support macros : -*- sh -*-

# globals that may be modified by the user -- only init if not set
if ($?NEBULOUS == 0)  	  set NEBULOUS = 0
if ($?NETWORK == 0)   	  set NETWORK = 1
if ($?BURNTOOLING == 0)   set BURNTOOLING = 0
if ($?PARALLEL == 0)  	  set PARALLEL = 1
if ($?VERBOSE == 0)   	  set VERBOSE = 1
if ($?LABEL:n == 0)   	  set LABEL:n = 0
if ($?POLL_LIMIT == 0) 	  set POLL_LIMIT = 32
if ($?KEEP_FAILURES == 0) set KEEP_FAILURES = 0

if ($?LOGDIR == 0) 
  $LOGDIR = `pwd`
  $LOGDIR = $LOGDIR/pantasks_logs
  mkdir $LOGDIR
end

# globals used to control system behavior : these should be in uppercase
$LOADPOLL = 1.0
$LOADEXEC = 5.0

$RUNPOLL  = 0.5
$RUNEXEC  = 2.5

$EXIT_SUCCESS     = 0
$EXIT_UNKNOWN_ERR = 1
$EXIT_SYS_ERR     = 2
$EXIT_CONFIG_ERR  = 3
$EXIT_PROG_ERR    = 4
$EXIT_DATA_ERR    = 5
$EXIT_CRASH_ERR   = 256

# DB lists the database names in use; the default ipprc database
# is only used if DB:n is 0
$DB:n = 0

# very basic values: set these with init.copy.mhpcc
$default_host     = any
$workdir_template = `pwd`

# user functions to manage databases
macro add.database -c "add a database to this pantasks"
  if ($0 != 2)
    echo "USAGE: add.database (db)"
    break
  end
  if ($?DB:n == 0)
    list DB -add $1
    return
  end

  local found
  $found = 0
  for i 0 $DB:n
    if ($DB:$i == $1) 
      $found = 1
      echo "$DB:$i set"
      last
    end
  end
  
  if ($found == 0)
    list DB -add $1
  end
end

macro del.database -c "remove a database from this pantasks" 
  if ($0 != 2)
    echo "USAGE: del.database (db)"
    break
  end
  if ($?DB:n == 0)
    return
  end

  list DB -del $1
end

macro show.databases -c "list the databases for this pantasks"
  if ($0 != 1)
    echo "USAGE: show.databases"
    break
  end
  if ($?DB:n == 0)
    echo "no databases defined"
  end
  if ($DB:n == 0)
    echo "no databases defined"
  end

  local i
  for i 0 $DB:n
    echo $DB:$i
  end
end

# user functions to manipulate labels
macro add.label -c "add a label to this pantasks"
  if ($0 != 2)
    echo "USAGE: add.label (label)"
    break
  end
  if ($?LABEL:n == 0)
    list LABEL -add $1
    return
  end

  local found
  $found = 0
  for i 0 $LABEL:n
    if ($LABEL:$i == $1) 
      $found = 1
      echo "$LABEL:$i set"
      last
    end
  end
  
  if ($found == 0)
    list LABEL -add $1
  end
end

macro del.label -c "remove a label from this pantasks"
  if ($0 != 2)
    echo "USAGE: del.label (label)"
    break
  end
  if ($?LABEL:n == 0)
    return
  end

  list LABEL -del $1
end

macro show.labels -c "list labels for this pantasks"
  if ($0 != 1)
    echo "USAGE: show.labels"
    break
  end
  if ($?LABEL:n == 0)
    echo "no labels defined"
  end
  if ($LABEL:n == 0)
    echo "no labels defined"
  end

  local i
  for i 0 $LABEL:n
    echo $LABEL:$i
  end
end

macro save.labels -c "save the currently defined labels in the file labels.list"
  if ($0 != 1)
    echo "USAGE: save.labels"
    break
  end
  if ($?LABEL:n == 0)
    echo "no labels defined"
  end
  if ($LABEL:n == 0)
    echo "no labels defined"
  end

  exec rm -f labels.list
  local i
  for i 0 $LABEL:n
    exec echo $LABEL:$i >> labels.list
  end
end

macro load.labels -c "read a list of labels from the file labels.list"
  if ($0 != 1)
    echo "USAGE: load.labels"
    break
  end

  file labels.list found
  if (not($found)) 
    echo "no saved labels in labels.list"
    return
  end
 
  list mylabels -x "cat labels.list"
  local i
  for i 0 $mylabels:n
    add.label $mylabels:$i
  end
end

## XXX these are very Pan-STARRS specific
macro init.isp
  list DB -add isp
end

macro init.essence
  list DB -add essence_v2
end

macro init.simtest.local
  list DB -add simtest
  $PARALLEL = 0
  controller exit true
end

macro init.simtest.default.db
  $host = `hostname`
  if ($PARALLEL) 
    controller exit true
    controller host add $host
  end
end

macro init.simtest
  list DB -add simtest

  $host = `hostname`
  if ($PARALLEL) 
    controller exit true
    controller host add $host
  end
end

# need the ability to activate or deactivate specific tasks...

# XXX move this to the detrend pantasks:
macro detrend.on
  detproc.on
  detstack.on
  detnorm.on
  detresid.on
  detreject.on
  detcorr.on
end

macro detrend.off
  detproc.off
  detstack.off
  detnorm.off
  detresid.off
  detreject.off
  detcorr.off
end

macro detrend.revert.off
  detproc.revert.off
  detstack.revert.off
  detnorm.revert.off
  detresid.revert.off
end

macro detrend.revert.on
  detproc.revert.on
  detstack.revert.on
  detnorm.revert.on
  detresid.revert.on
end

macro detrend.reset
  detproc.reset
  detstack.reset
  detnorm.reset
  detresid.reset
  detreject.reset
end

# XXX move this to the stdscience pantasks, remove detrend & flatcorr
macro all.on
  detrend.on
  flatcorr.on
  chip.on
  camera.on
  addstar.on
  fake.on
  warp.on
  diff.on
  stack.on
end

macro all.off
  detrend.off
  flatcorr.off
  chip.off
  camera.off
  addstar.off
  fake.off
  warp.off
  diff.off
  stack.off
end

# move to specific pantasks inputs
macro module.tasks
  module register.pro
  module detrend.process.pro
  module detrend.stack.pro
  module detrend.norm.pro
  module detrend.resid.pro
  module detrend.reject.pro
  module detrend.correct.pro
  module flatcorr.pro
  module chip.pro
  module camera.pro
  module addstar.pro
  module fake.pro
  module warp.pro
  module diff.pro
  module stack.pro
end

macro detrend.modules
  module detrend.process.pro
  module detrend.stack.pro
  module detrend.norm.pro
  module detrend.resid.pro
  module detrend.reject.pro
  module detrend.correct.pro
end

macro showcommand -c "print out the current command (used within tasks)"
  local n 

  if ($VERBOSE)
    $command = $taskarg:0
    for n 1 $taskarg:n
      $command = $command $taskarg:$n
    end
    date -var exitdate
    if ($0 == 2)
      echo ""
      echo "$1 for: $command"
      echo "job exit status: $JOB_STATUS"
      echo "job host: $JOB_HOSTNAME"
      echo "job dtime: $JOB_DTIME"
      echo "job exit date: $exitdate"
    else
      echo "command: $command"
    end    
  end
end

# Add standard arguments to the poll functions
macro add_poll_args -c "add standard poll options to commands (used within tasks)"
    if ($0 != 2)
	echo "Must pass in the command of interest, and this function will supplement"
	stop
    end

    local command i
    $command = $$1 -limit $POLL_LIMIT

    $$1 = $command
end

macro add_poll_labels -c "add standard labels to poll functions (used within tasks)"
    if ($0 != 2)
	echo "Must pass in the command of interest, and this function will supplement"
	stop
    end

    local command i

    $command = $$1

    # Only process the data with the specified label.
    for i 0 $LABEL:n
      $command = $command -label $LABEL:$i
    end

    $$1 = $command
end

macro add_standard_args -c "add standard options (dbname, no-op, no-update) to commands (used within tasks)"
  if ($0 != 2)
    echo "Must pass in the command of interest, and this function will supplement"
    stop
  end

  local command
  $command = $$1

  if ($?DBNAME && "$DBNAME" != "DEFAULT")
    $command = $command --dbname $DBNAME
  end
  if ($?NOOP != 0) 
    if ($NOOP != 0)
      $command = $command --no-op
    end
  end
  if ($?NOUPDATE != 0) 
    if ($NOUPDATE != 0)
      $command = $command --no-update
    end
  end
  $$1 = $command --verbose
end


macro process_exit -c "common handler for task exit"
  if ($0 != 4)
    echo "USAGE: process_exit (bookName) (pageName) (exitCode)"
    break
  end

  $bookName = $1
  $pageName = $2
  $exitCode = $3

  if ($VERBOSE > 4)
    echo "*** exit status ***"
    echo "JOB_STATUS: $JOB_STATUS"
    echo "JOB_HOSTNAME: $JOB_HOSTNAME"
    echo "JOB_DTIME: $JOB_DTIME"
    echo "*** stdout ***"
    queueprint stdout
    echo "*** stderr ***"
    queueprint stderr
  end

  # success
  if ($exitCode == $EXIT_SUCCESS) 
    # the handler scripts update DB the tables; here we just update the page
    book setword $bookName $pageName pantaskState DONE
    return
  end

  # failure related to the data files
  # jobs which result in DATAERR must have db pantaskState updated 
  if ($exitCode == $EXIT_DATA_ERR)
    showcommand failure
    book setword $bookName $pageName pantaskState DATA_ERR
    return
  end

  if ($VERBOSE)
    echo "*** stdout ***"
    queueprint stdout
    echo "*** stderr ***"
    queueprint stderr
  end

  # failure related to the data files
  if ($exitCode == $EXIT_SYS_ERR)
    # stop
    showcommand "system failure"
    book setword $bookName $pageName pantaskState SYS_ERR
    return
  end

  # failure related to the data files
  if ($exitCode == $EXIT_CONFIG_ERR)
    # stop
    showcommand "config error"
    book setword $bookName $pageName pantaskState CONFIG_ERR
    return
  end

  # failure related to the data files
  if ($exitCode == $EXIT_PROG_ERR)
    # stop
    showcommand "programming error"
    book setword $bookName $pageName pantaskState PROG_ERR
    return
  end

  # failure related to pantasks
  if ($exitCode == $EXIT_CRASH_ERR)
    # stop
    showcommand "crash error"
    book setword $bookName $pageName pantaskState CRASH_ERR
    return
  end

  # any other exit status
  showcommand failure
  book setword $bookName $pageName pantaskState UNKNOWN_ERR
end

## XXX for the moment, remove all errors
macro process_cleanup -c "standard handler to cleanup the books"
  if ($0 != 2)
    echo "USAGE: process_cleanup (bookname)"
    break
  end

  book delpage $1 -key pantaskState DONE

  if (not($KEEP_FAILURES))
    book delpage $1 -key pantaskState SYS_ERR
    book delpage $1 -key pantaskState DATA_ERR
    book delpage $1 -key pantaskState PROG_ERR
    book delpage $1 -key pantaskState CONFIG_ERR
    book delpage $1 -key pantaskState UNKNOWN_ERR
    book delpage $1 -key pantaskState CRASH_ERR
    book delpage $1 -key pantaskState CRASH
    book delpage $1 -key pantaskState TIMEOUT
  end
end

macro set.poll -c "set the POLL_LIMIT value"
  if ($0 != 2)
    echo "USAGE:set.poll (value)"
    break
  end
 
  $POLL_LIMIT = $1
end

macro get.poll -c "get the POLL_LIMIT value"
  echo "poll limit: $POLL_LIMIT"
end

macro set.keep.failures
  if ($0 != 2)
    echo "USAGE:set.keep.failures (value)"
    break
  end

  $KEEP_FAILURES = $1
end

macro get.keep.failures
  echo "KEEP_FAILURES: $KEEP_FAILURES"
end

macro change_polllimit -c "set the POLL_LIMIT value (same as set.poll)"
    $POLL_LIMIT = $1
end

macro change_runexec
    $RUNEXEC = $1
end

macro change_runpoll
    $RUNPOLL = $1
end

macro print_polllimit
    echo $POLL_LIMIT
end

macro print_runexec
    echo $RUNEXEC
end

macro print_runpoll
    echo $RUNPOLL
end

macro change_loadexec
    $LOADEXEC = $1
end

macro change_loadpoll
    $LOADPOLL = $1
end

macro print_loadexec
    echo $LOADEXEC
end

macro print_loadpoll
    echo $LOADPOLL
end

macro set.verbosity
  if ($0 != 2)
    echo "USAGE: set.verbosity (level)"
    break
  end
  $VERBOSE = $1
end

macro get.verbosity
  echo "verbosity: $VERBOSE"
end

macro show.variables
  ??
end

macro set.workdir.by.camera
  if ($0 != 6)
    echo "USAGE: set.workdir.by.camera (camera) (class_id) (template) (default) (varname)"
    break
  end

  local host default template camera classID varname
  $camera = $1
  $classID = $2
  $template = $3
  $default = $4
  $varname = $5

  if ("$template" == "NULL")
    $$varname = $workdir_template
    echo "WARNING: WORKDIR template not set.  Defaulting to $workdir_template"
    return
  end

  # missing camera and/or ipphosts table results in host = NULL
  book getword ipphosts $camera $classID -var host

  if ("$host" == "NULL")
    # we are modifying something like /data/@HOST@/foo/bar, 
    # but some implementations have multiple entries for the same host,
    # like /data/@HOST@.0/foo/bar.  
    # try a couple of these first:
    strsub $template @HOST@.0 $default -var $varname
    strsub $$varname @HOST@.1 $default -var $varname
    strsub $$varname @HOST@   $default -var $varname
  else
    strsub $template @HOST@ $host -var $varname
  end

    strsub $$varname .0.0/ .0/ -var $varname
    strsub $$varname .1.0/ .1/ -var $varname
    strsub $$varname .2.0/ .2/ -var $varname
end

macro set.workdir.by.skycell
  if ($0 != 5)
    echo "USAGE: set.workdir.by.skycell (skycellID) (template) (default) (varname)"
    break
  end

  local host default template skycellID varname count
  $skycellID = $1
  $template = $2
  $default = $3
  $varname = $4

  if ("$template" == "NULL")
    $$varname = $workdir_template
    echo "WARNING: WORKDIR template not set.  Defaulting to $workdir_template"
    return
  end

  # get the folding count for this camera  
  book getword ipphosts skycell count -var count
  if ("$count" == "NULL")
    strsub $template @HOST@.0 $default -var $varname
    strsub $$varname @HOST@.1 $default -var $varname
    strsub $$varname @HOST@   $default -var $varname
    return
  end    

  local j myValue skyhash
  list word -splitbychar . $skycellID
  $skyhash = 0
  for j 0 $word:n
    inthash $word:$j $count -var myValue
    $skyhash = $skyhash + $myValue
  end
  inthash $skyhash $count -var skyhash
  sprintf skyname "sky%02d" $skyhash

  # missing ipphosts table results in host = NULL
  book getword ipphosts skycell $skyname -var host

  if ("$host" == "NULL")
    strsub $template @HOST@.0 $default -var $varname
    strsub $$varname @HOST@.1 $default -var $varname
    strsub $$varname @HOST@   $default -var $varname
  else
    # Replace .0 forms if we can, otherwise, use plain forms.
    strsub $template @HOST@.0 $host -var $varname
    strsub $$varname @HOST@   $host -var $varname
  end
end

macro set.host.for.camera
  if ($0 != 3)
    echo "USAGE: set.host.for.camera (camera) (class_id)"
    break
  end

  local host

  if (not($PARALLEL))
    host local
    return
  end

  # missing camera and/or ipphosts table results in host = NULL
  book getword ipphosts $1 $2 -var host

  if ("$host" == "NULL")
    host anyhost
  else
    # echo host $host
    host $host
  end
end

macro set.host.for.skycell
  if ($0 != 2)
    echo "USAGE: set.host.for.skycell (skycellID)"
    break
  end

  local skycellID varname count host skyname skyhash fullhost
  $skycellID = $1

  # get the folding count for this camera  
  book getword ipphosts skycell count -var count
  if ("$count" == "NULL")
    host anyhost
    return
  end    

  local j myValue skyhash
  list word -splitbychar . $skycellID
  $skyhash = 0
  for j 0 $word:n
    inthash $word:$j $count -var myValue
    $skyhash = $skyhash + $myValue
  end
  inthash $skyhash $count -var skyhash
  sprintf skyname "sky%02d" $skyhash

  # missing ipphosts table results in host = NULL
  book getword ipphosts skycell $skyname -var fullhost
  list word -splitbychar . $fullhost
  $host = $word:0

  if ("$host" == "NULL")
    host anyhost
  else
    # echo host $host
    host $host
  end
end

macro get.host.for.camera
  if ($0 != 3)
    echo "USAGE: get.host.for.camera (camera) (class_id)"
    break
  end

  if (not($PARALLEL))
    echo local
    return
  end

  # missing camera and/or ipphosts table results in host = NULL
  book getword ipphosts $1 $2 -var host

  if ("$host" == "NULL")
    echo anyhost
  else
    echo $host
  end
end

macro check.globals
  if ($?NETWORK == 0)
    echo "NETWORK not defined: load pantasks.pro first"
    break
  end
  if ($?PARALLEL == 0)
    echo "PARALLEL not defined: load pantasks.pro first"
    break
  end
  if ($?VERBOSE == 0)
    echo "VERBOSE not defined: load pantasks.pro first"
    break
  end
  if ($?LOGDIR == 0)
    echo "LOGDIR not defined: load pantasks.pro first"
    break
  end
  if ($?KEEP_FAILURES == 0)
    echo "KEEP_FAILURES not defined: load pantasks.pro first"
    break
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

macro show.books
  book list
end

macro del.page.from.book
  if ($0 != 3)
   echo "USAGE: del.page.from.book (book) (page)"
   break
  end

  book delpage $1 $2
end

macro set.opihi.verbose
  if ($0 != 2)
    echo "USAGE: set.opihi.verbose (on/off)"
    break
  end

  opihi verbose $1
end

# identical to 'set.opihi.verbose'
macro opihi.verbosity
  opihi verbose $1
end

macro set.server.verbose
  if ($0 != 2)
    echo "USAGE: set.server.verbose (on/off)"
    break
  end

  verbose $1
end

macro set.output
  if ($0 != 2)
   echo "USAGE: set.output (destination)"
   break
  end
  output $1  
end

macro show.output
  output -current currentOutput
  echo $currentOutput
end


