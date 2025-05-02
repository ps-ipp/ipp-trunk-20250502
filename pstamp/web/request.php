<?php 

// prototype postage stamp server web interface

// A php program that generates a postage stamp request form

// There are two modes on the form List Images simply lists the input images
// that match the image selection critera

// When make stamps is selected when the page is posted, a postage stamp request is queued.
// 
// Then we respond to http posts by listing the status of the jobs.
// Once all of the jobs complete we set the page back to submission mode again
// The mode for the page is given by $request_id != 0

// XXX This is just a prototype for testing purposes. 

require "pstamp.php";
require "submitted.php";

# this script sets up the environment to run IPP commands with current directory
# $WORKDIR
$SCRIPT    = "$PSBINDIR/pstamp_runcommand.sh $PSCONFDIR $PSCONFIG $WORKDIR";

// END of moved to pstampconfig.php

// Initialize variables
$output_array = array();

$request_id = 0;
$last_request_id = 0;

$raw_selected = "";
$chip_selected = "";
$warp_selected = "";
$stack_selected = "";
$diff_selected = "";

$exp_checked = "";
$file_checked = "";
$coord_checked = "";
$diff_checked = "";
$list_checked = "";
$getstatus_checked = "";
$pstamp_checked = "";
$get_checked = "";

$command_line = "";
$error_line = "";
$command_status = "";

$outFileset = "";

// Initialize the request variables that we depend upon
$rvar_project="";

$gpc1_selected="";
$mops_selected="";
$simtest_selected="";

$require_class_id = 0;

$rvar_select_by = "";
$rvar_img_type = "";
$rvar_id = "";
$rvar_class_id = "";
$rvar_cell_id = "";

$rvar_center_type = "";
$rvar_range_type  = "";

$rvar_RA = "";
$rvar_DEC = "";
$rvar_dRA = "";
$rvar_dDEC = "";
$rvar_X = "";
$rvar_Y = "";
$rvar_W = "";
$rvar_H = "";

$rvar_cmd_mode = "";
$rvar_last_cmd_mode = "";
$rvar_request_id = 0;
$rvar_last_request_id = 0;

// now get the values from this post

import_request_variables("gp", "rvar_");

if ($rvar_project == "gpc1") {
    $gpc1_selected = "selected";
//    $require_class_id = 1;
} else if ($rvar_project == "megacam-mops") {
    $mops_selected = "selected";
    $require_class_id = 0;
} else { //    if ($rvar_project == "simtest") {
    $simtest_selected = "selected";
    $require_class_id = 0;
}

// figure out which select_by is set and save it's checked value 

if ($rvar_select_by == "exposure_id") {
    $exp_checked = "CHECKED";
} else if ($rvar_select_by == "db_id") {
    $file_checked = "CHECKED";
} else if ($rvar_select_by == "coord") {
    $coord_checked = "CHECKED";
} else if ($rvar_select_by == "diff_image_id") {
    $diff_checked = "CHECKED";
} else {
    // nothing checked default to By ID
    $file_checked = "CHECKED";
}

// get the Image type
if ($rvar_img_type == "raw") {
    $raw_selected = "selected";
} else if ($rvar_img_type == "chip") {
    $chip_selected = "selected";
} else if ($rvar_img_type == "warp") {
    $warp_selected = "selected";
} else if ($rvar_img_type == "stack") {
    $stack_selected = "selected";
} else if ($rvar_img_type == "diff") {
    $diff_selected = "selected";
}

// is the center is specified in Pixels or sky coordinates
if ($rvar_center_type == "Pixels") {
    $sky_checked="";
    $pix_checked="checked";
} else {
    $sky_checked="checked";
    $pix_checked="";
}

// is the range is specified in Pixels or sky coordinates
if ($rvar_range_type == "Sky") {
    $rsky_checked="checked";
    $rpix_checked="";
} else {
    $rpix_checked="checked";
    $rsky_checked="";
}

// When request_id is non-zero we respond to posts by check the status of that request
// request_id gets set to zero when the status of all jobs for the request is 'stop'
$request_id = $rvar_request_id;
$last_request_id = $rvar_last_request_id;

// during request the page doesn't have cmd_mode set. So we remember the lat real value
// and use that
if (!$rvar_cmd_mode) {
    $rvar_cmd_mode = "$rvar_last_cmd_mode";
}

$getstatus_checked = "";
$pstamp_checked = "";
$list_checked = "";
$get_checked = "";
if ($rvar_cmd_mode == "Make Stamps") {
    $pstamp_checked = "checked";
} else if ($rvar_cmd_mode == "Get Images") {
    $get_checked = "checked";
} else if ($rvar_cmd_mode == "Get Status") {
    $getstatus_checked = "checked";
} else {
    // default
    $pstamp_checked = "checked";
}


// echo "rvar_request_id: $rvar_request_id\n";

// HERE is the logic for running the various commands

// How do we know whether or not this is the intial page load or not?
// Well, the first time rvar_img_type is not set. So we key off of that.
// TODO: find a better way to decide whether or not to run commands

if ($rvar_img_type) {
    $jobFinished = 0;
    if (! $getstatus_checked) {
        try {
            $command_line = build_request_cmd();
            $error_line = "";
            run_command($command_line);
            if (! $list_checked) {
                // The only output from a successful run is the request_id
                $request_name = trim(Array_pop($output_array));
                $request_id = Array_pop($output_array);
                $last_request_id = $request_id;
                if ($request_id && $request_name) {
                    addRequest($request_id, $request_name);
                    // setcookie("our_request_id", $request_id);
                    // echo "The request id is $request_id\n";
                    $getstatus_checked = "checked";
                    $pstamp_checked = "";
                } else {
                    // XXX: TODO print out the error
                    if (count($output_array) != 0) {
                        throw new Exception("unexpected output returned by pstampwebrequest.");
                    }
                }
            } else {
                $last_request_id = 0;
            }
        } catch (Exception $e) {
            $error_line = $e->getMessage();
        }
    }
}

// This is the end of the Logic

function build_request_cmd()
{
    global $rvar_project;
    global $sky_checked, $rsky_checked;
    global $list_checked;
    global $get_checked;
    global $rvar_RA, $rvar_DEC;
    global $rvar_dRA, $rvar_dDEC;
    global $rvar_X, $rvar_Y;
    global $rvar_W, $rvar_H;
    global $exp_checked, $file_checked, $coord_checked, $diff_checked;
    global $rvar_img_type;
    global $rvar_id, $rvar_class_id;
    global $command_line;
    global $dbname;
    global $dbserver;
    global $require_class_id;
    global $PSCONFDIR, $PSCONFIG, $WORKDIR;
    global $SCRIPT;

    $making_stamps = 1;
    $cmd = "$SCRIPT pstamp_webrequest.pl";

    if ($list_checked) {
        $cmd .= " --job_type list_uri";
        $making_stamps = 0;
    } else if ($get_checked) {
        $making_stamps = 0;
        $cmd .= " --job_type get_image";
    }

    if ($dbname) {
        $cmd .= " --dbname $dbname --dbserver $dbserver";
    }

    if (! $rvar_project ) {
        throw new Exception('project must be specified.');
    }
    $cmd .= " --project $rvar_project";

    if ($making_stamps) {
        // TODO: put options on the GUI for these
//        $cmd .= " -mask -weight";
    }

    if ($making_stamps || $coord_checked) {
        // Set up the ROI parameters
        if ($sky_checked) {
            if (! $rvar_RA || ! $rvar_DEC) {
                throw new Exception('RA and DEC must be specified.');
            }
            $cmd .= " --ra $rvar_RA --dec $rvar_DEC";
        } else {
            if (! $rvar_X || ! $rvar_Y) {
                throw new Exception('X and Y must be specified.');
            }
            $cmd .= " -pixcenter --x $rvar_X --y $rvar_Y";
        }

        if ($rsky_checked) {
            if (! $rvar_dRA || ! $rvar_dDEC) {
                throw new Exception('dRA and dDEC must be specified.');
            }
            $cmd .= " --arcseconds --width $rvar_dRA --height $rvar_dDEC";
        } else {
            if (! $rvar_W || ! $rvar_H) {
                throw new Exception('width and height must be specified.');
            }
            $cmd .= " --width $rvar_W --height $rvar_H";
        }
    }


    if (! $rvar_img_type) {
        // this actually can't happen
        throw new Exception('Must set image type.');
    }

    if ($exp_checked) {
        if ($rvar_img_type == "stack") {
            throw new Exception('Lookup by exposure name not supported for stack images.');
        }
        if (! $rvar_id ) {
            throw new Exception('Must set ID to the Exposure ID.');
        }
        $cmd .= " --req_type byexp --stage $rvar_img_type --id $rvar_id";
    } else if ($file_checked) {
        if (! $rvar_id ) {
            throw new Exception('Must set ID to the exposure name.');
        }
        $cmd .= " --req_type byid --stage $rvar_img_type --id $rvar_id";
    } else if ($coord_checked) {
        $cmd .= " --req_type bycoord --stage $rvar_img_type";
        $coord_checked = "checked";
//        throw new Exception("Image selection by coordinate not implemented yet.");
    } else if ($diff_checked) {
        if (! $rvar_id ) {
            throw new Exception('Must set ID to Diff Image ID.');
        }
        $cmd .= " --req_type bydiff --stage $rvar_img_type --id $rvar_id";
    }

// XXX: don't need to require class_id anymore
//    if (($rvar_img_type == "raw") || ($rvar_img_type == "chip")) {
//        if (!$sky_checked && ($require_class_id && ! $rvar_class_id )) {
//            throw new Exception("must specify Class ID with Image Type $rvar_img_type.");
//        }
        // leave off compoennt if we're looking up by coordinates. It breaks it
        if (!$coord_checked && (($rvar_class_id) && ($rvar_class_id != "all"))) {
            $cmd .= " --component $rvar_class_id";
        }
//    }

    return escapeshellcmd($cmd);
}

function run_command($command_line)
{
    global $output_array;
    global $error_line;
    global $command_status;

    //    echo "running $command_line\n";

    exec ("$command_line", $output_array, $command_status);

    $size = sizeof($output_array);
    //     echo "command_status: $command_status output_array  contains $size lines\n";
    if ($command_status == 0) {
        // On success we just remember the results
        $dump_results = 0;
        if ($dump_results) {
            echo "Output: $size lines\n";
            for ($i = 0; $i < $size; $i++) {
                echo "$output_array[$i]\n";
            }
        }
    } else {
        // copy the output to the error_line
        $error_line = "";
        for ($i = 0; $i < $size; $i++) {
             $error_line .= "$output_array[$i]\n";
        }
    }

}

// This is no longer used
function printURL($line)
{
    global $request_id;
    global $last_request_id;

    echo "<tr><td>";
    $doURL = 1;
    if ($doURL) {
// echo "<pre>output_line: $line\n</pre>";
        // Parse the output from pstamp_list_jobs.pl
        $elements = explode(" ", $line);
        if (count($elements) == 6) {
            $job_id   = $elements[0];
            $state    = $elements[1];
            $fault    = $elements[2];
            $req_name = $elements[3];
            $product  = $elements[4];
            $path     = $elements[5];
            $fileName = basename($path);
            if ($state == "stop") {
                global $dsroot;
                $dirName  = "$dsroot/$product/$req_name";
                // XXX: TODO: make this a configuration parameter
                $filesetURL = "http://datastore.ipp.ifa.hawaii.edu/$product/$req_name";
                $fullpath = "$dirName/$fileName";
// echo "<pre>fullpath: $fullpath filesetURL: $filesetURL\n</pre>";
                if (file_exists($dirName)) {
                    // this job is done, list the url as a link
                    // echo "<a href=\"http:$path\" target=\"_blank\" type=\"image/fits\">";
                    echo "<a href=\"$filesetURL\" TARGET=\"form_results_fileset\">";
#                    echo $fileName;
                    $filesetName = basename($dirName);
                    echo "Fileset: $filesetName";
                    echo "</a>";
                    echo "&nbsp;&nbsp;&nbsp; Base: $fileName &nbsp;&nbsp;&nbsp; request id: $last_request_id &nbsp;&nbsp;";
                    echo "job_id: $job_id &nbsp;&nbsp;&nbsp; state: $state";
                } else {
                    echo "request id: $last_request_id  job id: $job_id failed";
                    echo "   $fullpath";
                }
            } else {
                // TODO: refine this output
                echo "$fileName&nbsp;&nbsp;&nbsp; request_id: $request_id &nbsp;&nbsp;&nbsp;";
                echo "job_id: $job_id &nbsp;&nbsp;&nbsp; state: $state";
            }
        }
    } else {
        print "$line";
    }

    echo "</td></tr>";
}
?>

<!----------------------Beginning of the HTML --------------------------------------------- -->

<html>
<head>
  <title>
    Postage Stamp Request Form (prototype)
  </title>
</head>
<body>

<H1 align=center>
Postage Stamp Request Form
</h1>

<?php
    welcomeHeader($auth_user, "pstamp_links.php", "Postage Stamp Home");
?>

<form method="post">
<!-- Whole page is a single column table -->


<table width=90% align=center>


<!-- first row in the main table is the image selector UI which consists of a 2 x 3 table -->
<tr>
<td>

<table width=100% align=center>

<!-- first row of image selector is "Project" pulldown menu "Select Image By" radio boxes -->
<tr>
<td width=20% >
  <table>
    <tr><td><b>Project:</b>&nbsp;&nbsp;&nbsp;</td>
        <td><select name="project">
           <option <?php echo $gpc1_selected;?> >gpc1
<!--
           <option <?php echo $mops_selected;?> >megacam-mops
           <option <?php echo $simtest_selected;?> >simtest
-->
        </td>
    </tr>
  </table>

<td>
&nbsp;<b>Select Images By:</b>&nbsp;&nbsp;&nbsp;
<input type=radio name="select_by" value="db_id" <?php echo $file_checked; ?> >Database ID
&nbsp;
<input type=radio name="select_by" value="exposure_id" <?php echo $exp_checked; ?> >Exposure Name
&nbsp;

<input type=radio name="select_by" value="coord" <?php echo $coord_checked; ?> >Coordinates
&nbsp;
<input type=radio name="select_by" value="diff_image_id" <?php echo $diff_checked; ?> >Diff Image ID
</td>
</tr>


<!-- the second row of the image selector table contains 
    "Image Type" pulldown menu   "ID:" text input "Class ID" text input
-->

<tr>
<td width=20%>
<table>
    <td><b>Image Type:</b>&nbsp;</td>
    <td>
        <select name="img_type">" >
            <option <?php echo $chip_selected;?>  >chip
            <option <?php echo $warp_selected ;?> >warp
            <option <?php echo $stack_selected;?> >stack   
            <option <?php echo $diff_selected;?>  >diff
            <option <?php echo $raw_selected;?>   >raw
        </select>
    </td>
</table>
</td>

<td>
&nbsp;<b>ID/Name:</b>&nbsp; &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
<input type="text" name="id" value= <?php echo $rvar_id; ?> >
&nbsp;&nbsp;&nbsp;&nbsp;
<b>
<?php if (0 && $rvar_project == "gpc1") {
        echo "Chip ID:";
      } else {
        echo "Component:";
      }
?>
</b>
&nbsp;<input type="text" name="class_id" size=10 value="<?php echo $rvar_class_id;?>" >

&nbsp;&nbsp;&nbsp;&nbsp;
<!-- add text input field for Cell ID, not used yet -->

<!--
<b>Cell ID:</b>&nbsp;<input type="text" name="cell_id" size=10 value="<?php echo $rvar_cell_id;?>" >

-->

</td>

</tr>

</table> <!-- end of Image selector table -->

<!-- a blank row for space-->
<tr height=20><td></td></tr>

<!-- next row of main table contains the ROI input UI -->
<tr align=center width=100%>
    <td>
    <!-- 2 row by 6 column table -->
    <table width=100%>
    <thead>
    <tr>
    <!-- <td></td><td></td><td><b>Center</b></td><td></td><td></td><td><b>Range</b></td> -->
    <td><td><td><b>Center</b><td><td><td><b>Range</b>
    </tr>
    </thead>
    <tr>
        <td>
            <input type=radio name="center_type" value="Sky" <?php echo $sky_checked; ?> >Sky
        </td>
        <td>
            &nbsp;
            RA:
            &nbsp;
            &nbsp;
            <input type="text" name="RA" size=10  value= <?php echo $rvar_RA; ?> >
        </td>
        <td>
            &nbsp;
            DEC:
            &nbsp;
            <input type="text" name="DEC" size=10 value="<?php echo $rvar_DEC;?>" >
        </td>
        <td>
            <input type=radio name="range_type" value="Sky" <?php echo $rsky_checked; ?> >Sky
        </td>
        <td>
            &nbsp;
            dRA:
            &nbsp;
            &nbsp;
            <input type="text" name="dRA" size=10  value= <?php echo $rvar_dRA; ?> >
            &nbsp; "
        </td>
        <td>
            &nbsp;
            dDEC:
            &nbsp;
            <input type="text" name="dDEC" size=10 value="<?php echo $rvar_dDEC;?>" >
            &nbsp; "
        </td>
    </tr>
    <tr>
        <td>
            <input type=radio name="center_type" value="Pixels" <?php echo $pix_checked; ?> >Pixels
        </td>
        <td>
            &nbsp;
            X:
            &nbsp;
            &nbsp;
            &nbsp;
            <input type="text" name="X" size=10  value= <?php echo $rvar_X; ?> >
        </td>
        <td>
            &nbsp;
            Y:
            &nbsp;
            &nbsp;
            &nbsp;
            <input type="text" name="Y" size=10 value="<?php echo $rvar_Y;?>" >
        </td>
        <td>
            <input type=radio name="range_type" value="Pixels" <?php echo $rpix_checked; ?> >Pixels
        </td>
        <td>
            &nbsp;
            width:
            &nbsp;
            &nbsp;
            <input type="text" name="W" size=10  value= <?php echo $rvar_W; ?> >
        </td>
        <td>
            &nbsp;
            height:
            &nbsp;
            <input type="text" name="H" size=10 value="<?php echo $rvar_H;?>" >
        </td>
    </tr>
    </table>
    </td>
</tr>


<!-- next row of the main table contains the Submit button and the Mode radio buttons -->

<tr align=center width=100% height=50>
  <td>
  <table width=80%>
  <tr>

    <td><input type=submit value="Submit"></td>
    <td><b>Mode:</b>&nbsp;&nbsp;
    <input type=radio name="cmd_mode" value="Get Status"<?php echo $getstatus_checked; ?> >Get Status
    <input type=radio name="cmd_mode" value="Make Stamps"<?php echo $pstamp_checked; ?> >Make Stamps
    <input type=radio name="cmd_mode" value="Get Images" <?php echo $get_checked; ?> >Get Bundles
<!--
    <input type=radio name="cmd_mode" value="List Images" <?php echo $list_checked; ?> >List Images
-->
    </td>
<?php
  // echo "<td><b>Request Id: $request_id";
?>

  </tr>
  </table>
  </td>
</tr>

<!--- next row in the main table is the Results of the previous request --->

<tr>
<!--- Don't show the command
    <td>
    <b>Command:</b>&nbsp;&nbsp; <?php echo "$command_line\n";?>
    </td>
--->

</tr>
<tr>
    <td>
    <b>Last Command</b>
    </td>
</tr>
<tr>
    <td>
    <b>Status:</b>&nbsp;&nbsp;&nbsp; <?php echo "$command_status\n";?>
    </td>
</tr>
<tr>
    <td>
    <b>Error:</b>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; 
        <?php 
            if ($error_line) {
                echo "<pre>";
                echo "$error_line\n";
                echo "<pre>\n";
            }
        ?>
    </td>
</tr>

<!-- a blank row for space -->
<tr height=20><td> </td></tr>

<tr>
<td>
<table align=center width=100% rules=none>
<caption height=10 valign=center><b>Request Results</b></caption>

<?php 
if (0) {
    // This is the old way of listing the status of the current request.
    // now we save the submitted requests in the session see listRequests() below

    $size = sizeof($output_array);
    // echo "<pre>size of output array is $size\n</pre>";
    if ($command_status == 0) {
        if ($list_checked) {
            // in list mode the output is a list of image files, just list them
            // later we might add links to cause a stamp to be made from a selected file
            for ($i = 0; $i < $size; $i++)  {
                // $uri = array_shift($output_array);
                $uri = $output_array[$i];
                echo "<tr><td>$uri</td></tr>";
            }
        } else {
            // output the list of urls
            for ($i = 0; $i < $size; $i++)  {
                // $uri = array_shift($output_array);
                $uri = $output_array[$i];
                printURL($uri);
            }
        }
    }
} // end if if(0)
?>
</table>
</td>
</tr>
<!-- a blank row for our hidden element here-->

<!-- request_id being non-zero causes us to issue status requests instead of new requsts -->
<!--

need a way to cancel a request There's probably no reason to have the value hidden, but we do
need to set it as the last thing that we do

-->

<tr height=20>
<td>
    <input type="hidden" name="request_id" value=<?php echo $request_id ?> >
</td>
<td>
    <input type="hidden" name="last_request_id" value=<?php echo $last_request_id ?> >
</td>
<td>
    <input type="hidden" name="last_cmd_mode" value=<?php echo "\"$rvar_cmd_mode\"" ?> >
</td>
</tr>
</table>

<?php
    listRequests("http://datastore.ipp.ifa.hawaii.edu/pstampresults", "pstamp_results_fileset");
?>

<!-- The end -->

<p>
<pre>


<?php 

//    echo "select_by: $rvar_select_by diff_checked: $diff_checked exp_checked: $exp_checked file_checked: $file_checked coord_checked: $coord_checked\n";

    // dump parameters 

    // phpinfo(32);

?>

</form>
</body>
</html>
