#include "http_handler.h"

/*
 * Extraction of informations from the url
 * + Resolution of domain name
 *
 * "free" is required
 */
int url_parser(char *url, char **domainName, char **ipAddress, char **page, 
	       char **portNum)
{
	//Informations by default
	*domainName = NULL;
	*page = NULL;
	*portNum = NULL;
	int httpMarkerExist = 0;
	char *urlAfterHttpMarker = url;

	if(url == NULL){
		fprintf(stderr, "ERROR: no url detected\n");
		return ERR;
	}

	/*
	 * Verification if there is "http://"
	 */
	unsigned int urlLength = (unsigned int)(strlen(url));
	if((urlLength>=7) && (strncmp(url, "http://", 7)==0)){
		urlAfterHttpMarker = url+7;
		httpMarkerExist = 1;
	}

	/*
	 * Information on the page
	 */
	char *afterFirstSeparator = strstr(urlAfterHttpMarker, "/");
	unsigned int afterFirstSepLength;
	if(afterFirstSeparator!= NULL){
		afterFirstSepLength = strlen(afterFirstSeparator);
		*page = calloc(afterFirstSepLength+1, sizeof(char));
		strncpy(*page, afterFirstSeparator, afterFirstSepLength);
	}else{
		afterFirstSepLength = 0;
		*page = calloc(2, sizeof(char));
		(*page)[0] = '/'; //page by default
	}
	
	/*
	 * Information on the domain name
	 */
	unsigned int domainNameLength;
	if(httpMarkerExist){
		domainNameLength =  urlLength - 7 - afterFirstSepLength;
	}else{
		domainNameLength =  urlLength - afterFirstSepLength;
	}
	if(domainNameLength == 0){
		free(*page);
		*page = NULL;
		fprintf(stderr, "ERROR: invalid domain name in the url\n");
		return ERR;
	}
	*domainName = calloc(domainNameLength+1, sizeof(char));
	strncpy(*domainName, urlAfterHttpMarker, domainNameLength);

	/*
	 * Information on the port number
	 */
	char *ptrPortNumber = strstr(*domainName, ":");
	unsigned int portNumberLength;
	if(ptrPortNumber == NULL){
		portNumberLength = 0;
		*portNum = calloc(3, sizeof(char));
		strncpy(*portNum, "80", 2);
	}else{
		portNumberLength = strlen(ptrPortNumber);
		*portNum = calloc(portNumberLength, sizeof(char));
		strncpy(*portNum, ptrPortNumber+1, portNumberLength-1);
		
		//Reconstruction of domain name (without the port number)
		free(*domainName);
		*domainName = calloc(domainNameLength+1-portNumberLength, 
				     sizeof(char));
		strncpy(*domainName, urlAfterHttpMarker, 
			domainNameLength-portNumberLength);
	}

	/*
	 * Information on the ip address of the domain name
	 */
	*ipAddress = resolve_domain_name(*domainName);
	if(*ipAddress == NULL){
		free(*page);
		*page = NULL;
		free(*domainName);
		*domainName = NULL;
		free(*portNum);
		*portNum = NULL;
		fprintf(stderr, "ERROR: failed to do the resolution of \
domain name in the url\n");
		return ERR;
	}

	return OK;
}

/*
 * Initialisation of HTTP Packet
 *
 * "free" is required
 */
int http_packet_parser(const char *httpPacket, HttpPacket **packet)
{
	*packet = NULL;

	if(httpPacket == NULL){
		fprintf(stderr, "ERROR: no packet detected\n");
		return ERR;
	}
								       
	/*
	 * Verification of the action
	 */
	unsigned int httpPacketLength = (unsigned int)(strlen(httpPacket));
	if((httpPacketLength<=3) || (strncmp(httpPacket, "GET", 3)!=0)){
		fprintf(stderr, "ERROR: Invalid http packet, only action GET allowed\n");
		return ERR;
	}
	
	/*
	 * --- START of Verification of the url and the version http ---
	 */

	//URL
	char *afterFirstSeparator = strstr(httpPacket+3, " ");
	if(afterFirstSeparator== NULL){
		fprintf(stderr, "ERROR: Invalid http packet, not enough parameters\n");
		return ERR;
	}
	
	afterFirstSeparator++;
	if(afterFirstSeparator== NULL){
		fprintf(stderr, "ERROR: Invalid http packet, not enough parameters\n");
		return ERR;
	}

	//Version HTTP
	char *afterSecondSeparator = strstr(afterFirstSeparator, " ");
	if(afterSecondSeparator== NULL){
		fprintf(stderr, "ERROR: Invalid http packet, not enough parameters\n");
		return ERR;
	}
	
	afterSecondSeparator++;
	if(afterSecondSeparator== NULL){
		fprintf(stderr, "ERROR: Invalid http packet, not enough parameters\n");
		return ERR;
	}
	
	//Version HTTP
	unsigned int versionLength = strlen(afterSecondSeparator);
	if((versionLength<8) || (strncmp(afterSecondSeparator, "HTTP/", 5)!=0)){
		fprintf(stderr, "ERROR: Invalid version\n");
		return ERR;
	}

	//URL
	unsigned int urlLength = strlen(afterFirstSeparator) - 
		strlen(afterSecondSeparator) - 1;
	if(urlLength == 0){
		fprintf(stderr, "ERROR: Invalid URL\n");
		return ERR;
	}

	//Local copy of url
	char *url = calloc(urlLength+1, sizeof(char));
	strncpy(url, afterFirstSeparator, urlLength);

	//Extraction of important info from URL
	char *domainName;
	char *ipAddress;
	char *page;
	char *portNum;
	if(url_parser(url, &domainName, &ipAddress, &page, &portNum) == ERR){
		fprintf(stderr, "ERROR: URL parser\n");
		free(url);
		return ERR;
	}
	free(url);

	/*
	 *  --- END of Verification of the url and the version http ---
	 */

	/*
	 * Initialisation of http packet data structure
	 */
	*packet = calloc(1, sizeof(HttpPacket));
	
	(*packet)->action = calloc(4, sizeof(char));
	strncpy((*packet)->action, httpPacket, 3);

	(*packet)->version = calloc(versionLength+1, sizeof(char));
	strncpy((*packet)->version, afterSecondSeparator, versionLength);

	(*packet)->domainName = domainName;
	(*packet)->ipAddress = ipAddress;
	(*packet)->page = page;
	(*packet)->portNum = portNum;

	return OK;
}

/*
 * Free-ing memory for http packet
 */
void free_http_packet(HttpPacket *packet)
{
	if(packet==NULL){
		return;
	}

	if(packet->action != NULL){
		free(packet->action);
	}

	if(packet->version != NULL){
		free(packet->version);
	}

	if(packet->domainName != NULL){
		free(packet->domainName);
	}

	if(packet->ipAddress != NULL){
		free(packet->ipAddress);
	}

	if(packet->page != NULL){
		free(packet->page);
	}

	if(packet->portNum != NULL){
		free(packet->portNum);
	}

	free(packet);
}

/*
 * Getting the firstline of http request header from the client and put it into
 * the defined data structure, while keeping the rest of http packet request 
 * header for later forwarding
 */
int get_http_request(int input_fd, HttpPacket **packet, char **remainingPacket)
{
	*packet = NULL;
	*remainingPacket = NULL;

	//Initialisation of rio reading descriptor
	rio_t readDescriptor;
	rio_readinitb(&readDescriptor, input_fd);

	//reading the first packet / line (header)
	char *receivedBytes;
	read_http_packet(&readDescriptor, &receivedBytes);
	if(receivedBytes == NULL){
		fprintf(stderr, "ERROR: failed to read first line of http \
request header!\n");
		return ERR;
	}

	//reading the following packets / lines (headers) until an empty line 
	//terminated by '\r\n' detected (to indicate end of request), 
	//and these packets will be just forwarded to the server
	char *nextReceivedBytes;
	char *nextEndPacket;
	unsigned int sizeNextPacket=0;
	unsigned int numReadBytes = 0;
	unsigned int totalNumReadBytes = 0;
	do{
	        //read one http packet / line (header)
	        read_http_packet(&readDescriptor, &nextReceivedBytes);
		if(nextReceivedBytes == NULL){
			break; //no more packet to be read
		}
		
		//keeping of the remaining of the packet (header) 
		//for later forwarding
		numReadBytes = strlen(nextReceivedBytes);
		if(numReadBytes > 0){
			totalNumReadBytes += numReadBytes;
			*remainingPacket = realloc(*remainingPacket, 
						   sizeof(char)*
						   (totalNumReadBytes+1));
			memcpy((*remainingPacket)+totalNumReadBytes
			       -numReadBytes, nextReceivedBytes, numReadBytes);
			*((*remainingPacket)+totalNumReadBytes) = '\0';
		}
	        
                //finding the end marker of http packet/ line
		//in order to find the information without this marker
		nextEndPacket = strstr(nextReceivedBytes,"\r\n");
		
		//Compute the size of the packet without the end marker
		sizeNextPacket = 0;
		if(nextEndPacket != NULL){
			sizeNextPacket = strlen(nextReceivedBytes) - 
				strlen(nextEndPacket);
		}
		if(nextReceivedBytes != NULL){
			free(nextReceivedBytes);
		}
	}while((nextEndPacket == NULL) || (sizeNextPacket != 0));

	//the end marker of the first packet / line
	char *endPacket = strstr(receivedBytes,"\r\n");
	if(endPacket == NULL){
		fprintf(stderr, "ERROR: end delimiter of http packet not found\
\n");
	        if((*remainingPacket) != NULL){
			free(*remainingPacket);
		}
		free(receivedBytes);
		return ERR;
	}
	unsigned int endPacketLength = strlen(endPacket);

	//The first packet length that we are interested in
	unsigned int packetLength = strlen(receivedBytes) - endPacketLength;
	
	//Local copy of first packet bytes
	char *httpPacketBytes = calloc(packetLength+1, sizeof(char));
	strncpy(httpPacketBytes, receivedBytes, packetLength);
	free(receivedBytes);

	//Initialisation of our data structure from the received bytes
	//from the first packet
	if(http_packet_parser(httpPacketBytes, packet) == ERR){
		fprintf(stderr, "ERROR: http packet parser failed\n");
		if((*remainingPacket) != NULL){
			free(*remainingPacket);
		}
		free(httpPacketBytes);
		return ERR;
	}
	free(httpPacketBytes);

	return OK;
}

/*
 * Creating a socket, and connecting it to ipAddress:portNum based on the
 * packet http headers received
 */
int server_forwarding_request(HttpPacket *packet)
{
	//Verification of the packet http header for forwarding details
	if(packet == NULL){
		fprintf(stderr, "ERROR: packet can not be read for information \
regarding forwarding\n");
		return ERR;
	}
	if((packet->ipAddress == NULL) || (packet->portNum == NULL)){
		fprintf(stderr, "ERROR: packet can not be read for information \
regarding forwarding\n");
		return ERR;
	}

	//Information of the server for the client socket to be created
	char *ipAddress = packet->ipAddress;
	char *endPtr;
	int portNum = (int)(strtol(packet->portNum, &endPtr, 10));
	if(*endPtr != '\0'){
		fprintf(stderr, "ERROR: invalid port number\n");
		return ERR;
	}
	if((portNum == 0) || (portNum > 65535)){
		fprintf(stderr, "ERROR: invalid port number \
(0<port number<=65535)\n");
		return ERR;
	}

	//Creation of the client socket
	return server_forwarding(ipAddress, portNum);
}

/*
 * Forwarding the firstline of http request header from the client
 * and the rest of headers
 */
int forward_http_request(int output_fd, HttpPacket *packet, 
			 char *remainingPacket)
{
	//Verification of packet header
	if(packet == NULL){
		fprintf(stderr, "ERROR: invalid packet to write\n");
		return ERR;
	}
	if((packet->action == NULL) || (packet->version == NULL) || 
	   (packet->domainName == NULL) ||  (packet->page == NULL)){
		fprintf(stderr, "ERROR: invalid packet to write\n");
		return ERR;
	}

	//Information on size of diff. elements of the packet header
	unsigned int actionLength = strlen(packet->action);
	unsigned int pageLength = strlen(packet->page);
	unsigned int versionLength = strlen(packet->version);
	unsigned int totalRequestLength = actionLength + 1 + pageLength 
		+ 1 + versionLength + 2;

	//Creating the output bytes for the main header
	char *requestBytes = calloc(totalRequestLength, sizeof(char));
	strncpy(requestBytes, packet->action, actionLength);
	strncpy(requestBytes+actionLength, " ", 1);
	strncpy(requestBytes+actionLength+1, packet->page, pageLength);
	strncpy(requestBytes+actionLength+1+pageLength, " ", 1);
	strncpy(requestBytes+actionLength+1+pageLength+1, 
		packet->version, versionLength);
	strncpy(requestBytes+actionLength+1+pageLength+1+versionLength, 
		"\r\n", 2);
	
	//Writing the bytes of the main header onto the output
	if(write_packet(output_fd, requestBytes, totalRequestLength) == ERR){
		fprintf(stderr, "ERROR: writing to a socket failed\n");
		free(requestBytes);
		return ERR;
	}
	free(requestBytes);

	/*
	 * Writing the remaining bytes of http request header 
	 * including the empty line
	 */
	//first case : if no other header or the other header being only 
	//empty line, we include an extra header "host"
	if((remainingPacket == NULL) || 
	   ((remainingPacket != NULL) && (strlen(remainingPacket)>=2) &&
	    (strncmp(remainingPacket, "\r\n", 2)==0))){
		unsigned int domainNameLength = strlen(packet->domainName);
		char *requestBytes2 = calloc(6+domainNameLength+4, 
					     sizeof(char));
		strncpy(requestBytes2, "Host: ", 6);
		strncpy(requestBytes2+6, packet->domainName, domainNameLength);
		strncpy(requestBytes2+6+domainNameLength, "\r\n\r\n", 4);
		if(write_packet(output_fd, requestBytes2, 6+domainNameLength+4)
		   == ERR){
			fprintf(stderr, "ERROR: writing to a socket failed\n");
			free(requestBytes2);
			return ERR;
		}
		free(requestBytes2);
	}
	//other case : we just forward the remaining headers 
	else{
		if(write_packet(output_fd, remainingPacket, 
				(unsigned int)strlen(remainingPacket)) == ERR){
			fprintf(stderr, "ERROR: writing to a socket failed\n");
			return ERR;
		}
	}

	return OK;
}

/*
 * Finding the content length from a http packet / line
 */
static unsigned int find_content_length(char *http_line, 
					unsigned int realSizeLine,
					int *contentLengthFound)
{
	if(http_line == NULL){
		return 0;
	}

	unsigned int contentLength = 0;
	char *endPtr;
	if(realSizeLine > 16){
		if(strncmp(http_line, "Content-Length: ", 16) == 0){;
			//Local copy
			char contentLengthString[realSizeLine-16+1];
			strncpy(contentLengthString,http_line+16,
				realSizeLine-16);
			contentLengthString[realSizeLine-16] = '\0';
			
			//Conversion
			contentLength = (unsigned int)
				(strtol(contentLengthString, &endPtr, 10));
			if(*endPtr != '\0'){
				fprintf(stderr, 
					"ERROR: invalid http content length\n");
				return 0;
			}
			*contentLengthFound = 1; //content length found
		} 
	}

	return contentLength;
}

/*
 * Receiving and forwarding the http reply data
 */
int get_forward_http_reply(int input_fd, int output_fd)
{
	//Initialisation of rio reading descriptor
	rio_t readDescriptor;
	rio_readinitb(&readDescriptor, input_fd);

	//reading and writing the packets / lines until an empty line 
	//terminated by '\r\n' detected (to indicate end of request), 
	char *nextReceivedBytes;
	char *nextEndPacket;
	unsigned int sizeNextPacket=0;
	int statusWrite;
	unsigned int contentLength = 0;
	int contentLengthFound = 0;
	do{
		//read one http packet / line
	        read_http_packet(&readDescriptor, &nextReceivedBytes);
		if(nextReceivedBytes == NULL){
			break; //no more packet to be read
		}

                //finding the end marker of http packet/ line
		//in order to find the information without this marker
		nextEndPacket = strstr(nextReceivedBytes,"\r\n");
		sizeNextPacket = 0;
		if(nextEndPacket != NULL){
			sizeNextPacket = strlen(nextReceivedBytes) - 
				strlen(nextEndPacket);
		}

		//finding the content length if it exists
		if(!contentLengthFound){
			contentLength = find_content_length(nextReceivedBytes, 
							    sizeNextPacket,
							    &contentLengthFound);
		}

		//write one http packet / line until it succeeds
		do{
			statusWrite = write_packet(output_fd, 
						   nextReceivedBytes, 
						   strlen(nextReceivedBytes));
		}while(statusWrite != OK);
		free(nextReceivedBytes);
	}while((nextEndPacket == NULL) || (sizeNextPacket != 0));

	if(contentLengthFound){
		char http_contents[contentLength];
		ssize_t numReadBytes = 0;
		numReadBytes = Rio_readnb(&readDescriptor, http_contents, 
					  contentLength);
		if(numReadBytes > 0){
			do{
				statusWrite = write_packet(output_fd, 
							   http_contents, 
							   (unsigned int)
							   numReadBytes);
			}while(statusWrite != OK);
		}
	}
	
	return OK;
}

/*
 * Showing details of http packet for debugging
 */
void show_http_packet(HttpPacket *packet)
{
	if(packet == NULL){
		return;
	}

	fprintf(stderr, "=========== START ===========\n");
	
	if(packet->action != NULL){
		fprintf(stderr, "ACTION: %s\n", packet->action);
	}

	if(packet->version != NULL){
		fprintf(stderr, "VERSION: %s\n", packet->version);
	}

	if(packet->domainName != NULL){
		fprintf(stderr, "DOMAIN NAME: %s\n", packet->domainName);
	}

	if(packet->page != NULL){
		fprintf(stderr, "PAGE: %s\n", packet->page);
	}

	if(packet->portNum != NULL){
		fprintf(stderr, "PORT NUMBER: %s\n", packet->portNum);
	}

	fprintf(stderr, "=========== END ===========\n");
}
