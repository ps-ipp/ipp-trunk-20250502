## survey.pro : tasks related to automation of the PS1 survey : -*- sh -*-

# test for required global variables
check.globals

if (not($?haveSurveyBooks))
 book create SURVEY_DIFF
 book create SURVEY_DIFF_WARPSTACK
 book create SURVEY_DIFF_STACKSTACK
 book create SURVEY_MAGIC
 book create SURVEY_DESTREAK
 book create SURVEY_DIST
 book create SURVEY_ADDSTAR   
 book create SURVEY_MERGEDVODB
 book create SURVEY_CHIP_BG
 book create SURVEY_WARP_BG
 book create SURVEY_PUBLISH
 book create SURVEY_STATICSKYSINGLE 
 book create SURVEY_SKYCAL
 book create SURVEY_LAPGROUP
 book create SURVEY_RELEXP
 book create SURVEY_RELSTACK
 $haveSurveyBooks = TRUE
end

$SURVEY_DIFF_DB = 0
$SURVEY_DIFF_WARPSTACK_DB = 0
$SURVEY_DIFF_STACKSTACK_DB = 0
$SURVEY_MAGIC_DB = 0
$SURVEY_DESTREAK_DB = 0
$SURVEY_DIST_DB = 0
$SURVEY_ADDSTAR_DB = 0
$SURVEY_MERGEDVODB_DB = 0
$SURVEY_CHIP_BG_DB = 0
$SURVEY_WARP_BG_DB = 0
$SURVEY_PUBLISH_DB = 0
$SURVEY_SKYCAL_DB = 0
$SURVEY_LAPGROUP_DB = 0
$SURVEY_RELEXP_DB = 0
$SURVEY_RELSTACK_DB = 0
$SURVEY_STATICSKYSINGLE_DB = 0
$SURVEY_EXEC = 120
$SURVEY_POLL = 10
$SURVEY_TIMEOUT = 600
# the tasks for loading the release tabels for nightly science do not need to run very often
$SURVEY_RELEASE_EXEC = 300

macro survey.on
  task survey.diff
    active true
  end
  task survey.warpstack.diff
    active true
  end
  task survey.stackstack.diff
    active true
  end
  task survey.magic
    active true
  end
  task survey.addstar
    active true
  end
  task survey.mergedvodb
    active true
  end
  task survey.destreak
    active true
  end
  task survey.dist
    active true
  end
  task survey.chip.bg
    active true
  end
  task survey.warp.bg
    active true
  end
  task survey.publish
    active true
  end
  task survey.staticskysingle
    active true
  end  
  task survey.skycal
    active true
  end
  task survey.lapgroup
    active true
  end
  task survey.relexp
    active true
  end
  task survey.relstack
    active true
  end
end

macro survey.off
  task survey.diff
    active false
  end
  task survey.warpstack.diff
    active false
  end
  task survey.stackstack.diff
    active false
  end
  task survey.magic
    active false
  end
  task survey.addstar
    active false
  end
  task survey.mergedvodb
    active false
  end  
  task survey.destreak
    active false
  end
  task survey.dist
    active false
  end
  task survey.chip.bg
    active false
  end
  task survey.warp.bg
    active false
  end
  task survey.publish
    active false
  end
  task survey.staticskysingle
    active false
  end
  task survey.skycal
    active false
  end
  task survey.lapgroup
    active false
  end
  task survey.relexp
    active false
  end
  task survey.relstack
    active false
  end
end

macro survey.stage
  if ($0 != 3)
    echo "USAGE: survey.stage (stage) (mode)"
    echo " turn the specified survey stage on or off"
    echo " stage = diff, warpstack.diff, stackstack.diff, magic, destreak, dist, addstar"
    echo " mode = on, off"
    break
  end

  local found stage mode

  $stage = $1
  $mode = $2

  $found = 0
  if ("$stage" == "diff") 
    $found = 1
  end
  if ("$stage" == "warpstack.diff") 
    $found = 1
  end
  if ("$stage" == "stackstack.diff") 
    $found = 1
  end
  if ("$stage" == "magic") 
    $found = 1
  end
  if ("$stage" == "destreak") 
    $found = 1
  end
  if ("$stage" == "dist") 
    $found = 1
  end
  if ("$stage" == "addstar") 
    $found = 1
  end
  if ($found == 0) 
    echo "unknown stage $stage"
    break
  end

  if (("$mode" != "on") && ("$mode" != "off"))
    echo "unknown mode $mode"
    break
  end

  if ("$mode" == "on")
    task survey.$stage
      active true
    end
  end

  if ("$mode" == "off")
    task survey.$stage
      active false
    end
  end
end

macro survey.publish.status
  if ($0 != 2) 
    echo "USAGE: survey.publish.status (true/false)"
    break
  end

  task survey.publish
    active $1
  end
end

# user functions to manipulate diff labels
macro survey.add.diff
  if ($0 != 4)
    echo "USAGE: survey.add.diff (label) (dist_group) (workdir base)"
    break
  end
  book newpage SURVEY_DIFF $1
  book setword SURVEY_DIFF $1 DIST_GROUP $2
  book setword SURVEY_DIFF $1 WORKDIR $3
  book setword SURVEY_DIFF $1 STATE PENDING
end

macro survey.del.diff
  if ($0 != 2)
    echo "USAGE: survey.del.diff (label)"
    break
  end
  book delpage SURVEY_DIFF $1
end

macro survey.show.diff
  if ($0 != 1)
    echo "USAGE: survey.show.diff"
    break
  end
  book listbook SURVEY_DIFF
end

# user functions to manipulate warp/stack diff labels
macro survey.add.WSdiff
  if ($0 != 7)
    echo "USAGE: survey.add.WSdiff (key) (warp label) (stack label) (dist_group) (workdir base) (target label)"
    break
  end
  book newpage SURVEY_DIFF_WARPSTACK $1
  book setword SURVEY_DIFF_WARPSTACK $1 WARP_LABEL $2
  book setword SURVEY_DIFF_WARPSTACK $1 STACK_LABEL $3
  book setword SURVEY_DIFF_WARPSTACK $1 DIST_GROUP $4
  book setword SURVEY_DIFF_WARPSTACK $1 WORKDIR $5
  book setword SURVEY_DIFF_WARPSTACK $1 TARGET_LABEL $6
  book setword SURVEY_DIFF_WARPSTACK $1 STATE PENDING
end

macro survey.del.WSdiff
  if ($0 != 2)
    echo "USAGE: survey.del.WSdiff (warp label)"
    break
  end
  book delpage SURVEY_DIFF_WARPSTACK $1
end

macro survey.show.WSdiff
  if ($0 != 1) 
    echo "USAGE: survey.show.WSdiff"
    break
  end
  book listbook SURVEY_DIFF_WARPSTACK
end

# user functions to manipulate stack/stack diff labels
macro survey.add.SSdiff
  if ($0 != 7)
    echo "USAGE: survey.add.SSdiff (key) (input label) (stack label) (dist_group) (workdir base) (target label)"
    break
  end
  book newpage SURVEY_DIFF_STACKSTACK $1
  book setword SURVEY_DIFF_STACKSTACK $1 INPUT_LABEL $2 
  book setword SURVEY_DIFF_STACKSTACK $1 STACK_LABEL $3
  book setword SURVEY_DIFF_STACKSTACK $1 DIST_GROUP $4
  book setword SURVEY_DIFF_STACKSTACK $1 WORKDIR $5
  book setword SURVEY_DIFF_STACKSTACK $1 TARGET_LABEL $6
  book setword SURVEY_DIFF_STACKSTACK $1 STATE PENDING
end

macro survey.del.SSdiff
  if ($0 != 2)
    echo "USAGE: survey.del.SSdiff (stack label)"
    break
  end
  book delpage SURVEY_DIFF_STACKSTACK $1
end

macro survey.show.SSdiff
  if ($0 != 1) 
    echo "USAGE: survey.show.SSdiff"
    break
  end
  book listbook SURVEY_DIFF_STACKSTACK
end

# user functions to manipulate magic labels
macro survey.add.magic
  if (($0 != 3)&&($0 != 4))
    echo "USAGE: survey.add.magic (label) (workdir base) (multidiff)"
    break
  end
  book newpage SURVEY_MAGIC $1
  book setword SURVEY_MAGIC $1 WORKDIR $2
  if ($0 == 3)
     book setword SURVEY_MAGIC $1 MULTIDIFF FALSE
  else 
     book setword SURVEY_MAGIC $1 MULTIDIFF $3
  end
  book setword SURVEY_MAGIC $1 STATE PENDING
end

macro survey.del.magic
  if ($0 != 2)
    echo "USAGE: survey.del.magic (label)"
    break
  end
  book delpage SURVEY_MAGIC $1
end

macro survey.show.magic
  if ($0 != 1)
    echo "USAGE: survey.show.magic"
    break
  end
  book listbook SURVEY_MAGIC
end

macro survey.add.addstar
  if ($0 != 6)
    echo "USAGE: survey.add.addstar (tag) (label) (dvodb) (minidvodb_group) (stage)"
    break
  end
  book newpage SURVEY_ADDSTAR $1
  book setword SURVEY_ADDSTAR $1 LABEL $2
  book setword SURVEY_ADDSTAR $1 DVODB $3
  book setword SURVEY_ADDSTAR $1 MINIDVODB_GROUP $4
  book setword SURVEY_ADDSTAR $1 STAGE $5  
  book setword SURVEY_ADDSTAR $1 STATE PENDING
end

macro survey.del.addstar
  if ($0 != 2)
    echo "USAGE: survey.del.addstar (tag)"
    break
  end
  book delpage SURVEY_ADDSTAR $1
end

macro survey.show.addstar
  if ($0 != 1)
    echo "USAGE: survey.show.addstar"
    break
  end
  book listbook SURVEY_ADDSTAR
end

macro survey.add.mergedvodb
  if ($0 != 4)
    echo "USAGE: survey.add.mergedvodb (tag) (mergedvodb) (minidvodb_group)"
    break
  end
  book newpage SURVEY_MERGEDVODB $1
  book setword SURVEY_MERGEDVODB $1 MERGEDVODB $2
  book setword SURVEY_MERGEDVODB $1 MINIDVODB_GROUP $3
end

macro survey.del.mergedvodb
  if ($0 != 2)
    echo "USAGE: survey.del.mergedvodb (tag)"
    break
  end
  book delpage SURVEY_MERGEDVODB $1
end

macro survey.show.mergedvodb
  if ($0 != 1)
    echo "USAGE: survey.show.mergedvodb"
    break
  end
  book listbook SURVEY_MERGEDVODB
end


# user functions to manipulate destreak labels
macro survey.add.destreak
  if ($0 != 4)
    echo "USAGE: survey.add.destreak (label) (workdir base) (recovery root)"
    break
  end
  book newpage SURVEY_DESTREAK $1
  book setword SURVEY_DESTREAK $1 WORKDIR $2
  book setword SURVEY_DESTREAK $1 RECOVERYROOT $3
  book setword SURVEY_DESTREAK $1 STATE PENDING
end

macro survey.del.destreak
  if ($0 != 2)
    echo "USAGE: survey.del.destreak (label)"
    break
  end
  book delpage SURVEY_DESTREAK $1
end

macro survey.show.destreak
  if ($0 != 1)
    echo "USAGE: survey.show.destreak"
    break
  end
  book listbook SURVEY_DESTREAK
end

# user functions to manipulate dist labels
macro survey.add.dist
  if ($0 != 4)
    echo "USAGE: survey.add.dist (label) (workdir) (muggle)"
    break
  end
  book newpage SURVEY_DIST $1
  book setword SURVEY_DIST $1 WORKDIR $2
  book setword SURVEY_DIST $1 MUGGLE $3
  book setword SURVEY_DIST $1 STATE PENDING
end

macro survey.del.dist
  if ($0 != 2)
    echo "USAGE: survey.del.dist (label)"
    break
  end
  book delpage SURVEY_DIST $1
end

macro survey.show.dist
  if ($0 != 1)
    echo "USAGE: survey.show.dist"
    break
  end
  book listbook SURVEY_DIST
end

# user functions to manipulate chip_bg labels
macro survey.add.chip.bg
  if ($0 != 4)
    echo "USAGE: survey.add.chip.bg (label) (cam_label) (dist_group)"
    break
  end
  book newpage SURVEY_CHIP_BG $1
  book setword SURVEY_CHIP_BG $1 LABEL $1
  book setword SURVEY_CHIP_BG $1 CAM_LABEL $2
  book setword SURVEY_CHIP_BG $1 DIST_GROUP $3
  book setword SURVEY_CHIP_BG $1 STATE PENDING
end

macro survey.del.chip.bg
  if ($0 != 2)
    echo "USAGE: survey.del.chip.bg (label)"
    break
  end
  book delpage SURVEY_CHIP_BG $1
end

macro survey.show.chip.bg
  if ($0 != 1)
    echo "USAGE: survey.show.chip.bg"
    break
  end
  book listbook SURVEY_CHIP_BG
end

# user functions to manipulate warp_bg labels
macro survey.add.warp.bg
  if ($0 != 4)
    echo "USAGE: survey.add.warp.bg (label) (warp_label) (dist_group)"
    break
  end
  book newpage SURVEY_WARP_BG $1
  book setword SURVEY_WARP_BG $1 LABEL $1
  book setword SURVEY_WARP_BG $1 WARP_LABEL $2
  book setword SURVEY_WARP_BG $1 DIST_GROUP $3
  book setword SURVEY_WARP_BG $1 STATE PENDING
end

macro survey.del.warp.bg
  if ($0 != 2)
    echo "USAGE: survey.del.warp.bg (label)"
    break
  end
  book delpage SURVEY_WARP_BG $1
end

macro survey.show.warp.bg
  if ($0 != 1)
    echo "USAGE: survey.show.warp.bg"
    break
  end
  book listbook SURVEY_WARP_BG
end

# user functions to manipulate publish labels
macro survey.add.publish
  if ($0 != 5)
    echo "USAGE: survey.add.publish (tag) (label) (client_id) (comment)"
    break
  end
  book newpage SURVEY_PUBLISH $1
  book setword SURVEY_PUBLISH $1 LABEL $2
  book setword SURVEY_PUBLISH $1 CLIENT_ID $3
  book setword SURVEY_PUBLISH $1 COMMENT $4
  book setword SURVEY_PUBLISH $1 STATE PENDING
end

macro survey.del.publish
  if ($0 != 2)
    echo "USAGE: survey.del.publish (label)"
    break
  end
  book delpage SURVEY_PUBLISH $1
end

macro survey.show.publish
  if ($0 != 1)
    echo "USAGE: survey.show.publish"
    break
  end
  book listbook SURVEY_PUBLISH
end

macro survey.add.staticskysingle
# adds each of the filters for the label chosen
# for LAP - the final stacks chosen have %final% in them, and we 
# want some ability to choose that
# for others can use %

 if ($0 != 7)
    echo "USAGE: survey.add.staticskysingle (tag) (label) (workdir) (distgroup)  (selectdatagroup) (filter)"
    break
 end
    book newpage SURVEY_STATICSKYSINGLE $1
    book setword SURVEY_STATICSKYSINGLE $1 LABEL $2
    book setword SURVEY_STATICSKYSINGLE $1 WORKDIR $3
    book setword SURVEY_STATICSKYSINGLE $1 DIST_GROUP $4
    book setword SURVEY_STATICSKYSINGLE $1 SELECTDATAGROUP $5
    book setword SURVEY_STATICSKYSINGLE $1 FILTER $6
    book setword SURVEY_STATICSKYSINGLE $1 STATE PENDING
end

macro survey.show.staticskysingle
 if ($0 != 1)
    echo "USAGE: survey.show.staticskysingle"
    break
 end
 book listbook SURVEY_STATICSKYSINGLE
end

macro survey.del.staticskysingle
  if ($0 != 2)
    echo "USAGE: survey.del.staticskysingle (tag)"
    break
  end
  book delpage SURVEY_STATICSKYSINGLE $1
end

macro survey.add.skycal
  if ($0 != 3)
    echo "USAGE: survey.add.skycal (label) (dist_group)"
    break
  end
  book newpage SURVEY_SKYCAL $1
  book setword SURVEY_SKYCAL $1 LABEL $1
  book setword SURVEY_SKYCAL $1 DIST_GROUP $2
  book setword SURVEY_SKYCAL $1 STATE PENDING
end

macro survey.del.skycal
  if ($0 != 2)
    echo "USAGE: survey.del.skycal (label)"
    break
  end
  book delpage SURVEY_SKYCAL $1
end

macro survey.show.skycal
  if ($0 != 1)
    echo "USAGE: survey.show.skycal"
    break
  end
  book listbook SURVEY_SKYCAL
end

macro survey.add.lapgroup
  if ($0 != 3)
    echo "USAGE: survey.add.lapgroup (label) (seq_id)"
    break
  end
  book newpage SURVEY_LAPGROUP $1
  book setword SURVEY_LAPGROUP $1 LABEL $1
  book setword SURVEY_LAPGROUP $1 SEQ_ID $2
  book setword SURVEY_LAPGROUP $1 STATE PENDING
end

macro survey.del.lapgroup
  if ($0 != 2)
    echo "USAGE: survey.del.lapgroup (label)"
    break
  end
  book delpage SURVEY_LAPGROUP $1
end

macro survey.show.lapgroup
  if ($0 != 1)
    echo "USAGE: survey.show.lapgroup"
    break
  end
  book listbook SURVEY_LAPGROUP
end

macro survey.add.relexp
  if ($0 != 3)
    echo "USAGE: survey.add.relexp (label) (releasename)"
    break
  end
  book newpage SURVEY_RELEXP $1
  book setword SURVEY_RELEXP $1 LABEL $1
  book setword SURVEY_RELEXP $1 RELEASE_NAME $2
  book setword SURVEY_RELEXP $1 STATE PENDING
end

macro survey.del.relexp
  if ($0 != 2)
    echo "USAGE: survey.del.relexp (label)"
    break
  end
  book delpage SURVEY_RELEXP $1
end

macro survey.show.relexp
  if ($0 != 1)
    echo "USAGE: survey.show.relexp"
    break
  end
  book listbook SURVEY_RELEXP
end

macro survey.add.relstack
  if ($0 != 4)
    echo "USAGE: survey.add.relstack (label) (releasename) (stacktype)"
    break
  end
  book newpage SURVEY_RELSTACK $1
  book setword SURVEY_RELSTACK $1 LABEL $1
  book setword SURVEY_RELSTACK $1 RELEASE_NAME $2
  book setword SURVEY_RELSTACK $1 STACK_TYPE $3
  book setword SURVEY_RELSTACK $1 STATE PENDING
end

macro survey.del.relstack
  if ($0 != 2)
    echo "USAGE: survey.del.relstack (label)"
    break
  end
  book delpage SURVEY_RELSTACK $1
end

macro survey.show.relstack
  if ($0 != 1)
    echo "USAGE: survey.show.relstack"
    break
  end
  book listbook SURVEY_RELSTACK
end


task survey.diff
  host local
 
  periods      -poll $SURVEY_POLL
  periods      -exec $SURVEY_EXEC
  periods      -timeout $SURVEY_TIMEOUT
  npending     1

  stdout $LOGDIR/survey.diff.log
  stderr $LOGDIR/survey.diff.log

  # generate diff warp-warp runs
  task.exec
    book npages SURVEY_DIFF -var N
    if ($N == 0)
#      echo "No labels for processing"
      break
    endif

    book getpage SURVEY_DIFF 0 -var label -key STATE NEW
    if ("$label" == "NULL")
      # All labels have been done --- reset
#      echo "Resetting labels"
      for i 0 $N
        book getpage SURVEY_DIFF $i -var label
	book setword SURVEY_DIFF $label STATE NEW
      end
      book getpage SURVEY_DIFF 0 -var label -key STATE NEW

      # Select different database
      $SURVEY_DIFF_DB ++
      if ($SURVEY_DIFF_DB >= $DB:n) set SURVEY_DIFF_DB = 0
    end

    book setword SURVEY_DIFF $label STATE DONE
    book getword SURVEY_DIFF $label WORKDIR -var workdir
    book getword SURVEY_DIFF $label DIST_GROUP -var dist_group

    $year = `date +%Y`
    $month = `date +%m`
    $day = `date +%d`

    $run = difftool -definewarpwarp -distance 0.1 -good_frac 0.1 -timediff 3600
    $run = $run -input_label $label
    $run = $run -template_label $label
    $run = $run -set_dist_group $dist_group
    $run = $run -set_label $label
    $run = $run -set_workdir $workdir/$label/$year/$month/$day
    $run = $run -set_reduction WARPWARP

    if ($DB:n == 0)
      option DEFAULT
    else
      $run = $run -dbname $DB:$SURVEY_DIFF_DB
      option $DB:$SURVEY_DIFF_DB
    end
    
    command $run
  end

  # success
  task.exit    0
#    echo "Success"
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

task survey.warpstack.diff
  host local
 
  periods      -poll $SURVEY_POLL
  periods      -exec $SURVEY_EXEC
  periods      -timeout $SURVEY_TIMEOUT
  ## time in UT -- want to start ~0900 HST and run until next night starts. 24h boundary so two trange needed?
  trange        19:00:00 23:59:00
  trange        00:00:00 05:00:00
  npending     1

  stdout $LOGDIR/survey.WSdiff.log
  stderr $LOGDIR/survey.WSdiff.log

  # generate diff warp-stack runs
  task.exec
    book npages SURVEY_DIFF_WARPSTACK -var N
    if ($N == 0)
#      echo "No labels for processing"
      break
    endif

    book getpage SURVEY_DIFF_WARPSTACK 0 -var wsKey -key STATE NEW
    if ("$wsKey" == "NULL")
      # All labels have been done --- reset
#      echo "Resetting labels"
      for i 0 $N
        book getpage SURVEY_DIFF_WARPSTACK $i -var wsKey
	book setword SURVEY_DIFF_WARPSTACK $wsKey STATE NEW
      end
      book getpage SURVEY_DIFF_WARPSTACK 0 -var wsKey -key STATE NEW

      # Select different database
      $SURVEY_DIFF_WARPSTACK_DB ++
      if ($SURVEY_DIFF_WARPSTACK_DB >= $DB:n) set SURVEY_DIFF_WARPSTACK_DB = 0
    end

    book setword SURVEY_DIFF_WARPSTACK $wsKey STATE DONE
    book getword SURVEY_DIFF_WARPSTACK $wsKey WARP_LABEL -var warp_label
    book getword SURVEY_DIFF_WARPSTACK $wsKey STACK_LABEL -var stack_label
    book getword SURVEY_DIFF_WARPSTACK $wsKey WORKDIR -var workdir
    book getword SURVEY_DIFF_WARPSTACK $wsKey DIST_GROUP -var dist_group
    book getword SURVEY_DIFF_WARPSTACK $wsKey TARGET_LABEL -var target_label

    $year = `date +%Y`
    $month = `date +%m`
    $day = `date +%d`

# Do we need anything else here? 
    $run = difftool -definewarpstack -good_frac 0.2
    $run = $run -warp_label $warp_label
    $run = $run -stack_label $stack_label
    $run = $run -set_dist_group $dist_group
    $run = $run -set_label $target_label
    $run = $run -set_workdir $workdir/$target_label/$year/$month/$day
    $run = $run -set_reduction WARPSTACK
    $run = $run -available
    if ($DB:n == 0)
      option DEFAULT
    else
      $run = $run -dbname $DB:$SURVEY_DIFF_WARPSTACK_DB
      option $DB:$SURVEY_DIFF_WARPSTACK_DB
    end
    
    command $run
  end

  # success
  task.exit    0
#    echo "Success"
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

task survey.stackstack.diff
  host local
 
  periods      -poll $SURVEY_POLL
  periods      -exec $SURVEY_EXEC
  periods      -timeout $SURVEY_TIMEOUT
# 13:00 was being done in early morning rather than afternoon HST (so UT value), want both times to have SSdiff ASAP?
# changing earlier for QUB to fetch @10pm for nightly processing there, should be enough time to finish MD stacks
#  trange        13:00:00 14:00:00
  trange	20:00:00 21:00:00
  npending      1

  stdout $LOGDIR/survey.SSdiff.log
  stderr $LOGDIR/survey.SSdiff.log

  # generate diff stack-stack runs
  task.exec
    book npages SURVEY_DIFF_STACKSTACK -var N
    if ($N == 0)
#      echo "No labels for processing"
      break
    endif

    book getpage SURVEY_DIFF_STACKSTACK 0 -var ssKey -key STATE NEW
    if ("$ssKey" == "NULL")
      # All labels have been done --- reset
#      echo "Resetting labels"
      for i 0 $N
        book getpage SURVEY_DIFF_STACKSTACK $i -var ssKey 
        book setword SURVEY_DIFF_STACKSTACK $ssKey STATE NEW
      end
      book getpage SURVEY_DIFF_STACKSTACK 0 -var ssKey -key STATE NEW

      # Select different database
      $SURVEY_DIFF_STACKSTACK_DB ++
      if ($SURVEY_DIFF_STACKSTACK_DB >= $DB:n) set SURVEY_DIFF_STACKSTACK_DB = 0
    end

    book setword SURVEY_DIFF_STACKSTACK $ssKey STATE DONE
    book getword SURVEY_DIFF_STACKSTACK $ssKey INPUT_LABEL -var input_label    
    book getword SURVEY_DIFF_STACKSTACK $ssKey STACK_LABEL -var stack_label
    book getword SURVEY_DIFF_STACKSTACK $ssKey WORKDIR -var workdir
    book getword SURVEY_DIFF_STACKSTACK $ssKey DIST_GROUP -var dist_group
    book getword SURVEY_DIFF_STACKSTACK $ssKey TARGET_LABEL -var target_label    

    $year = `date +%Y`
    $month = `date +%m`
    $day = `date +%d`
# Do we need anything else here? 
    $run = difftool -definestackstack -good_frac 0.2 
    $run = $run -input_label $input_label
    $run = $run -template_label $stack_label
    $run = $run -set_dist_group $dist_group
    $run = $run -set_label $target_label
    $run = $run -set_workdir $workdir/$target_label/$year/$month/$day
    $run = $run -set_reduction STACKSTACK
#    $run = $run -available
    if ($DB:n == 0)
      option DEFAULT
    else
      $run = $run -dbname $DB:$SURVEY_DIFF_STACKSTACK_DB
      option $DB:$SURVEY_DIFF_STACKSTACK_DB
    end
    
    command $run
  end

  # success
  task.exit    0
#    echo "Success"
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



task survey.magic
  host local
 
  periods      -poll $SURVEY_POLL
  periods      -exec $SURVEY_EXEC
  periods      -timeout $SURVEY_TIMEOUT
  npending     1

  stdout $LOGDIR/survey.magic.log
  stderr $LOGDIR/survey.magic.log

  # generate magic warp-warp runs
  task.exec
    book npages SURVEY_MAGIC -var N
    if ($N == 0)
#      echo "No labels for processing"
      break
    endif

    book getpage SURVEY_MAGIC 0 -var label -key STATE NEW
    if ("$label" == "NULL")
      # All labels have been done --- reset
#      echo "Resetting labels"
      for i 0 $N
        book getpage SURVEY_MAGIC $i -var label
	book setword SURVEY_MAGIC $label STATE NEW
      end
      book getpage SURVEY_MAGIC 0 -var label -key STATE NEW

      # Select different database
      $SURVEY_MAGIC_DB ++
      if ($SURVEY_MAGIC_DB >= $DB:n) set SURVEY_MAGIC_DB = 0
    end

    book setword SURVEY_MAGIC $label STATE DONE
    book getword SURVEY_MAGIC $label WORKDIR -var workdir
    book getword SURVEY_MAGIC $label MULTIDIFF -var multidiff

    $run = magictool -rerun -definebyquery -label $label -diff_label $label -workdir $workdir/$label
    
    if (("$multidiff" == "TRUE")||("$multidiff" == "MULTIDIFF"))
       $run = $run -multidiff
    end

    if ($DB:n == 0)
      option DEFAULT
    else
      $run = $run -dbname $DB:$SURVEY_MAGIC_DB
      option $DB:$SURVEY_MAGIC_DB
    end
    
    echo $run
    command $run
  end

  # success
  task.exit    0
#    echo "Success"
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


task survey.addstar
  host local
 
  periods      -poll $SURVEY_POLL
  periods      -exec $SURVEY_EXEC
  periods      -timeout $SURVEY_TIMEOUT
  npending     1

  stdout $LOGDIR/survey.addstar.log
  stderr $LOGDIR/survey.addstar.log

  # generate magic warp-warp runs
  task.exec
    book npages SURVEY_ADDSTAR -var N
    if ($N == 0)
#      echo "No labels for processing"
      break
    endif

    book getpage SURVEY_ADDSTAR 0 -var tag -key STATE NEW
    if ("$tag" == "NULL")
      # All labels have been done --- reset
    # echo "Resetting labels"
      for i 0 $N
        book getpage SURVEY_ADDSTAR $i -var tag
	book setword SURVEY_ADDSTAR $tag STATE NEW
      end
      book getpage SURVEY_ADDSTAR 0 -var tag -key STATE NEW

      # Select different database
      $SURVEY_ADDSTAR_DB ++
      if ($SURVEY_ADDSTAR_DB >= $DB:n) set SURVEY_ADDSTAR_DB = 0
    end

    book setword SURVEY_ADDSTAR $tag STATE DONE
    book getword SURVEY_ADDSTAR $tag LABEL -var label
    book getword SURVEY_ADDSTAR $tag DVODB -var dvodb
    book getword SURVEY_ADDSTAR $tag MINIDVODB_GROUP -var minidvodb_group
    book getword SURVEY_ADDSTAR $tag STAGE -var stage
    
  
 #   $run = addtool -definebyquery -destreaked -label $label -set_dvodb $dbodb
    $run = addtool 
    if ($DB:n == 0)
      option DEFAULT
    else
      $run = $run -dbname $DB:$SURVEY_ADDSTAR_DB
      option $DB:$SURVEY_ADDSTAR_DB
    end
    
    $run = $run -definebyquery -label $label -set_dvodb $dvodb -set_minidvodb_group $minidvodb_group -set_minidvodb -set_label $minidvodb_group -stage $stage
    # echo $run
    if ("$stage" == "cam") 
        #only queue destreaked cams. stacks and staticsky don't need this
    #    if ("$DB:$SURVEY_ADDSTAR_DB" == "isp")
            #this is the only way I can think of how to handle this (but it is messy). If it is a database that we KNOW doesn't use magicked (ie, isp), then do not queue destreaked
           # now with no magic, all are uncensored
           $run = $run -uncensored
#	else
	   #if not isp, then run destreaked version only
#	   $run = $run -destreaked
#	end
    end
    if ("$stage" == "staticsky") 
        #only queue uncensored staticsky
        $run = $run -uncensored
    end
    if ("$stage" == "stack") 
        #only queue uncensored stacks
        $run = $run -uncensored
    end
    if ("$stage" == "skycal")
        #skycal doesn't have magic, however, we still need to tell addtool 
	$run = $run -uncensored
    end
    if ("$stage" == "fullforce")
        $run = $run -uncensored
    end
    if ("$stage" == "diff")
        $run = $run -uncensored
    end
    # we need to handle isp/gpc1: gpc1 wants destreaked, isp wants uncensored.  Perhaps the best way is by default do destreaked (the most paranoid), and if db = isp (is this the best way?) then do uncensored
    

    echo $run
    command $run
  end

  # success
   
  task.exit    0
#    echo "Success"
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


task survey.mergedvodb
  host local
 
  periods      -poll $SURVEY_POLL
  periods      -exec $SURVEY_EXEC
  periods      -timeout $SURVEY_TIMEOUT
  npending     1

  stdout $LOGDIR/survey.mergedvodb.log
  stderr $LOGDIR/survey.mergedvodb.log

  task.exec
    book npages SURVEY_MERGEDVODB -var N
    if ($N == 0)
#      echo "No labels for processing"
      break
    endif

    book getpage SURVEY_MERGEDVODB 0 -var tag -key STATE NEW
    if ("$tag" == "NULL")
      # All labels have been done --- reset
    # echo "Resetting labels"
      for i 0 $N
        book getpage SURVEY_MERGEDVODB $i -var tag
	book setword SURVEY_MERGEDVODB $tag STATE NEW
      end
      book getpage SURVEY_MERGEDVODB 0 -var tag -key STATE NEW

      # Select different database
      $SURVEY_MERGEDVODB_DB ++
      if ($SURVEY_MERGEDVODB_DB >= $DB:n) set SURVEY_MERGEDVODB_DB = 0
    end
   book setword SURVEY_MERGEDVODB $tag STATE DONE
   book getword SURVEY_MERGEDVODB $tag MERGEDVODB -var mergedvodb
   book getword SURVEY_MERGEDVODB $tag MINIDVODB_GROUP -var minidvodb_group
     
 #   $run = addtool -definebyquery -destreaked -label $label -set_dvodb $dbodb
    #$run = mergetool 
    $run = mergedvodb_queue.pl --outroot $mergedvodb --mergedvodb $mergedvodb --minidvodb_group $minidvodb_group
    if ($DB:n == 0)
      option DEFAULT
    else
      $run = $run --dbname $DB:$SURVEY_MERGEDVODB_DB
      option $DB:$SURVEY_MERGEDVODB_DB
    end
    #this run doesn't work because there's no workdir 
    #$run = $run -definebyquery -mergedvodb $mergedvodb -minidvodb_group $minidvodb_group 
    
    
    # echo $run

  echo $run
    command $run
  end

  # success
   
  task.exit    0
#    echo "Success"
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



task survey.destreak
  host local
 
  periods      -poll $SURVEY_POLL
  periods      -exec $SURVEY_EXEC
  periods      -timeout $SURVEY_TIMEOUT
  npending     1

  stdout $LOGDIR/survey.destreak.log
  stderr $LOGDIR/survey.destreak.log

  # generate destreak runs
  task.exec
    book npages SURVEY_DESTREAK -var N
    if ($N == 0)
#      echo "No labels for processing"
      break
    endif

    book getpage SURVEY_DESTREAK 0 -var label -key STATE NEW
    if ("$label" == "NULL")
      # All labels have been done --- reset
#      echo "Resetting labels"
      for i 0 $N
        book getpage SURVEY_DESTREAK $i -var label
	book setword SURVEY_DESTREAK $label STATE NEW
      end
      book getpage SURVEY_DESTREAK 0 -var label -key STATE NEW

      # Select different database
      $SURVEY_DESTREAK_DB ++
      if ($SURVEY_DESTREAK_DB >= $DB:n) set SURVEY_DESTREAK_DB = 0
    end

    book setword SURVEY_DESTREAK $label STATE DONE
    book getword SURVEY_DESTREAK $label WORKDIR -var workdir
    book getword SURVEY_DESTREAK $label RECOVERYROOT -var recoveryroot
  
    $run = magic_destreak_defineruns.pl --label $label --workdir $workdir/$label
    if ("$recoveryroot" != "NULL") 
        $run = $run --recoveryroot $recoveryroot/$label
    end

    if ($DB:n == 0)
      option DEFAULT
    else
      $run = $run --dbname $DB:$SURVEY_DESTREAK_DB
      option $DB:$SURVEY_DESTREAK_DB
    end
    
#    echo $run
    command $run
  end

  # success
  task.exit    0
#    echo "Success"
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

task survey.dist
  host local
 
  periods      -poll $SURVEY_POLL
  periods      -exec $SURVEY_EXEC
  periods      -timeout $SURVEY_TIMEOUT
  npending     1

  stdout $LOGDIR/survey.dist.log
  stderr $LOGDIR/survey.dist.log

  # generate distribution runs
  task.exec
    book npages SURVEY_DIST -var N
    if ($N == 0)
#      echo "No labels for processing"
      break
    endif

    book getpage SURVEY_DIST 0 -var label -key STATE NEW
    if ("$label" == "NULL")
      # All labels have been done --- reset
#      echo "Resetting labels"
      for i 0 $N
        book getpage SURVEY_DIST $i -var label
	book setword SURVEY_DIST $label STATE NEW
      end
      book getpage SURVEY_DIST 0 -var label -key STATE NEW

      # Select different database
      $SURVEY_DIST_DB ++
      if ($SURVEY_DIST_DB >= $DB:n) set SURVEY_DIST_DB = 0
    end

    book getword SURVEY_DIST $label WORKDIR -var workdir_base
    $year = `date +%Y`
    $month = `date +%m`
    $day = `date +%d`
    $workdir = $workdir_base/$label/$year/$month/$day

    book getword SURVEY_DIST $label MUGGLE -var muggle

    book setword SURVEY_DIST $label STATE DONE
  
    # note workdir is set by the script based on site.config
    $run = dist_defineruns.pl --label $label --workdir $workdir
    if ($muggle != 0)
        $run = $run --no_magic 
    end

    if ($DB:n == 0)
      option DEFAULT
    else
      $run = $run --dbname $DB:$SURVEY_DIST_DB
      option $DB:$SURVEY_DIST_DB
    end
    
    if ($VERBOSE > 1 )
        echo $run
    end
    command $run
  end

  # success
  task.exit    0
#    echo "Success"
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

task survey.chip.bg
  host local
 
  periods      -poll $SURVEY_POLL
  periods      -exec $SURVEY_EXEC
  periods      -timeout $SURVEY_TIMEOUT
  npending     1

  stdout $LOGDIR/survey.chip.bg.log
  stderr $LOGDIR/survey.chip.bg.log

  # generate chip_bg runs
  task.exec
    book npages SURVEY_CHIP_BG -var N
    if ($N == 0)
#      echo "No labels for processing"
      break
    endif

    book getpage SURVEY_CHIP_BG 0 -var label -key STATE NEW
    if ("$label" == "NULL")
      # All labels have been done --- reset
#      echo "Resetting labels"
      for i 0 $N
        book getpage SURVEY_CHIP_BG $i -var label
	book setword SURVEY_CHIP_BG $label STATE NEW
      end
      book getpage SURVEY_CHIP_BG 0 -var label -key STATE NEW

      # Select different database
      $SURVEY_CHIP_BG ++
      if ($SURVEY_CHIP_BG >= $DB:n) set SURVEY_CHIP_BG = 0
    end

    book setword SURVEY_CHIP_BG $label STATE DONE
    book getword SURVEY_CHIP_BG $label CAM_LABEL -var cam_label
    book getword SURVEY_CHIP_BG $label DIST_GROUP -var dist_group
#    book getword SURVEY_CHIP_BG $label MUGGLE -var muggle
  
    $run = bgtool -definechip -label $label -cam_label $cam_label -set_dist_group $dist_group
#    if ($muggle == 0)
#        $run = $run -destreaked
#    end

    if ($DB:n == 0)
      option DEFAULT
    else
      $run = $run -dbname $DB:$SURVEY_CHIP_BG
      option $DB:$SURVEY_CHIP_BG
    end
    
    if ($VERBOSE > 1) 
        echo $run
    end
    command $run
  end

  # success
  task.exit    0
#    echo "Success"
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

task survey.warp.bg
  host local
 
  periods      -poll $SURVEY_POLL
  periods      -exec $SURVEY_EXEC
  periods      -timeout $SURVEY_TIMEOUT
  npending     1

  stdout $LOGDIR/survey.warp.bg.log
  stderr $LOGDIR/survey.warp.bg.log

  # generate warp_bg runs
  task.exec
    book npages SURVEY_WARP_BG -var N
    if ($N == 0)
#      echo "No labels for processing"
      break
    endif

    book getpage SURVEY_WARP_BG 0 -var label -key STATE NEW
    if ("$label" == "NULL")
      # All labels have been done --- reset
#      echo "Resetting labels"
      for i 0 $N
        book getpage SURVEY_WARP_BG $i -var label
	book setword SURVEY_WARP_BG $label STATE NEW
      end
      book getpage SURVEY_WARP_BG 0 -var label -key STATE NEW

      # Select different database
      $SURVEY_WARP_BG ++
      if ($SURVEY_WARP_BG >= $DB:n) set SURVEY_WARP_BG = 0
    end

    book setword SURVEY_WARP_BG $label STATE DONE
    book getword SURVEY_WARP_BG $label WARP_LABEL -var warp_label
    book getword SURVEY_WARP_BG $label DIST_GROUP -var dist_group
  
    $run = bgtool -definewarp -set_label $label -chip_bg_label $label -warp_label $warp_label -set_dist_group $dist_group
#    if ($muggle == 0)
#        $run = $run -destreaked
#    end


    if ($DB:n == 0)
      option DEFAULT
    else
      $run = $run -dbname $DB:$SURVEY_WARP_BG
      option $DB:$SURVEY_WARP_BG
    end
    
    if ($VERBOSE > 1) 
        echo $run
    end
    command $run
  end

  # success
  task.exit    0
#    echo "Success"
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

task survey.publish
  host local
 
  # we are going to use a long timeout (900 sec) between passes of the list
  # and a short timeout (10 sec) while we are doing the full list
  periods      -exec 10
  periods      -poll 2
  periods      -timeout $SURVEY_TIMEOUT
  npending     1

  stdout $LOGDIR/survey.publish.log
  stderr $LOGDIR/survey.publish.log

  # generate publish runs
  task.exec
    book npages SURVEY_PUBLISH -var N
    if ($N == 0)
      # echo "No labels for processing"
      break
    endif

    # short timeout while we have something to do
    periods -exec 10

    # survey.publish allows multiple entries per label.
    # The key is called tag and must be unique
    book getpage SURVEY_PUBLISH 0 -var tag -key STATE NEW
    if ("$tag" == "NULL")
      # All tags have been done --- reset
      # echo "Resetting tags"
      for i 0 $N
        book getpage SURVEY_PUBLISH $i -var tag
	book setword SURVEY_PUBLISH $tag STATE NEW
      end
      book getpage SURVEY_PUBLISH 0 -var tag -key STATE NEW

      # Select different database, set a long timeout
      $SURVEY_PUBLISH_DB ++
      if ($SURVEY_PUBLISH_DB >= $DB:n) set SURVEY_PUBLISH_DB = 0
      periods -exec 300
      date -var mytime
      echo "done with publish list @ $mytime"
    end

    book setword SURVEY_PUBLISH $tag STATE DONE
    book getword SURVEY_PUBLISH $tag LABEL -var label
    book getword SURVEY_PUBLISH $tag CLIENT_ID -var client_id
    book getword SURVEY_PUBLISH $tag COMMENT -var comment
  
    $run = pubtool -definerun -label $label -client_id $client_id

    if ("$comment" != "NULL") 
        $run = $run -comment $comment
    end

    if ($DB:n == 0)
      option DEFAULT
    else
      $run = $run -dbname $DB:$SURVEY_PUBLISH_DB
      option $DB:$SURVEY_PUBLISH_DB
    end
    
    # echo $run
    command $run
    
  end

  # success
  task.exit    0
    #echo "Success"
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



task survey.staticskysingle
  host local

  periods      -poll $SURVEY_POLL
  periods      -exec $SURVEY_EXEC
  periods      -timeout $SURVEY_TIMEOUT
  npending     1

  stdout $LOGDIR/survey.staticskysingle.log
  stderr $LOGDIR/survey.staticskysingle.log

    task.exec
    book npages SURVEY_STATICSKYSINGLE -var N
    if ($N == 0)
       # echo "No STATICSKYSINGLE labels for processing"
       break
    endif
    book getpage SURVEY_STATICSKYSINGLE 0 -var tag -key STATE NEW
    if ("$tag" == "NULL")
      # All tags have been done --- reset
      # echo "Resetting tags"
      for i 0 $N
        book getpage SURVEY_STATICSKYSINGLE $i -var tag
        book setword SURVEY_STATICSKYSINGLE $tag STATE NEW
      end
      book getpage SURVEY_STATICSKYSINGLE 0 -var tag -key STATE NEW
      
      # Select different database
      $SURVEY_STATICSKYSINGLE_DB ++
      if ($SURVEY_STATICSKYSINGLE_DB >= $DB:n) set SURVEY_STATICSKYSINGLE_DB = 0
    end

    book setword SURVEY_STATICSKYSINGLE $tag STATE DONE
    book getword SURVEY_STATICSKYSINGLE $tag LABEL -var label
    book getword SURVEY_STATICSKYSINGLE $tag WORKDIR -var workdir
    book getword SURVEY_STATICSKYSINGLE $tag DIST_GROUP -var dist_group
    book getword SURVEY_STATICSKYSINGLE $tag SELECTDATAGROUP -var selectdatagroup
    book getword SURVEY_STATICSKYSINGLE $tag FILTER -var filter

    $year = `date +%Y`
    $month = `date +%m`
    $day = `date +%d`

    $run = staticskytool -definebyquery
    $run = $run -set_workdir $workdir/$label/$year/$month/$day
    $run = $run -set_dist_group $dist_group
    $run = $run -set_label $label
    $run = $run -select_label $label
    $run = $run -select_data_group $selectdatagroup
    $run = $run -select_filter $filter
#   $run = $run -pretend -simple

    if ($DB:n == 0)
      option DEFAULT
    else
      $run = $run -dbname $DB:$SURVEY_STATICSKYSINGLE_DB
      option $DB:$SURVEY_STATICSKYSINGLE_DB
    end

#    echo $run

    command $run
  end

  # success
     task.exit    0
#    echo "Success"
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

# Survey task for Skycal stage

task survey.skycal
  host local
 
  periods      -poll $SURVEY_POLL
  periods      -exec $SURVEY_EXEC
  periods      -timeout $SURVEY_TIMEOUT
  npending     1

  stdout $LOGDIR/survey.skycal.log
  stderr $LOGDIR/survey.skycal.log

  task.exec
    book npages SURVEY_SKYCAL -var N
    if ($N == 0)
#      echo "No labels for processing"
      break
    endif

    book getpage SURVEY_SKYCAL 0 -var label -key STATE NEW
    if ("$label" == "NULL")
      # All labels have been done --- reset
#      echo "Resetting labels"
      for i 0 $N
        book getpage SURVEY_SKYCAL $i -var label
	book setword SURVEY_SKYCAL $label STATE NEW
      end
      book getpage SURVEY_SKYCAL 0 -var label -key STATE NEW

      # Select different database
      $SURVEY_SKYCAL_DB ++
      if ($SURVEY_SKYCAL_DB >= $DB:n) set SURVEY_SKYCAL_DB = 0
    end

    book setword SURVEY_SKYCAL $label STATE DONE
    book getword SURVEY_SKYCAL $label DIST_GROUP -var dist_group

    # note: currently skycal uses the staticskyRun workdir 

    $run = staticskytool -defineskycalrun -set_label $label -select_label $label -set_dist_group $dist_group

    if ($DB:n == 0)
      option DEFAULT
    else
      $run = $run -dbname $DB:$SURVEY_SKYCAL_DB
      option $DB:$SURVEY_SKYCAL_DB
    end
    
    # echo $run
    command $run
  end

  # success
  task.exit    0
#    echo "Success"
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

task survey.lapgroup
  host local
 
  periods      -poll $SURVEY_POLL
  periods      -exec $SURVEY_EXEC
  periods      -timeout $SURVEY_TIMEOUT
  npending     1

  stdout $LOGDIR/survey.lapgroup.log
  stderr $LOGDIR/survey.lapgroup.log

  task.exec
    book npages SURVEY_LAPGROUP -var N
    if ($N == 0)
#      echo "No labels for processing"
      break
    endif

    book getpage SURVEY_LAPGROUP 0 -var label -key STATE NEW
    if ("$label" == "NULL")
      # All labels have been done --- reset
#      echo "Resetting labels"
      for i 0 $N
        book getpage SURVEY_LAPGROUP $i -var label
	book setword SURVEY_LAPGROUP $label STATE NEW
      end
      book getpage SURVEY_LAPGROUP 0 -var label -key STATE NEW

      # Select different database
      $SURVEY_LAPGROUP_DB ++
      if ($SURVEY_LAPGROUP_DB >= $DB:n) set SURVEY_LAPGROUP_DB = 0
    end

    book setword SURVEY_LAPGROUP $label STATE DONE
    book getword SURVEY_LAPGROUP $label SEQ_ID -var SEQ_ID

    # For now the list of filters is hardcoded here
    $run = laptool -definegroup -seq_id $SEQ_ID -set_label $label -filter g.00000 -filter r.00000 -filter i.00000 -filter z.00000 -filter y.00000

    if ($DB:n == 0)
      option DEFAULT
    else
      $run = $run -dbname $DB:$SURVEY_LAPGROUP_DB
      option $DB:$SURVEY_LAPGROUP_DB
    end
    
    # echo $run
    command $run
  end

  # success
  task.exit    0
#    echo "Success"
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

task survey.relexp
  host local
 
  periods      -poll $SURVEY_POLL
  periods      -exec $SURVEY_RELEASE_EXEC
  periods      -timeout $SURVEY_TIMEOUT
  npending     1

  stdout $LOGDIR/survey.relexp.log
  stderr $LOGDIR/survey.relexp.log

  task.exec
    book npages SURVEY_RELEXP -var N
    if ($N == 0)
#      echo "No labels for processing"
      break
    endif

    book getpage SURVEY_RELEXP 0 -var label -key STATE NEW
    if ("$label" == "NULL")
      # All labels have been done --- reset
#      echo "Resetting labels"
      for i 0 $N
        book getpage SURVEY_RELEXP $i -var label
	book setword SURVEY_RELEXP $label STATE NEW
      end
      book getpage SURVEY_RELEXP 0 -var label -key STATE NEW

      # Select different database
      $SURVEY_RELEXP_DB ++
      if ($SURVEY_RELEXP_DB >= $DB:n) set SURVEY_RELEXP_DB = 0
    end

    book setword SURVEY_RELEXP $label STATE DONE
    book getword SURVEY_RELEXP $label RELEASE_NAME -var RELEASE_NAME

    $run = releasetool -definerelexp -label $label -release_name $RELEASE_NAME -set_state processed

    if ($DB:n == 0)
      option DEFAULT
    else
      $run = $run -dbname $DB:$SURVEY_RELEXP_DB
      option $DB:$SURVEY_RELEXP_DB
    end
    
    # echo $run
    command $run
  end

  # success
  task.exit    0
#    echo "Success"
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

task survey.relstack
  host local
 
  periods      -poll $SURVEY_POLL
  periods      -exec $SURVEY_RELEASE_EXEC
  periods      -timeout $SURVEY_TIMEOUT
  npending     1

  stdout $LOGDIR/survey.relstack.log
  stderr $LOGDIR/survey.relstack.log

  task.exec
    book npages SURVEY_RELSTACK -var N
    if ($N == 0)
#      echo "No labels for processing"
      break
    endif

    book getpage SURVEY_RELSTACK 0 -var label -key STATE NEW
    if ("$label" == "NULL")
      # All labels have been done --- reset
#      echo "Resetting labels"
      for i 0 $N
        book getpage SURVEY_RELSTACK $i -var label
	book setword SURVEY_RELSTACK $label STATE NEW
      end
      book getpage SURVEY_RELSTACK 0 -var label -key STATE NEW

      # Select different database
      $SURVEY_RELSTACK_DB ++
      if ($SURVEY_RELSTACK_DB >= $DB:n) set SURVEY_RELSTACK_DB = 0
    end

    book setword SURVEY_RELSTACK $label STATE DONE
    book getword SURVEY_RELSTACK $label RELEASE_NAME -var RELEASE_NAME
    book getword SURVEY_RELSTACK $label STACK_TYPE -var STACK_TYPE

    # XXX: Can we use the survey task for stack types other than nightly?
    $run = releasetool -definerelstack -label $label -release_name $RELEASE_NAME -set_state processed -set_stack_type $STACK_TYPE

    if ($DB:n == 0)
      option DEFAULT
    else
      $run = $run -dbname $DB:$SURVEY_RELSTACK_DB
      option $DB:$SURVEY_RELSTACK_DB
    end
    
    command $run
  end

  # success
  task.exit    0
#    echo "Success"
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
