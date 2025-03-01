#!/usr/bin/env python3

import cgi

# Set content type header
print("Content-Type: text/html\n")  # The '\n' separates headers from the body

# Parse form data
form = cgi.FieldStorage()

# Get data from form fields
name = form.getvalue("name", "Unknown")  # Default to "Unknown" if no input

print(f"<p>Hello, {name}!</p>")

print("Hello from Python!")