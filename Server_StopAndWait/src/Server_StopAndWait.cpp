//============================================================================
// Name        : Server_StopAndWait.cpp
// Author      : Motaz ElShaer
// Version     :
// Copyright   : Your copyright notice
// Description : Server in C++, Ansi-style
//============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <time.h>

#define PKT_DATA_SIZE 1000 // identify data size (in bytes) in a packet
#define TIME_OUT_VAL 500000ll // value in micro seconds
#define SERVER_PORT_NO 7777
#define CLIENT_PORT_NO 9999
#define MAX_cwnd 30

// TODO read from file
#define MAX_SEQ_N 4*MAX_cwnd
#define BUFF_SIZE PKT_DATA_SIZE*MAX_SEQ_N //file buffer size in bytes
#define HEADER_SIZE 8 // size of header in bytes
#define PKT_SIZE HEADER_SIZE+PKT_DATA_SIZE

typedef struct {
	int16_t len;
	int16_t checksum;
	int32_t seqno;
	char data[PKT_DATA_SIZE];
} pkt_t;

typedef struct {
	int16_t len;
	int16_t checksum;
	int32_t seqno;
} ack_t;

int cwnd = MAX_cwnd;
unsigned int trans_sock;

int main_sock, worker_sock;
struct sockaddr_in server_addr, client_addr;
socklen_t server_addr_len = sizeof(server_addr);
socklen_t client_addr_len = sizeof(client_addr);

pkt_t packet_buff[MAX_SEQ_N];
char file_buff[MAX_SEQ_N];

int buff_base = 0;
char file_name[100] = "oblivion.mp3";
FILE *file;

#define close_file(a) fclose(a)
#define open_file(s) (file = fopen(s, "rb"))

void read_nbyte(unsigned long offset, unsigned long n) {
	// read n bytes from the the file starting from offset
	fseek(file, offset, SEEK_SET);
	n = n >= MAX_SEQ_N ? MAX_SEQ_N - 1 : n;
	fread(file_buff, n, 1, file);
}

unsigned long calc_file_size() {
	//Get file length
	fseek(file, 0, SEEK_END);
	unsigned long len = ftell(file);
	fseek(file, 0, SEEK_SET);
	return len;
}

void process_pkt(int seqno) {
	packet_buff[seqno].len = PKT_SIZE;
	packet_buff[seqno].seqno = seqno;

	int offset = seqno * PKT_DATA_SIZE;
	memcpy(packet_buff[seqno].data, file_buff + offset, PKT_DATA_SIZE);

	// TODO calc checksum or crc
	// pkt.checksum = ?

}

long long timer[MAX_cwnd];
short valid_timer[MAX_cwnd]; // boolean if timer is acive = true, false otherwise.
int timer_n_index = cwnd; // largest index in the timers array

void send_pkt(int seqno) {
	int nbytes = 0;
	if ((nbytes = send(trans_sock, &packet_buff[seqno], PKT_SIZE, 0)) < 0) {
		perror("Sending packet");
	} else {
		//start timer
		valid_timer[seqno - buff_base] = 1;
		timer[seqno - buff_base] = TIME_OUT_VAL;
	}

}

timeval welcome_timeval = { TIME_OUT_VAL / 1000000, TIME_OUT_VAL % (1000000) };
itimerval welcome_itimer = { welcome_timeval, welcome_timeval };

long long request_timers[MAX_cwnd];
unsigned char valid_w_timer[MAX_cwnd]; // boolean if timer is acive = true, false otherwise.

// rename
void timer_handler(int sig) {
	//start in method timer
	clock_t local_timer;
	long long min_timer = TIME_OUT_VAL; // set to maximum value

	int i;
	for (i = 0; i <= timer_n_index; i++) {
		local_timer = clock();
		if (valid_timer[i]) {
			request_timers[i] -= (welcome_timeval.tv_usec
					+ welcome_timeval.tv_sec * 1000000
					+ ((float) local_timer) / CLOCKS_PER_SEC * 1000000);
			if (request_timers[i] <= 0) {
				// fire_action(timer[i]);
				request_timers[i] = TIME_OUT_VAL;
			} else if (request_timers[i] < min_timer) {
				min_timer = request_timers[i];
			}
		}
	}

	welcome_timeval.tv_sec = min_timer / 1000000;
	welcome_timeval.tv_usec = min_timer % (1000000);

	welcome_itimer.it_interval = welcome_timeval;
	welcome_itimer.it_value = welcome_timeval;
	setitimer(ITIMER_REAL, &welcome_itimer, NULL);
}

#define RQST_BUFF_SZ 100
// assuming that the coming request will always fit in this buffer
char request_buff[RQST_BUFF_SZ];
void receive_rqst() {

}

void rdt() {
	// ACK request
	//TODO seqno ?? for the ack ? and seqno for data pkts add ack size to them ? ?
	ack_t req_ack = { 8, 0, 0 };
	int n = sendto(worker_sock, (void *) &req_ack, sizeof(req_ack), 0,
			(struct sockaddr*) &client_addr, client_addr_len);
	if (n < 0)
		perror("ERROR couldn't write to socket");
	// send data

}

void connect() {
	rdt();
}

int main() {

	signal(SIGALRM, timer_handler);

	//TODO read from input file
	in_port_t server_portno = SERVER_PORT_NO;
	in_port_t client_portno = CLIENT_PORT_NO;

	if ((main_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Error: Creating the main socket");
		return EXIT_FAILURE;
	}

	// setting server_addr
	memset((char *) &server_addr, 0, sizeof(server_addr));	// initialization
	// 0 init is important since the server_addr.zero[] must be zero
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(server_portno);

	// setting client_addr
	memset((char *) &client_addr, 0, client_addr_len);

	if ((bind(main_sock, (struct sockaddr*) &server_addr, sizeof(server_addr)))
			< 0) {
		perror("ERROR on binding socket to server address");
		return EXIT_FAILURE;
	}

	int worker_pid;

	while (1) {

		// receive request
		// since we know the server the request message only contains the file path/name
		int recvlen = recvfrom(main_sock, request_buff, (size_t) RQST_BUFF_SZ,
				0, (struct sockaddr*) &client_addr, &client_addr_len);
		printf("%s\n", request_buff);
		worker_pid = fork();

		if (worker_pid = fork() < 0) {
			perror("ERROR on forking a new worker process");
			continue;
		}

		if (!worker_pid) { // worker_pid = 0 then we are in the child
			// create the working socket
			if ((worker_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
				perror("Error: Creating the worker socket");
				return EXIT_FAILURE;
			}

			signal(SIGALRM, timer_handler);
			connect();
			close(main_sock);
			exit(EXIT_SUCCESS);
		} else { // pid != 0 then we are in the parent;
			close(worker_sock);
		}
	}
	return EXIT_SUCCESS;
}