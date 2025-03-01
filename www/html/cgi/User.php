<?php
// Start the session
session_start();

// Check if the session variable 'views' is set
if (!isset($_SESSION['views'])) {
    $_SESSION['views'] = 0;
}
$_SESSION['views']++;

// Set a cookie
setcookie('user', 'test_user', time() + 3600, "/"); // Cookie expires in 30 days

// Output the session and cookie information
echo "<h1>Contact Page</h1>";
echo "<p>Session Views: " . $_SESSION['views'] . "</p>";
echo "<p>Cookies:</p>";
foreach ($_COOKIE as $key => $value) {
    echo "<p>$key: $value</p>";
}
?>