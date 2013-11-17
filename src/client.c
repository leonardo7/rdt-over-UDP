/*
 * client.c
 *
 *  Created on: Oct 19, 2013
 *      Author: Ashraf Saleh
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#define BUFFER_SIZE 1024
#define PORT_NO 7777
#define HOSTNAME "localhost"

 char request_buffer[BUFFER_SIZE];
 unsigned char buffer[BUFFER_SIZE];
 int socket_fd;
struct sockaddr_in server_addr;
	socklen_t server_addr_len;


 char file_cname[100];
// char read_buffer[BUFFER_SIZE];

 int parse_response() {
 	int n = recvfrom(socket_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &server_addr, &server_addr_len);
 //file size
 	FILE *op = fopen(file_cname, "wb");

 	while (1) {
 		if (n <= 0) {
 			fclose(op);
 			break;
 		}
 		fwrite(buffer, sizeof(char), n, op);
 		n = read(socket_fd, buffer, BUFFER_SIZE);
 	}

 }

 	
 int main(int argc, char *argv[]) {

 	in_port_t port_no = PORT_NO;
 	struct hostent *server;
 	char *hostname = HOSTNAME;

 	server = gethostbyname(hostname);

 	if (server == NULL ) {
 		perror("ERROR no such host exists");
 		exit(EXIT_FAILURE);
 	}

 	char file_cname[] = "oblivion.mp3";

 	socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
 	if (socket_fd < 0)
 		perror("ERROR Opening client socket");

    memset(&server_addr, 0, sizeof(server_addr));// initialization
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port_no);
	memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);	

 // TODO 
 // request_buffer = file_cname;
	memcpy(&request_buffer, file_cname, sizeof(file_cname));

	server_addr_len = sizeof(server_addr);
 	//send to server
 	int n = sendto(socket_fd, request_buffer, strlen(request_buffer) , 0, (struct sockaddr*) &server_addr, server_addr_len);
 	if (n < 0)
 		perror("ERROR couldn't write to socket");

 	printf("[starting] read_response\n");
 	parse_response();
 	printf("[closing] read_response\n");

 	close(socket_fd);

	return EXIT_SUCCESS;
}