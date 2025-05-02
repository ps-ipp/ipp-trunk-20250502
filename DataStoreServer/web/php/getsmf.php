<?php // getsmf.php
// Simple smf retrieval program
// To download from the page use wget command line
// wget 'http://ippc17/ipp-misc/getsmf.php?exp_name=o5732g0036o' --content-disposition
// To just list use wget 'http://ippc17/ipp-misc/getsmf.php?exp_name=o5732g0036o&list=1'


$error_string = "";
$debug = 0;

$rvar_exp_name = getVar('exp_name');
$rvar_cam_id = getVar('cam_id');
$rvar_exp_id = getVar('exp_id');
$rvar_release = getVar('release');
$rvar_data_group = getVar('data_group');
$rvar_list = getVar('list');

$command = "/data/ippc17.0/datastore/ds-cgi/findsmf.pl";

# cam_id takes priority
if ($rvar_cam_id) {
    $command .= " --cam_id $rvar_cam_id";
} else if ($rvar_exp_name) {
    $command .= " --exp_name $rvar_exp_name";
} else if ($rvar_exp_id) {
    $command .= " --exp_id $rvar_exp_id";
} else {
    $command = "";
}

if ($command) {
    if ($rvar_release) {
        if ($rvar_release == "3PI.GR1") {
            $rvar_release = "3PI.PV1";
        }
        $command .= " --release $rvar_release";
    } 
    if ($rvar_data_group) {
        $command .= " --data_group $rvar_data_group";
    } 
}

$gotFile = 0;
if ($command) {
    if ($debug) {
        echo "<br>$command\n<br>";
    }
    $command = escapeshellcmd($command);
    $output = array();

    exec($command, $output, $command_status);

    if ($command_status == 0) {
        // we only expect one line of output
        $len = count($output);
        if ($len == 1) {
            list($filename, $pathname) = explode(" ", $output[0]);
            if ($filename && $pathname) {
                if (!$debug) {
                    $gotFile = 1;
                } else {
                    echo "$filename $pathname\n";
                    echo "$command\n";
                }
            }
        } else {
            echo "<br>unexpected output from $command: $output[0] $output[1]\n";
        }
    } else {
        if ($debug) {
            echo "<br>command failed $command_status\n";
        }
    }
} else {
}

if ($gotFile) {
    if (!$rvar_list) {
        // All systems are go. Time to write the output.
        // First set up the header
        header('Content-type: application/fits');
        header("Content-Disposition: attachment; filename=\"$filename\"");
        $filesize = filesize($pathname);
        header("Content-Length: $filesize");
        header('Expires: now');

        // copy the contents of the file to the stream
        readfile($pathname);
    } else {
        echo "smf : $filename\n";
        # echo "path: $pathname\n";
    }
} else {
    // XXX: Figure out how to stop wget from redirecting these error
    // messages to the nasty filename
    $error_string="Could not find smf";
    if ($rvar_cam_id) {
        $error_string .= " for cam_id: $rvar_cam_id";
    } elseif ($rvar_exp_name) {
        $error_string .= " for exposure: $rvar_exp_name";
    } else {
        $error_string .= ". No cam_id or exp_name provided";
    }
        
    echo "$error_string.\n";
}

function getVar($var) {
    if ($_SERVER['REQUEST_METHOD'] == 'POST') {
        $rvar = $_POST[$var];
    } else {
        $rvar = $_GET[$var];
    }
    $rvar = stripslashes($rvar);
    $rvar = htmlentities($rvar);
    $rvar = strip_tags($rvar);
    return $rvar;
}


if ($list) {
    // print lots of information
    // phpinfo(-1);

    // print the most useful variables
    //    phpinfo(32);
}

?>
