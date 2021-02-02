#include "./lib/udp_owamp.h"
#include "./lib/err_handle.c"
#include "./lib/wrapunix.c"
#include "./lib/udp_connect.c"
#include "output.c"

char sendline[MAXLINE], rcvline[MAXLINE];  // Package content
int pkt_num;   // The number of packets sent in the detection
long RAND_ID;  // Each detection task has a unique ID 
int interval;  // Packet sending interval (ms)
int add_size = 0;

/*
 * struct timeval
 * {
 * __time_t tv_sec;           // Seconds. 
 * __suseconds_t tv_usec;     // Microseconds. 
 * };
 */

/* struct Probe_Pkt: probe packet structure */

/* struct Reply_Pkt: reply packet structure */

/* struct Raw_Res: raw data obtained by measurement */
struct Raw_Res raw_res[1000];

void ms_sleep(int ms)
{
	struct timeval ST;
	ST.tv_sec    = 0;
	ST.tv_usec   = ms*1000;
	select(0, NULL, NULL, NULL, &ST);
}

/* Send probe packet */
void * send_pkt(void * send_sd){
	int sd = *(int *) send_sd;
	long send_time;
	struct timeval tv;
	struct Probe_Pkt * probe_pkt = sendline;
	probe_pkt->ID = RAND_ID;
	int i;
	for(i=0; i<pkt_num; i++){
		gettimeofday(&tv, NULL);
		send_time = tv.tv_sec * 1000000 + tv.tv_usec;
		probe_pkt->Send_time = send_time;
		probe_pkt->SSN = pkt_num*10000 + i + 1;
		write(sd, sendline, sizeof(struct Probe_Pkt)+add_size);	/* send probe datagram */
		if(interval && i+1<pkt_num){
			ms_sleep(interval);
		}
	}
}


/* Receive reply packet */
void * rcv_pkt(void * rcv_sd){
	int sd = *(int *) rcv_sd;
	int SN;
	struct Reply_Pkt * reply_pkt = rcvline;
	struct timeval tv;
	for(; ; ){
		read(sd, rcvline, MAXLINE);             /* receive reply datagram */
		SN = (reply_pkt->SSN) % 10000 - 1;
		raw_res[SN].Send_time = reply_pkt->Send_time;
		raw_res[SN].Rcv_time = reply_pkt->Rcv_time;
		gettimeofday(&tv, NULL);
		raw_res[SN].Reply_time = tv.tv_sec * 1000000 + tv.tv_usec;
		raw_res[SN].SSN = reply_pkt->SSN;
		raw_res[SN].RSN = reply_pkt->RSN;
	}
}

int main(int argc, char **argv)
{
	memset(raw_res, 0, sizeof(raw_res));
	int sockfd;
	struct timeval tv_id;
	pkt_num = 10;
	interval = 100;
	char *c_val = "10", *i_val = "100", *b_val = "false";
	if (argc == 3){
		sockfd = Udp_connect(argv[1], argv[2]);
	}else if(argc == 5 || argc == 7){
		int opt, flags;
		flags = 0;
		char *optstring = "c:i:b:";
		while ((opt = getopt(argc-2, argv+2, optstring)) != -1) {
			switch(opt) {
				case 'c':
					c_val = optarg;
					break;
				case 'i':
					i_val = optarg;
					break;
				case 'b':
					b_val = optarg;
					break;
				default:
					break;

			}
		}
		pkt_num = atoi(c_val);
		interval = atoi(i_val);
		add_size = 0;
		if(b_val[0] == 'T' || b_val[0] == 't'){
			pkt_num = 100;
			interval = 0;
			add_size = 1000;
		}
		if(pkt_num > 1000){
			err_quit("error: count should be smaller than 1000");
		}
		if(pkt_num * interval>20000){
			err_quit("error: count*interval should be smaller than 20000");
		}
		sockfd = Udp_connect(argv[1], argv[2]);
	}else{
		err_quit("usage: daytimeudpcli2 <hostname/IPaddress> <service/port#> [-c count] [-i interval/ms]");
	}
	sockfd = Udp_connect(argv[1], argv[2]);
	gettimeofday(&tv_id, NULL);
	RAND_ID = tv_id.tv_sec * 1000000 + tv_id.tv_usec;
	pthread_t send_t, rcv_t;

	// Create the sending thread and receiving thread
	pthread_create(&send_t, NULL, send_pkt, &sockfd);
	pthread_create(&rcv_t, NULL, rcv_pkt, &sockfd);
	ms_sleep(interval * pkt_num + 1000);

	// Kill the sending thread and receiving thread
	pthread_cancel(send_t);
	pthread_cancel(rcv_t);

	close(sockfd);

	print_raw_data(raw_res, pkt_num);
	printf("===============================================\n");
	delay_calc(raw_res, pkt_num);
	printf("===============================================\n");
	jitter_calc(raw_res, pkt_num);
	printf("===============================================\n");
	loss_rate_calc(raw_res, pkt_num);
	printf("===============================================\n");
	if(b_val[0] == 'T' || b_val[0] == 't'){
		abw_calc(raw_res, pkt_num, add_size + 24);
	}
	exit(0);
}

