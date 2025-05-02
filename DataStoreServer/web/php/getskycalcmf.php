<?php // getskycalcmf.php
// Simple cmf retrieval program
// To download from the page use wget command line
// wget 'http://ippc17/ipp-misc/getskycalcmf.php?release=3PI.PV3&skycell_id=skycell.2386.085&filter=i' --content-disposition
// or 
// curl --remote-name --remote-header-name --location 'http://misc.ipp.ifa.hawaii.edu/getskycalcmf.php?tess_id=RINGS.V3&skycell_id=skycell.2386.085&filter=i'
// This will get the highest prirority (latest) release
// For the cmf specfic release add something like    &release=3PI.PV3
// To just list use wget 'http://ippc17/ipp-misc/getskycalcmf.php?release=3PI.PV3&skycell_id=skycell.2386.085&filter=i&list=1'

$error_string = "";
$debug = 0;

$rvar_tess_id = getVar('tess_id');
$rvar_skycell_id = getVar('skycell_id');
$rvar_filter = getVar('filter');
$rvar_release = getVar('release');

$rvar_stack_id = getVar('stack_id');
$rvar_skycal_id = getVar('skycal_id');
$rvar_list = getVar('list');

$command = "/data/ippc17.0/datastore/ds-cgi/findskycalcmf.pl";

# skycal_id takes priority
if ($skycal_id) {
    $command .= " --skycal_id $skycal_id";
} else if ($rvar_stack_id) {
    $command .= " --stack_id $stack_id";
} else if ($rvar_tess_id && $rvar_skycell_id && $rvar_filter) {
    $command .= " --tess_id $rvar_tess_id --skycell_id $rvar_skycell_id --filter $rvar_filter%";
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
    // something went wrong with the paramters. Error message produced below.
}

if ($gotFile) {
    if ($rvar_list) {
        echo "cmf : $filename\n";
        // echo "<br>path: $pathname\n<br>";
    } else {
        // All systems are go. Time to write the output.
        // First set up the header
        header('Content-type: application/fits');
        header("Content-Disposition: attachment; filename=\"$filename\"");
        $filesize = filesize($pathname);
        header("Content-Length: $filesize");
        header('Expires: now');

        // copy the contents of the file to the stream
        readfile($pathname);
    }
} else {
    // XXX: Figure out how to stop wget from redirecting these error
    // messages to the nasty filename
    $error_string="Could not find cmf";
    if ($rvar_skycal_id) {
        $error_string .= " for skycal_id: $rvar_skycal_id";
    } elseif ($rvar_skycell_id and $rvar_tess_id and $rvar_filter) {
        $error_string .= " for tess_id $rvar_tess_id skycell_id: $rvar_skycell_id filter: $rvar_filter";
    } else {
        $error_string .= ".<br>Not enough parameters supplied";
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
    // print lots of information from the PHP installation
    // phpinfo(-1);

    // print the most useful variables
    //    phpinfo(32);
}

?>
