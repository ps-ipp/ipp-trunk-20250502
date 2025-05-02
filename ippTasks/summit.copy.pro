## summit.copy.pro : tasks for the summit to IPP download : -*- sh -*-
## PanTasks scripts for Summit Copy

## XXX note that this currently works with a single database as defined in .ipprc
## XXX tie the database for output to the database from which the datastore was determined

# pztool -adddatastore -inst isp -telescope ps1 -uri http://otis1.ifa.hawaii.edu/ds/skyprobe/index.txt
# pztool -adddatastore -inst ssp -telescope ps1 -uri http://otis1.ifa.hawaii.edu/ds/ssp/index.txt                                                                                        
# pztool -adddatastore -inst gpc1 -telescope ps1 -uri http://conductor/ds/gpc1/index.txt
# pztool -adddatastore -inst allskycam -telescope ps1 -uri  http://otis1.ifa.hawaii.edu/ds/allskycam/index.txt


# NOTE: workdir / volume mangling and nebulous.  these tasks copy the
# imfiles from the summit, placing the output files in a directory
# which is based on the chip/host relationship.  If nebulous is being
# used, the volume name is set based on the host; if nebulous is not
# being used, the workdir is set to include the host name.  The copy
# operation is targetted to the same host by pcontrol.  The value of
# workdir is set to be a template into which the appropriate value of
# @HOST@ may be substituted in later scripts.

# test for required global variables
check.globals

# list of DataStores to pull data from
# book init pzDataStore
# list of summit exps that need to be queried
book init pzPendingExp
# list of summit imfiles that need to be downloaded
book init pzPendingImfile
# list of pzDownloadExps that have completed downloading
book init pzPendingAdvance

macro copy.status
    book listbook pzPendingImfile
end

macro copy.reset
    book init pzPendingExp
    book init pzPendingImfile
    book init pzPendingAdvance
end

# aliases because copy.* isn't intuitive
macro pztool.reset
    copy.reset
end

macro summitcopy.reset
    summitcopy.reset
end

# pzgetexp is loading exposure files to summitExp 
macro pzgetexp.on
  task pzgetexp
    active true
  end
end

macro pzgetexp.off
  task pzgetexp
    active false
  end
end

macro copy.on
  task pztool.datastore
    active true
  end
  task pzgetexp
    active true
  end
  task pztool.pendingexp
    active true
  end
  task pzgetimfile 
    active true
  end
  task pztool.pendingimfile
    active true
  end
  task summit_copy
    active true
  end
  task pztool.clearfault
      active true
  end
  task summit.toadvance
      active true
  end
  task summit.advance
      active true
  end
end

macro copy.off
  task pztool.datastore
    active false
  end
  task pzgetexp
    active false
  end
  task pztool.pendingexp
    active false
  end
  task pzgetimfile 
    active false
  end
  task pztool.pendingimfile
    active false
  end
  task summit_copy
    active false
  end
  task pztool.clearfault
      active false
  end
  task summit.toadvance
      active false
  end
  task summit.advance
      active false
  end
end

# these variables will cycle through the known database names
$pztoolDatastore_DB = 0
$pztoolPendingExp_DB = 0
$pztoolPendingImfile_DB = 0
$pztoolPendingAdvance_DB = 0
$pztoolClearFault_DB = 0
$pztoolAdvance_DB = 0;

# build a book of datastores to poll for data
task pztool.datastore
    host         local

    # timeout shorter than exec so jobs do not build up
    periods      -exec       1
    periods      -poll       1
    periods      -timeout   20
    npending     1

    # only active in the day (06:00 to 19:00 HST, times are UT):
    trange        -reset
    # trange        16:00 23:59
    # trange        00:00 05:00

    task.exec
#      echo "DB_DEBUG: PRE" $pztoolDatastore_DB $DB:$pztoolDatastore_DB
      if ($DB:n == 0)
        option DEFAULT
        command pztool -datastore
      else
        # save the DB name for the exit tasks
        option $DB:$pztoolDatastore_DB
        command pztool -datastore -dbname $DB:$pztoolDatastore_DB
        $pztoolDatastore_DB ++
        if ($pztoolDatastore_DB >= $DB:n) set pztoolDatastore_DB = 0
      end

      # More debug
#      echo "DB_DEBUG: POST" $pztoolDatastore_DB $DB:$pztoolDatastore_DB
      periods -exec 120
    end

    # success
    task.exit 0
        # flush pzDataStore book
        book init pzDataStore
        # convert 'stdout' to book format
        ipptool2book stdout pzDataStore -key camera:telescope -uniq -setword dbname $options:0
    end

    task.exit default
        showcommand failure
    end
    task.exit crash
        showcommand crash
    end
    task.exit timeout
        showcommand timeout
    end
end

$datastore_index = 0

# run pzgetexp periodically to populate summitExp in the database (no I/O) 
# this task is querying the data store for a list of exposures ("filesets")
# and inserting these into a db table on the local cluster (pzDownloadExp)
task pzgetexp
  host         local

  periods      -exec     30
  periods      -poll     1
  periods      -timeout  700
  npending      1

  # only active in the day (06:00 to 19:00 HST, times are UT):
  trange        -reset
  # trange        16:00 23:59
  # trange        00:00 05:00

  task.exec
        # find an exp that needs imfiles fetched
        book getpage pzDataStore $datastore_index -var pageName
        if ("$pageName" == "NULL") break

        # increment our pzDataStore index and loop back to 0 and the end of the
        # book
        $datastore_index ++
        book npages pzDataStore -var npages
        if ($datastore_index == $npages )
            $datastore_index = 0
        end

        book getword pzDataStore $pageName camera    -var CAMERA
        book getword pzDataStore $pageName telescope -var TELESCOPE
        book getword pzDataStore $pageName uri       -var URI
        book getword pzDataStore $pageName dbname    -var DBNAME

        # store the current page
        options $pageName

        $run = pzgetexp -uri $URI -inst $CAMERA -telescope $TELESCOPE -dbname $DBNAME -timeout 650

        # create the command line
        if ($VERBOSE > 1)
          echo command $run
        end

        command $run
  end

  task.exit     0
  end

  task.exit     default
    showcommand failure
  end
  task.exit     crash
    showcommand crash
  end
  task.exit     timeout
    showcommand timeout
  end
end

# queries summitExp to generate a list of exposures to download, 
# builds a book (pzPendingExp) of exps/filesetids that need to be queried
task pztool.pendingexp
    host         local

    periods      -exec     10
    periods      -poll     1
    periods      -timeout  60
    npending     1

    # only active in the day (06:00 to 19:00 HST, times are UT):
    trange        -reset
    # trange        16:00 23:59
    # trange        00:00 05:00

    task.exec

    # CZW 2016-04-08 This needs to be set to the date that pztool should start considering.  
    #                I've chosen to use date's "relative items in date strings" calculation
    #                because it's easy to set a time frame.
      $pztool_date_begin = `date -d "-30 days" +%Y-%m-%dT00:00:00`

      if ($DB:n == 0)
        option DEFAULT
        command pztool -pendingexp -limit 5
      else
        # save the DB name for the exit tasks
        option $DB:$pztoolPendingExp_DB
        command pztool -pendingexp -dateobs_begin $pztool_date_begin -limit 10 -dbname $DB:$pztoolPendingExp_DB
        $pztoolPendingExp_DB ++
        if ($pztoolPendingExp_DB >= $DB:n) set pztoolPendingExp_DB = 0
      end
    end

    # success
    task.exit 0
        # convert 'stdout' to book format
        ipptool2book stdout pzPendingExp -key exp_name:camera:telescope -uniq -setword dbname $options:0 -setword pantaskState INIT

        # delete existing entries in the appropriate pantaskStates
        process_cleanup pzPendingExp
    end

    task.exit default
        showcommand failure
        if ($VERBOSE)
          echo "*** stdout ***"
          queueprint stdout
          echo "*** stderr ***"
          queueprint stderr
        end
    end
    task.exit crash
        showcommand crash
    end
    task.exit timeout
        showcommand timeout
    end
end

# run pzgetimfiles on pending exps.  analogous to pzgetexp, this task
# is downloading a list of imfiles reported by the datastore for a
# given exposure ("fileset"), and placing the result in the local
# database table of imfiles
task pzgetimfile 
    host 	local

    periods      -exec     0.05
    periods      -poll     0.025
    periods      -timeout  700
    npending 	10

    # only active in the day (06:00 to 19:00 HST, times are UT):
    trange        -reset
    # trange        16:00 23:59
    # trange        00:00 05:00

    task.exec
        periods -exec 20

        # if we are waiting on data, make the interval long
        book npages pzPendingExp -var N
        if ($N == 0) break
        if ($NETWORK == 0) break

        # find an exp that needs imfiles fetched
        book getpage pzPendingExp 0 -var pageName -key pantaskState INIT
        if ("$pageName" == "NULL") break

        # set that exp to run
        book setword pzPendingExp $pageName pantaskState RUN

        book getword pzPendingExp $pageName exp_name  -var EXP_NAME
        book getword pzPendingExp $pageName camera    -var CAMERA
        book getword pzPendingExp $pageName telescope -var TELESCOPE
        book getword pzPendingExp $pageName dateobs   -var DATEOBS
        book getword pzPendingExp $pageName exp_type  -var EXP_TYPE
        book getword pzPendingExp $pageName uri       -var URI
        book getword pzPendingExp $pageName imfiles   -var IMFILES
        book getword pzPendingExp $pageName dbname    -var DBNAME

        # store the current page
        options $pageName

        $batman = $EXP_NAME
        # Sidik says we should use a longer timeout
        $run = pzgetimfiles -uri $URI -filesetid $batman -inst $CAMERA -telescope $TELESCOPE -dbname $DBNAME -timeout 650

        # create the command line
        if ($VERBOSE > 1)
          echo command $run
        end
        periods -exec 0.05
        command $run
    end

    # success
    task.exit 0
        process_exit pzPendingExp $options:0 $JOB_STATUS
    end

    task.exit default
        showcommand failure
        process_exit pzPendingExp $options:0 $JOB_STATUS
    end

    task.exit crash
        showcommand crash
        book setword pzPendingExp $options:0 pantaskState CRASH
    end

    task.exit timeout
        showcommand timeout
        book setword pzPendingExp $options:0 pantaskState TIMEOUT
    end
end


# build a book of imfiles/files that need to be downloaded
task pztool.pendingimfile
    host         local

    periods      -exec     10
    periods      -poll      1
    periods      -timeout  120
    npending     1

    # only active in the day (06:00 to 19:00 HST, times are UT):
    trange        -reset
    # trange        16:00 23:59
    # trange        00:00 05:00

    # select entries from the current DB; cycle to the next DB, if it exists
    # iff the DB list is not set, use the value defined in .ipprc
    task.exec
      if ($DB:n == 0)
        option DEFAULT
        command pztool -pendingimfile -limit 40
      else
        # save the DB name for the exit tasks
        option $DB:$pztoolPendingImfile_DB
        command pztool -pendingimfile -limit 240 -dbname $DB:$pztoolPendingImfile_DB
        $pztoolPendingImfile_DB ++
        if ($pztoolPendingImfile_DB >= $DB:n) set pztoolPendingImfile_DB = 0
      end
    end
  
    # success
    task.exit    0
        # convert 'stdout' to book format
        ipptool2book stdout pzPendingImfile -key exp_name:camera:telescope:class:class_id -uniq -setword dbname $options:0 -setword pantaskState INIT
	book shuffle pzPendingImfile 

        # delete existing entries in the appropriate pantaskStates
        process_cleanup pzPendingImfile
    end

    task.exit     default
        showcommand failure
    end
    task.exit     crash
        showcommand crash
    end
    task.exit     timeout
        showcommand timeout
    end
end

# retreive an imfile with dsget and then call pztool -copydone
task summit_copy
    periods      -exec     5
    periods      -poll     0.05
    periods      -timeout  1150

    # only active in the day (06:00 to 19:00 HST, times are UT):
    trange        -reset
    # trange        16:00 23:59
    # trange        00:00 05:00

    task.exec
        periods -exec 20

        # if we are waiting on data, make the interval long
        book npages pzPendingImfile -var N
        if ($N == 0) break
        if ($NETWORK == 0) break

        # find an exp that needs imfiles fetched
        book getpage pzPendingImfile 0 -var pageName -key pantaskState INIT
        if ("$pageName" == "NULL") break

        # set that exp to run
        book setword pzPendingImfile $pageName pantaskState RUN

        book getword pzPendingImfile $pageName uri     	 -var URI
        book getword pzPendingImfile $pageName bytes   	 -var BYTES
        book getword pzPendingImfile $pageName md5sum  	 -var MD5SUM
        book getword pzPendingImfile $pageName dateobs 	 -var DATEOBS
	book getword pzPendingImfile $pageName summit_id -var SUMMIT_ID
        book getword pzPendingImfile $pageName exp_name  -var EXP_NAME
        book getword pzPendingImfile $pageName camera    -var CAMERA
        book getword pzPendingImfile $pageName telescope -var TELESCOPE
        book getword pzPendingImfile $pageName class     -var CLASS
        book getword pzPendingImfile $pageName class_id  -var CLASS_ID
        book getword pzPendingImfile $pageName dbname    -var DBNAME

        set.host.for.camera $CAMERA $CLASS_ID

        # 2007-08-30T05:09:59Z
        substr $DATEOBS 0 4 YEAR
        substr $DATEOBS 5 2 MONTH
        substr $DATEOBS 8 2 DAY

        # we need to set the workdir based on 1) nebulous or not? 2) chip/host relationship
        # this function uses workdir_template, default_host, volume_template, volume_default,
        # it sets workdir and volume

        # Look to see if we have a stare type observation, and redirect it to an appropriate host
	substr $EXP_NAME 10 1 EXPTYPE_KEY
	if ("$EXPTYPE_KEY" == "a") 
	  if ($NEBULOUS == 1) 
	    $workdir_template = neb://@HOST@.1
	  end
	  set.workdir.by.camera STARE $CLASS_ID $workdir_template $default_host workdir_base
        else 
	  if ($NEBULOUS == 1)
	    $workdir_template = neb://@HOST@.0
	  end
          set.workdir.by.camera $CAMERA $CLASS_ID $workdir_template $default_host workdir_base
        end

        # figure out filename
	# XXX may need to use sprintf here
        $FILENAME = $workdir_base/$CAMERA/$YEAR\$MONTH\$DAY/$EXP_NAME/$EXP_NAME.$CLASS_ID.fits
        $workdir = $workdir_template/$CAMERA/$YEAR\$MONTH\$DAY

	# workdir examples:
	# file://data/@HOST@.0/gpc1/20080130
	# neb://@HOST@.0/gpc1/20080130

	# filename examples:
	# file://data/ipp005.0/gpc1/20080130/o4437g0025d/o4437g0025d.XY05.fits
	# neb://@HOST@.0/gpc1/20080130/o4437g0025d/o4437g0025d.XY05.fits

        book setword pzPendingImfile $pageName filename $FILENAME

	stdout $LOGDIR/summit.copy.log
	stderr $LOGDIR/summit.copy.log

	book getpage pzDataStore 0 -var PZDSPAGE -key dbname $DBNAME 
	book getword pzDataStore $PZDSPAGE use_compress  -var USECOMPRESS

	# Debug line
#	echo "DEBUG: " $DBNAME $EXP_NAME $CAMERA $PZDSPAGE $USECOMPRESS

        # unconditionally turn on requesting compression until we figuure
        # out why the above doesn't work
	if ($USECOMPRESS == "NULL") 
#	if ("$CAMERA" == "gpc1") 
	    $USECOMPRESS = 1
	end

        $run = summit_copy.pl --uri $URI --filename $FILENAME --summit_id $SUMMIT_ID --exp_name $EXP_NAME --inst $CAMERA --telescope $TELESCOPE --class $CLASS --class_id $CLASS_ID --bytes $BYTES --md5 $MD5SUM --dbname $DBNAME --timeout 600 --verbose --copies 2
	if ($USECOMPRESS == 1) 
            $run = $run --compress
        end
#        if (("$MD5SUM" != "NULL") && ("$MD5SUM" != "0") && (not($COMPRESS)))
# && (($YEAR > 2008) || (("$YEAR" = "2007") && ($MONTH > 8))))
#            $run = $run --md5 $MD5SUM
#        end
	if ($NEBULOUS) 
            $run = $run --nebulous
        end
        # add_standard_args run

        # store the pageName for future reference below
        options $pageName
	
        # create the command line
        if ($VERBOSE > 1)
          echo command $run
        end
	# More debug:
#	echo "DEBUG2: " $run
        periods -exec 0.05
        command $run
    end

    # default exit status
    task.exit default
        process_exit pzPendingImfile $options:0 $JOB_STATUS
    end

    task.exit crash
        showcommand crash
        book setword pzPendingImfile $options:0 pantaskState CRASH
    end 

    # operation timed out?
    task.exit timeout
        showcommand timeout
        book setword pzPendingImfile $options:0 pantaskState TIMEOUT
    end 
end

task pztool.clearfault
    host         local

    # -exec is set much longer the first time this task runs
    periods      -exec       7  
    periods      -poll       1
    periods      -timeout   30
    npending     1

    task.exec
      if ($DB:n == 0)
        command pztool -clearcommonfaults
      else
        command pztool -clearcommonfaults -dbname $DB:$pztoolClearFault_DB

        # loop over the multiple databases quickly, then pause for 5 min
        $pztoolClearFault_DB ++
        if ($pztoolClearFault_DB >= $DB:n) 
          set pztoolClearFault_DB = 0
          periods -exec 300
        else
          periods -exec 30
        end
      end
    end

    # success
    task.exit 0
    end

    task.exit default
        showcommand failure
    end
    task.exit crash
        showcommand crash
    end
    task.exit timeout
        showcommand timeout
    end
end

# build a book of exposures that have completed downloading
task summit.toadvance
    host         local

    periods      -exec     30
    periods      -poll      1
    periods      -timeout  120
    npending     1

    # select entries from the current DB; cycle to the next DB, if it exists
    # iff the DB list is not set, use the value defined in .ipprc
    task.exec
      if ($DB:n == 0)
        option DEFAULT
        command pztool -toadvance -limit 40
      else
        # save the DB name for the exit tasks
        option $DB:$pztoolPendingAdvance_DB
        command pztool -toadvance -limit 60 -dbname $DB:$pztoolPendingAdvance_DB
        $pztoolPendingAdvance_DB ++
        if ($pztoolPendingAdvance_DB >= $DB:n) set pztoolPendingAdvance_DB = 0
      end
    end
  
    # success
    task.exit    0
        # convert 'stdout' to book format
        ipptool2book stdout pzPendingAdvance -key exp_name:camera:telescope -uniq -setword dbname $options:0 -setword pantaskState INIT
	book shuffle pzPendingAdvance 

        # delete existing entries in the appropriate pantaskStates
        process_cleanup pzPendingAdvance
    end

    task.exit     default
        showcommand failure
    end
    task.exit     crash
        showcommand crash
    end
    task.exit     timeout
        showcommand timeout
    end
end

task summit.advance
    periods      -exec     5
    periods      -poll     0.05
    periods      -timeout  650

    task.exec
        periods -exec 20

        # if we are waiting on data, make the interval long
        book npages pzPendingAdvance -var N
        if ($N == 0) break
        if ($NETWORK == 0) break

        # find an exp that needs imfiles fetched
        book getpage pzPendingAdvance 0 -var pageName -key pantaskState INIT
        if ("$pageName" == "NULL") break

        # set that exp to run
        book setword pzPendingAdvance $pageName pantaskState RUN

	book getword pzPendingAdvance $pageName summit_id -var SUMMIT_ID
        book getword pzPendingAdvance $pageName exp_name  -var EXP_NAME
        book getword pzPendingAdvance $pageName camera    -var CAMERA
        book getword pzPendingAdvance $pageName telescope -var TELESCOPE
        book getword pzPendingAdvance $pageName dbname    -var DBNAME
        book getword pzPendingAdvance $pageName dateobs    -var DATEOBS

        # 2007-08-30T05:09:59Z
        substr $DATEOBS 0 4 YEAR
        substr $DATEOBS 5 2 MONTH
        substr $DATEOBS 8 2 DAY

        # we need to set the workdir based on 1) nebulous or not? 2) chip/host relationship
        # this function uses workdir_template, default_host, volume_template, volume_default,
        # it sets workdir and volume
       	set.workdir.by.camera $CAMERA $CLASS_ID $workdir_template $default_host workdir_base

        $workdir = $workdir_template/$CAMERA/$YEAR\$MONTH\$DAY

	# workdir examples:
	# file://data/@HOST@.0/gpc1/20080130
	# neb://@HOST@.0/gpc1/20080130

	stdout $LOGDIR/summit.advance.log
	stderr $LOGDIR/summit.advance.log

        $run = pztool -advance -summit_id $SUMMIT_ID -exp_name $EXP_NAME -inst $CAMERA -telescope $TELESCOPE -end_stage reg -workdir $workdir -dbname $DBNAME

        # store the pageName for future reference below
        options $pageName

        # create the command line
        if ($VERBOSE > 1)
          echo command $run
        end
        periods -exec 0.05
        command $run
    end

    # default exit status
    task.exit default
        process_exit pzPendingAdvance $options:0 $JOB_STATUS
    end

    task.exit crash
        showcommand crash
        book setword pzPendingAdvance $options:0 pantaskState CRASH
    end 

    # operation timed out?
    task.exit timeout
        showcommand timeout
        book setword pzPendingAdvance $options:0 pantaskState TIMEOUT
    end 
end
