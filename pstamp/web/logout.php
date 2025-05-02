<?php

// require_once "authenticate.php";

// logout();
session_start();
$_SESSION = array();
if (isset($_COOKIE[session_name()])) {
    setcookie(session_name(), '', time()-42000, '/');
}
session_destroy();

echo "You are now logged out<br /><br />";
echo "<a href=\"./upload.php\">Upload</a>";

// phpinfo(32);
?>

