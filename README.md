# What is this
This is our http webserver, we built a lightweight HTTP server from scratch that can handle multiple connections at once.

# Tech stack we used to build the webserver
- C/C++
- Epoll API for handling many connections without creating a thread for each one
- HTTP protocol implementation

# How this cool webserver work :D
- clone the repo <b>(obviously)</b>
- compile the source code using 'make'
- run ```./webserver webserv.conf```
- here is how you can define the configuration file:
```
SERVER = [
    port = 9999, 7331;
    host = 127.0.0.1;
    allowed_methods = GET, POST, DELETE;
    max_body_size = 880803840;
    server_names = www.enginx.com, www.enginx.ma;
    error_pages = 500:www/html/500.html, 404:www/html/404.html;
    
    route = / : ROOT=www/html, DEFAULT_FILE=index.html...;
    route = /contact-us : ROOT=www/html/contact-us...;
]
```
### simple, right? it does the job without any unnecessary complexity.
- You can create multiple server blocks for different configurations
- You can assign additional rules to a route by simply append
  the rule KEY to the end of the route line. (take a look at the given configuration file, maybe you can understand)  


# Benchmark Results
### I tested the server performance using <b>wrk<b> tool and here is the result:

![image](https://github.com/user-attachments/assets/48b66360-4fc5-410d-b934-445ffda145e6)

As u can see
- Requests/sec: 16789.31
- Transfer/sec: 25.59MB
- Over 1 million requests processed in 1 minute
- No connection errors (100% availability)

### if you liked this project, give it a ⭐
