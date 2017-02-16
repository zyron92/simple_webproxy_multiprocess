#include "socket_helper.h"

/*
 * Creating a server socket, binding it to 0.0.0.0:12345 and preparing it
 * Binding to all interfaces with the port 12345
 */
int server_listening(void)
{	//STEP 1 : Creating a socket descriptor for the server
	int resultSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(resultSocket < 0){
		fprintf(stderr, "ERROR: failed to establish a server socket \
[socket()]\n");
		return ERR;
	}

	//Removing the binding error for a faster debugging
	int optValue = 1;
	if(setsockopt(resultSocket, SOL_SOCKET, SO_REUSEADDR, 
		      (const void *)(&optValue), sizeof(int)) < 0){
		fprintf(stderr, "ERROR: failed to establish a server socket \
[setsockopt()]\n");
		return ERR;
	}

	//STEP 2 : Binding process
	//we use calloc to initialize the structure with zeros
	struct sockaddr_in *serverAddress = calloc(1, 
						   sizeof(struct sockaddr_in));
	serverAddress->sin_family = AF_INET;
	serverAddress->sin_port = htons(12345);
	inet_pton(AF_INET, "0.0.0.0", &(serverAddress->sin_addr));
	if(bind(resultSocket, (struct sockaddr *)(serverAddress), 
		sizeof(struct sockaddr_in)) < 0){
		free(serverAddress);
		fprintf(stderr, "ERROR: failed to establish a server socket \
[bind()]\n");
		return ERR;
	}

	//STEP 3 : Making the socket ready to accept incomming connection
	if(listen(resultSocket, 20) < 0){ //20 clients accepted in the queue
		free(serverAddress);
		fprintf(stderr, "ERROR: failed to establish a server socket \
[listen()]\n");
		return ERR;
	}

	free(serverAddress);
	return resultSocket;
}

/*
 * Creating a client socket, and connecting it to ipAddress:portNum
 */
int server_forwarding(const char *ipAddress, int portNum)
{
	//STEP 1 : Creating a socket descriptor for the client
	int resultSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(resultSocket < 0){
		fprintf(stderr, "ERROR: failed to establish a client socket \
[socket()]\n");
		return ERR;
	}

	//STEP 2 : Connecting process to the server
	//we use calloc to initialize the structure with zeros
	struct sockaddr_in *serverAddress = calloc(1, 
						   sizeof(struct sockaddr_in));
	serverAddress->sin_family = AF_INET;
	serverAddress->sin_port = htons(portNum);
	inet_pton(AF_INET, ipAddress, &(serverAddress->sin_addr));
	if(connect(resultSocket, (struct sockaddr *)(serverAddress), 
		   sizeof(struct sockaddr_in)) < 0){
		free(serverAddress);
		fprintf(stderr, "ERROR: failed to establish a client socket \
[bind()]\n");
		return ERR;
	}

	free(serverAddress);
	return resultSocket;
}

/*
 * Reading bytes from a rio reading descriptor (with internal buffer) for 
 * a single packet / line (header) terminated with '\r\n'
 * - Requires initialised rio reading descriptor
 * - Blocking reading
 * - Ended with '\0'
 */
int read_http_packet(rio_t *readDescriptor, char **readPacket)
{
	//Values by default
	*readPacket = NULL;

	//Temporary buffer
	char *buffer = calloc(MAX_BUFFER, sizeof(char));
	size_t numReadBytes = 0;

	//Final buffer
	char *finalBuffer = NULL;
	size_t totalNumReadBytes = 0;

	//Reading from the socket until we find '\r\n'
	do{
		numReadBytes = Rio_readlineb(readDescriptor, buffer, 
					     MAX_BUFFER);
		if(numReadBytes > 0){
			//Remove the '\0' added by Rio_readlineb
			if(*(buffer+numReadBytes-1)=='\0'){
				numReadBytes--;
			}

			//we accumulate the received bytes into the final buffer
			//to be delivered
			totalNumReadBytes += numReadBytes;
			finalBuffer = realloc(finalBuffer, 
					      sizeof(char)*(
						      totalNumReadBytes+1));
			memcpy(finalBuffer+totalNumReadBytes-numReadBytes, 
			       buffer, numReadBytes);

			//Adding the nul character for the string
			*(finalBuffer+totalNumReadBytes) = '\0';
		}
	}while(((int)(numReadBytes) == -1) || 
	       ((numReadBytes > 1) && (*(buffer+numReadBytes-2) != '\r') 
		&& (*(buffer+numReadBytes-1) != '\n')));
		
	free(buffer);

	//Delivering the read packet 
	*readPacket = finalBuffer;

	return OK;
}

/*
 * Writing bytes to a file descriptor
 */
int write_packet(int output_fd, char *packetToWrite, unsigned int sizePacket)
{
	if(packetToWrite == NULL){
		fprintf(stderr, "ERROR: zero byte to write\n");
		return ERR;
	}

	if(sizePacket != rio_writen(output_fd, packetToWrite, sizePacket)){
		fprintf(stderr, "ERROR: writing to a socket failed\n");
		return ERR;
	}

	return OK;
}

/*
 * Getting client socket information
 */
int get_connected_client_info(struct sockaddr_in *client_socket_info, 
			      char **ipAddress, int *portNum)
{
	if(client_socket_info == NULL){
		fprintf(stderr, "ERROR: failed to get client socket info\n");
		return ERR;
	}

	*ipAddress = inet_ntoa(client_socket_info->sin_addr);
	*portNum = ntohs(client_socket_info->sin_port);

	return OK;
}

/*
 * Return the ip address after the resolution of domain name if it is a domain
 * name, or else return itself if it is already an ip address
 *
 * Free is required
 */
char * resolve_domain_name(const char *domainName)
{
	if(domainName == NULL){
		return NULL;
	}

	char *ipAddress;

	struct in_addr address;
	//If it is an ip address, we just send a copy of itself
	if(inet_aton(domainName, &address) != 0){
		ipAddress = calloc(strlen(domainName)+1, sizeof(char));
		strncpy(ipAddress, domainName, strlen(domainName));
	}
	//Or if it is a domain name, we try to resolve it
	else{
		//Resolution of domain name
		struct hostent *hostInfo;
		hostInfo = gethostbyname(domainName);
		if(hostInfo == NULL){
			fprintf(stderr, "ERROR1: resolution of domain name \
failed!\n");
			return NULL;
		}
	        
		if(hostInfo->h_addr_list == NULL){
			fprintf(stderr, "ERROR2: resolution of domain name \
failed!\n");
			return NULL;
		}
		
		//we take the first answer of ip address answer corresponding
		//to the domain name and send it as a string
		address.s_addr = ((struct in_addr *)
				  (hostInfo->h_addr_list[0]))->s_addr;
		char *ipAddressTemp = inet_ntoa(address);
		int ipAddressLength = strlen(ipAddressTemp);
		ipAddress = calloc(ipAddressLength+1, sizeof(char));
		strncpy(ipAddress, ipAddressTemp, ipAddressLength);
	}
	
	return ipAddress;
}
