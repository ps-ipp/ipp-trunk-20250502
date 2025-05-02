<?php

// function check_login()

session_start();

// XXX TODO use mysql
$auth_user="setme";
$auth_passwd="setmetoo";

$user   = $_SERVER['PHP_AUTH_USER'];
$passwd = $_SERVER['PHP_AUTH_PW'];
$did_login = isset($_SESSION['did_login']);

if ($did_login && isset($user) && isset($passwd) &&
    ($auth_user == $user) && ($auth_passwd == $passwd)) {

    echo "Welcome:  " . $user;
    echo "&nbsp;&nbsp;&nbsp; <a href=\"./logout.php\">Logout</a>";
    echo "<br />";
    echo "<br />";
} else {
    $_SESSION['did_login'] = true;
    header('WWW-Authenticate: Basic realm="Restricted Section"');
    header('HTTP/1.0 401 Unauthorized');
    // The following will be output if the user hits the cancel button
    echo "please enter username and password";
    echo "&nbsp;&nbsp;&nbsp;<a href=\"./upload.php\">Login</a>";
    exit;
}

?>

