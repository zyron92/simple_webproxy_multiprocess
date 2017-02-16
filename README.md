#Simple Webproxy Multiprocess
Simple Webproxy Multi-process, Non-blocking I/O

##
###Preliminaries
csapp module (csapp.c & csapp.h) is borrowed from the [page](http://csapp.cs.cmu.edu/public/code.html) of Bryant and O'Hallaron's Computer Systems: A Programmer's Perspective.

##
###Simple Descriptions
- A simple web proxy multi-process to handle all clients simultaneously. 
- The proxy port by default is 12345. 
- There is a log file (using flock) to keep track of each request of cients (count client_IP:port server_IP:port client_request)

##
###Detailed Descriptions
1. How to use :
  1. Compile and run (make run). [make help for more info]
  2. Launch the client programs via socket-executable, browser, or telnet-terminal; and connect it to the proxy server which is binded to "0.0.0.0:12345".
  3. Send the HTTP request GET header terminated by "\r\n" ("enter" in telnet).
    1. "http://" may or may not be included in the GET request.
    2. the format need to be respected.(the capital letters respected and no extra spaces even)
  4. Send other HTTP request headers such as "Host:, Connection-Alive etc" but these extra headers will be ignored (and be only forwarded) by proxy server.
  5. Finish the request with an empty line terminated by "\r\n" .
  6. Check the proxy.log for details of successful connections.
    1. Unreachable webserver considered as unsuccessful connection as the proxy server will stop the job and close the connection.
    2. Connection which produces an error of "Page not found" will be considered as successful connection as the whole process succeeded but returns an error "page not found" to the client which is still a page.

2. Conception of proxy server:
  1. Proxy server will run continuosly until a control-c detected.
  2. It accepts HTTP requests from different clients in a concurrent way.
  3. It then creates a new client socket to relay the packet to the requested web server and receives the reply, before forwarding the reply to the client.
  4. The child process will do all the main job for one time and then exits, while the parent process will wait for new connection request every iteration.
  5. In case of any error encountered in any part of the main job, child process will stop and exit immediately (without continuing the rest of the job).
  6. In case of error encountered in logging part, this part will be skipped and the rest of the job will be continued.
  7. A data structure called HttpPacket is implemented to store the http request GET header from the client. (domain name can be ip address if clients initiate connections with only ip address)
  8. The log file (proxy.log) is accessed by concurent proccesses in a secure way using the mecanism of file lock (fcntl).
  9. Numbering of the log file is determined by counting previous written lines as we suppose that log information will never be destroyed, or else we may have duplicate number for numbering.

3. Other elements to be considered :
  1. Codes are divided into 3 parts : main program, http handler and socket handler.
    1. main program : contains the principal loop of continuous routine of handling theclient and does all the job, besides the logging part.
    2. http handler : contains all the functions and data structures related to handling http request, reply and forward.
    3. socket handler : contains all the functions to create a new server listening socket, a new client socket (for proxy to connect to web server), to write the packets onto socket and to get informations of connected client socket.
  2. Basic error / exception handlers are implemented (Unreachable web server, Web server not accepting any request, etc)

##
There are still bugs to be resolved, and improvements to be done.
