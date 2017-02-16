#ifndef __HTTP_HANDLER_H__
#define __HTTP_HANDLER_H__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "socket_helper.h"
#include "csapp.h"

//Data structure of HTTP Packet (first header)
typedef struct _httpPacket{
	//the domain name could be the same as ip address 
	//if we don't have the information of the domain name
	char *domainName;
	char *ipAddress;
	char *portNum;
	char *action;
	char *page;
	char *version;
} HttpPacket;

/*
 * Extraction of informations from the url
 * + Resolution of domain name
 *
 * "free" is required
 */
int url_parser(char *url, char **domainName, char **ipAddress, char **page, 
	       char **portNum);
/*
 * Initialisation of HTTP Packet
 *
 * "free" is required
 */
int http_packet_parser(const char *httpPacket, HttpPacket **packet);

/*
 * Free-ing memory for http packet
 */
void free_http_packet(HttpPacket *packet);

/*
 * Showing details of http packet for debugging
 */
void show_http_packet(HttpPacket *packet);

/*
 * Getting the firstline of http request header from the client and put it into
 * the defined data structure, while keeping the rest of http packet request 
 * header for later forwarding
 */
int get_http_request(int input_fd, HttpPacket **packet, char **remainingPacket);

/*
 * Creating a socket, and connecting it to ipAddress:portNum based on the
 * packet http headers received
 */
int server_forwarding_request(HttpPacket *packet);

/*
 * Forwarding the firstline of http request header from the client
 * and the rest of headers
 */
int forward_http_request(int output_fd, HttpPacket *packet, 
			 char *remainingPacket);

/*
 * Receiving and forwarding the http reply data
 */
int get_forward_http_reply(int input_fd, int output_fd);

#endif
