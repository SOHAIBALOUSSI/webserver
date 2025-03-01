<?php
// Home.php

// Set content type header
header("Content-Type: text/html");

echo "<!DOCTYPE html>\n";
echo "<html>\n";
echo "<head><title>Home - Submit Form</title></head>\n";
echo "<body>\n";
echo "<h1>Welcome to Home Page</h1>\n";
echo "<p>Please enter your name and age:</p>\n";

// Form to submit to contact.php via POST
echo "<form method='POST' action='/cgi/testPost.php'>\n";
echo "<label for='name'>Name:</label><br>\n";
echo "<input type='text' id='name' name='name' value='' required><br>\n";
echo "<label for='age'>Age:</label><br>\n";
echo "<input type='number' id='age' name='age' value='' required><br><br>\n";
echo "<input type='submit' value='Submit'>\n";
echo "</form>\n";

echo "</body>\n";
echo "</html>\n";

// for test enter data to form with  url http://localhost:9999/contact-us/Home.php
?>