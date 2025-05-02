<?php // listsmfs.php
// Simple smf listing program


$error_string = "";
$debug = 0;
$list  = 0;  # another debugging tool. list phpinfo

$rvar_date_min = getVar('date_min');
$rvar_date_max = getVar('date_max');
$rvar_filter   = getVar('filter');
$rvar_label    = getVar('label');
$rvar_data_group = getVar('data_group');
$rvar_release  = getVar('release');
$rvar_camera   = getVar('camera');

# these params aren't yet used (and may not be)
$rvar_exp_name = getVar('exp_name');
$rvar_cam_id = getVar('cam_id');
$rvar_exp_id = getVar('exp_id');

$command = "/data/ippc17.0/datastore/ds-cgi/listsmfs.pl";

# max date is ignored unless min date is supplied
if ($rvar_date_min) {
    $command .= " --dateobs_min $rvar_date_min";
    if ($rvar_date_max) {
        $command .= " --dateobs_max $rvar_date_max";
    }
}

if ($rvar_filter) {
    $command .= " --filter $rvar_filter";
}

if ($rvar_label) {
    $command .= " --label $rvar_label";
}

if ($rvar_release) {
    $command .= " --release $rvar_release";
} 

if ($rvar_data_group) {
    $command .= " --data_group $rvar_data_group";
} 

if ($rvar_camera) {
    $command .= " --dbname $rvar_camera";
}




$agent = $_SERVER['HTTP_USER_AGENT'];
$pos_curl = stripos($agent, 'curl');
$pos_wget = stripos($agent, 'wget');

if ($pos_curl === false && $pos_wget === false) {
    $not_browser = 0;
} else {
    $not_browser = 1;
}

$submitter_ip_addr = $_SERVER['HTTP_X_FORWARDED_FOR'];
$remote_ip_addr = $_SERVER['REMOTE_ADDR'];
if ($submitter_ip_addr  === false) {
    $submitter_ip_addr = $remote_ip_addr;
}
if (!$submitter_ip_addr) {
    $submitter_ip_addr = $remote_ip_addr;
}

# OUTPUT begins here

# not sure whether this really makes a difference
if ($not_browser) {
    echo header('text/plain', '200 OK');
} else {
    echo header('text/html', '200 OK');
}

if ($list) {
    # debug mode to list phpinfo, set the title
    # to prevent title from being phpinfo
    echo "<head> <title>smf list</title></head>\n";
}

if ($debug) {
    echo "pos_curl: $pos_curl pos_wget: $pos_wget\n";
}


if ($command) {
    $command = escapeshellcmd($command);
    if ($debug) {
        $agent = $_SERVER['HTTP_USER_AGENT'];
        echo "HTTP_USER_AGENT is: $agent\n<br>";
        echo "<br>$command\n<br>";
    }
    $output = array();

    # RUN the command
    exec($command, $output, $command_status);

    if ($command_status == 0) {
        if (!$not_browser) {
            # when viewing in a web browser without this the newlines get lost
            echo "<pre>";
        }

        # output the list
        foreach( $output as $line) {
            echo "$line\n";
        }
    } else {
        if ($debug) {
            echo "<br>command failed $command_status\n";
        }
    }
} else {
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
       print "</pre>\n";
       print "<p>Submitter IP ADDR: $submitter_ip_addr\n";
       print "<br>Remote IP ADDR: $remote_ip_addr\n";

    // print the most useful variables
       phpinfo(32);
}

?>
