<?php // upload.php

// get the locatl configuration variables
include "pstamp.php";


$user = $_SERVER['PHP_AUTH_USER'];
$passwd = $_SERVER['PHP_AUTH_PW'];
echo "<HTML>
<head>
    <title>
        Upload Postage Stamp Request File
    </title>
<body>
";

// echo "Hello $user $passwd";

echo <<<_END
    <form method="post" enctype="multipart/form-data" action="">
        <label>Postage Stamp Request File:
        <input type="file" name='filename' accept='image/x-fits' /></label>
    <br />
    <br />
    <input type="submit" value="Upload" />
    &nbsp; &nbsp; &nbsp;
    <input type="reset" name="cancel" value="Cancel"/>
    <br />
    </form>
    <br />
_END;

$command = "";
if ($_FILES) {
    $name = $_FILES['filename']['name'];
    $tmp_name = $_FILES['filename']['tmp_name'];
    $type = $_FILES['filename']['type'];
    $size = $_FILES['filename']['size'];

    if ($name && ($size > 0)) {
  //      echo "Uploaded $size bytes for '$type' file '$name' as '$tmp_name'<br />";
        $command = "$SCRIPT pstamp_insert_request.pl --tmp_req_file $tmp_name --dbname $dbname --dbserver $dbserver --workdir $WORKDIR";
    }
}

class Request
{
    public $id;
    public $name;
    function __construct($p1, $p2)
    {
        $this->id = $p1;
        $this->name = $p2;
    }
}

if (!isset($_SESSION['requests'])) {
//    echo "initializing requests\n";
//    echo "<br />";
    $_SESSION['requests'] = array();
}

// echo "<br />SCRIPT is <br />$SCRIPT";
$req_id = 0;
$req_name = "";
$new_request = 0;
if ($command) {
    $command = escapeshellcmd($command);
   // echo "<br />command:<br />$command";
    echo "<br />";
    exec($command, $output, $command_status);
    if ($command_status == 0) {
        // it worked!
        $req_id = $output[0];
        // get rid of any whitespace in req_name
        $req_name = trim($output[1]);
        echo "Submitted Request ID:&nbsp; $req_id Request Name: &nbsp; $req_name<br \>";
        $new_request = new Request($req_id, $req_name);
        $num = count($_SESSION['requests']);
//        echo "request array length: $num<br />";
        $_SESSION['requests'][$num] = $new_request;
        $num = count($_SESSION['requests']);
//        echo "after request array length: $num<br />";
    } else if ($command_status == 5) {
        // PS_EXIT_DATA_ERROR
        echo "Error:&nbsp;&nbsp;&nbsp;";
        for ($i=0; $i < count($output); $i++) {
            echo $output[$i];
        }
        echo "<br />\n";
    } else {
        echo "Unexpected Error.<br /> insert command returned $command_status<br \>";
    }
}
echo "<br />";

foreach ($_SESSION['requests'] as $req) {
    echo "<br />";
    echo $req->id;
    echo "&nbsp;&nbsp";
    $name = $req->name;
    echo "&nbsp;&nbsp";
    // XXX: get this data store product location a configuation
    echo "<a href=\"http://datastore.ipp.ifa.hawaii.edu/pstampresults/$name\" TARGET=\"upload_results_fileset\">$name</a>\n";
}
echo "<br />";

// print lots of information
// phpinfo(-1);
// print the most useful variables
// phpinfo(32);

echo "</body></html>";

?>
