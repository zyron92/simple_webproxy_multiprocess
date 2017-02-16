#ifndef __SOCKET_HELPER_H__
#define __SOCKET_HELPER_H__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "csapp.h"

#define ERR -1 //Error
#define OK 0 //Success

#define MAX_BUFFER 512

//typedef struct HttpPacket HttpPacket;

/*
 * Creating a socket, binding it to localhost:12345 and preparing it
 */
int server_listening(void);

/*
 * Creating a socket, and connecting it to ipAddress:portNum
 */
int server_forwarding(const char *ipAddress, int portNum);

/*
 * Reading bytes from a rio reading descriptor (with internal buffer) for 
 * a single packet / line (header) terminated with '\r\n'
 * - Requires initialised rio reading descriptor
 * - Blocking reading
 * - Ended with '\0'
 */
int read_http_packet(rio_t *readDescriptor, char **readPacket);

/*
 * Writing bytes to a file descriptor
 */
int write_packet(int output_fd, char *packetToWrite, unsigned int sizePacket);

/*
 * Getting client socket information
 */
int get_connected_client_info(struct sockaddr_in *client_socket_info, 
			      char **ipAddress, int *portNum);

/*
 * Return the ip address after the resolution of domain name if it is a domain
 * name, or else return itself if it is already an ip address
 *
 * Free is required
 */
char * resolve_domain_name(const char *domainName);

#endif
