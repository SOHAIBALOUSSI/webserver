<?php
header("Content-Type: text/html");
$name = isset($_POST['name']) ? htmlspecialchars($_POST['name']) : "Guest";
$age = isset($_POST['age']) ? htmlspecialchars($_POST['age']) : "Unknown";
echo "Hello, $name!\nYour age is: $age\n";
?>