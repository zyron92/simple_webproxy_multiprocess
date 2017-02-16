#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#include "socket_helper.h"
#include "http_handler.h"

/*
 * Kill all zombie children
 */
void handle_sigchld(int sig);

/*
 * Logging for Proxy Server
 */
int logging(char *clientIpAdd, int clientPortNum, HttpPacket *packet);

int main()
{
	//STEP 0 : Preparing the proxy server and client information
	int server_fd = server_listening();
	if(server_fd == ERR){
		fprintf(stderr, "ERROR: failed to establish a server socket\n");
		return ERR;
	}
	int client_fd;
	struct sockaddr_in clientAddress;
	socklen_t addLength = (socklen_t)(sizeof(struct sockaddr_in));
	char *client_ipAddress;
	int client_portNumber=0;
	int proxy_fd;

	//Reaping zoombie processes
	signal(SIGCHLD, handle_sigchld);

        while(1){
		//STEP 1 : Accepting a new request from the client
		client_fd = accept(server_fd, 
				   (struct sockaddr *)(&clientAddress), 
				   &addLength);
		if(client_fd < 0){
			fprintf(stderr, "ERROR: unable to accept a new \
connection request. Retry from the beginning!\n");
			continue;
		}

		/*
		 * Child process do all the jobs
		 */
		if(fork() == 0){
			//Keep only one binding at anytime
			close(server_fd);

			//STEP 2 : Receiving client HTTP request
			HttpPacket *packet;
			char *remainingPacket;
			if(get_http_request(client_fd, &packet, 
					    &remainingPacket) == ERR){
				fprintf(stderr, "ERROR: failed to receive \
HTTP request. Retry from the beginning!\n");
				close(client_fd);
				exit(OK);
			}

			//STEP 3 : Creating a new client socket for forwarding
			proxy_fd = server_forwarding_request(packet);
			if(proxy_fd == ERR){
				fprintf(stderr, "ERROR: failed to establish \
a client socket for forwarding. Retry from the beginning!\n");
				free_http_packet(packet);
				if(remainingPacket != NULL){
					free(remainingPacket);
				}
				close(client_fd);
				exit(OK);
			}

			//STEP 4 : Forwarding request
			if(forward_http_request(proxy_fd, packet, 
						remainingPacket) == ERR){
				fprintf(stderr, "ERROR: failed to forward \
request. Retry from the beginning!\n");
				free_http_packet(packet);
				if(remainingPacket != NULL){
					free(remainingPacket);
				}
				close(client_fd);
				close(proxy_fd);
				exit(OK);
			}
			if(remainingPacket != NULL){
				free(remainingPacket);
			}

			//STEP 5 : Receiving and forwarding reply
			if(get_forward_http_reply(proxy_fd, client_fd) == ERR){
				fprintf(stderr, "ERROR: failed to get and \
forward reply. Retry from the beginning!\n");
				free_http_packet(packet);
				close(client_fd);
				close(proxy_fd);
				exit(OK);
			}

			//STEP 6 : Getting info and Logging
			//Get client info
			if(get_connected_client_info(&clientAddress, 
						     &client_ipAddress, 
						     &client_portNumber) == OK){

				//Do the logging process
				if(logging(client_ipAddress, client_portNumber,
					   packet)== ERR){
					fprintf(stderr, "ERROR: logging failed\
 for the current request!\n");
				}

			}else{
				fprintf(stderr, "ERROR: logging failed for \
the current request!\n");
			}

			//STEP 7 : Free and close all the related sockets
			free_http_packet(packet);
			close(client_fd);
			close(proxy_fd);
			exit(OK);
		}

		/*
		 * Parent process closes its connected socket to open freshly
		 * new one later
		 */
		close(client_fd);
	}

	close(server_fd);
	
	return OK;
}

/*
 * Kill zombies children
 * (provided from the lecture)
 */
void handle_sigchld(int sig)
{
	while(waitpid(-1, 0, WNOHANG) > 0);
	return;
}

/*
 * Logging for Proxy Server
 */
int logging(char *clientIpAdd, int clientPortNum, HttpPacket *packet)
{
	if(packet == NULL)
	{
		fprintf(stderr, "ERROR: zero information to log\n");
		return ERR;
	}

	//Set up file lock
	struct flock lock;
	memset (&lock, 0, sizeof(lock));

	int file_fd = open("proxy.log", O_CREAT | O_RDWR, 0777);
	if(file_fd == -1){
		fprintf(stderr, "ERROR: failed to create/open proxy.log\n");
		return ERR;
	}

	//Lock the file
	lock.l_type = F_WRLCK;
	fcntl (file_fd, F_SETLKW, &lock);

	//----------------- START LOCKING AREA ----------------------------//

	//To find the numbering
	unsigned int line_number = 0;
	unsigned int number_of_lines = 0;
	char readCharacter;
	int readByte;
	while((readByte = read(file_fd, &readCharacter, 1))!=0) {
		if(readByte==1 && readCharacter=='\n'){
			number_of_lines++;
		}
	}
	line_number = number_of_lines + 1;
	
	//Converting line number to string format, max line number is a number
	//with 100 digits
	char line_number_string[100];
	memset(line_number_string, 0, 100);
	sprintf(line_number_string, "%u", line_number);

	/*
	 * Writing the information of the request
	 */
	if((line_number_string == NULL) | (clientIpAdd == NULL) || 
	   (packet == NULL) || (packet->ipAddress == NULL) || 
	   (packet->portNum == NULL) || (packet->action == NULL) || 
	   (packet->page == NULL)){
		fprintf(stderr, "ERROR: failed to read info for logging\n");
		return ERR;
	}
	//Numbering & client ip address
	write(file_fd, line_number_string, strlen(line_number_string));
	write(file_fd, " ", 1); 
	write(file_fd, clientIpAdd, strlen(clientIpAdd)); 
	write(file_fd, ":", 1); 
	//Client port number (from int to string)
	char client_portNum_string[6];
	memset(client_portNum_string, 0, 6);
	sprintf(client_portNum_string, "%u", clientPortNum);
	write(file_fd, client_portNum_string, strlen(client_portNum_string));
	//Server & action info
	write(file_fd, " ", 1); 
	write(file_fd, packet->ipAddress, strlen(packet->ipAddress));
	write(file_fd, ":", 1); 
	write(file_fd, packet->portNum, strlen(packet->portNum));
	write(file_fd, " ", 1); 
	write(file_fd, packet->action, strlen(packet->action)); 
	write(file_fd, " ", 1); 
	write(file_fd, packet->page, strlen(packet->page));
	write(file_fd, "\n", 1);
	
	//----------------- END LOCKING AREA ----------------------------//

	//Unlock the file
	lock.l_type = F_UNLCK;
	fcntl(file_fd, F_SETLKW, &lock);

	close(file_fd);

	return OK;
}
