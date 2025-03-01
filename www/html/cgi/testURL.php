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

echo "<p>Environment variables:</p>\n";
echo "<pre>\n";
// Display some CGI environment variables
echo "REQUEST_METHOD: " . $_SERVER['REQUEST_METHOD'] . "\n";
echo "HTTP_HOST: " . $_SERVER['HTTP_HOST'] . "\n";
echo "REMOTE_ADDR: " . $_SERVER['REMOTE_ADDR'] . "\n";
echo "</pre>\n";

echo "</body>\n";
echo "</html>\n";

//for test on URL do : http://localhost:9999/cgi/testURL.php?name=mohammed&age=30
?>

