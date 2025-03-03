<?php

// Set content type header
header("Content-Type: text/html");

// Get query parameters (GET request)
$name = isset($_GET['name']) ? htmlspecialchars($_GET['name']) : "Guest";
$age = isset($_GET['age']) ? htmlspecialchars($_GET['age']) : "0";

// Output HTML response
echo "<!DOCTYPE html>\n";
echo "<html>\n";
echo "<head><title>PHP CGI Test</title></head>\n";
echo "<body>\n";
echo "<h1>Hello, $name! your age is $age</h1>\n";
echo "</body>\n";
echo "</html>\n";

//for test on URL do : http://localhost:9999/cgi/testURL.php?name=mohammed&age=30
?>

