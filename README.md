#Simple Webproxy Multiprocess
Simple Webproxy Multi-process, Non-blocking I/O

##
###Preliminaries
csapp module (csapp.c & csapp.h) is borrowed from the [page](http://csapp.cs.cmu.edu/public/code.html) of Bryant and O'Hallaron's Computer Systems: A Programmer's Perspective.

##
###Descriptions
- A simple web proxy multi-process to handle all clients simultaneously. 
- The proxy port by default is 12345. 
- There is a log file (using flock) to keep track of each request of cients (count client_IP:port server_IP:port client_request)

##
There are still bugs to be resolved, and improvements to be done.
