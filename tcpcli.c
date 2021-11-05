#include "./lib/udp_owamp.h"
#include "./lib/err_handle.c"
#include "./lib/wrapunix.c"
#include "./lib/udp_connect.c"
#include "./lib/func.c"
#include "output.c"

char sendline[MAXLINE], rcvline[MAXLINE], sendline_tcp[MAXLINE], rcvline_tcp[MAXLINE];  // Package content
int meas_mode; // measurement mode (1:basic, 2:Available bandwidth measurement)
int pkt_num;   // The number of packets sent in the detection
long RAND_ID;  // Each measurement task has a unique ID 
int interval;  // Packet sending interval (us)
int psize;     // probe packet size (no more than 1400 bytes)
double rate;   // sending rate (bps)
int duration;  // mesurement duration (us)

/* struct Raw_Res for mode1: raw data obtained by measurement */
struct Raw_Res raw_res[1005];

/* Send probe packet */
void * send_pkt(void * send_sd){
	int sd = *(int *) send_sd;
	long send_time, start_time, cur_time;
	struct timeval tv;
	struct Probe_Pkt * probe_pkt = sendline;
	probe_pkt->ID = RAND_ID;
	int i;
	int wait_time;
	gettimeofday(&tv, NULL);
	start_time = tv.tv_sec * 1000000 + tv.tv_usec;
	for(i=0; i<pkt_num; i++){
		gettimeofday(&tv, NULL);
		cur_time = tv.tv_sec * 1000000 + tv.tv_usec;
		if(duration+1000 < cur_time-start_time) break;  // timeout 1ms, no longer send
		wait_time = (int)(duration * i/(pkt_num - 1)) - (int)(cur_time - start_time);
		if (wait_time > 0) us_sleep(wait_time);
		gettimeofday(&tv, NULL);
		send_time = tv.tv_sec * 1000000 + tv.tv_usec;
		probe_pkt->Send_time = send_time;
		probe_pkt->TOT = pkt_num;
		probe_pkt->SSN = i + 1;
		write(sd, sendline, sizeof(struct Probe_Pkt));	/* send probe datagram */
	}
	pkt_num = i;
}

/* Receive reply packet */
void * rcv_pkt(void * rcv_sd){
	int sd = *(int *) rcv_sd;
	int SN;
	struct Reply_Pkt * reply_pkt = rcvline;
	struct timeval tv;
	for(; ; ){
		read(sd, rcvline, MAXLINE);             /* receive reply datagram */
		printf("OK");
		SN = reply_pkt->SSN - 1;
		gettimeofday(&tv, NULL); 
		raw_res[SN].Reply_time = tv.tv_sec * 1000000 + tv.tv_usec;
		raw_res[SN].Send_time = reply_pkt->Send_time;
		raw_res[SN].Rcv_time = reply_pkt->Rcv_time;
		raw_res[SN].TOT = reply_pkt->TOT;
		raw_res[SN].SSN = reply_pkt->SSN;
		raw_res[SN].RSN = reply_pkt->RSN;
	}
}

int main(int argc, char **argv)
{
	int					sockfd, n,n2,n3, counter = 0, sockfd_udp;
	struct sockaddr_in	servaddr;
	struct timeval		tv_id, tv;
	struct Task_Meta *	task_metadata = sendline_tcp;

	if (argc != 2)
		err_quit("usage: a.out <IPaddress>");

	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err_sys("socket error");
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port   = htons(19999);	/* daytime server */
	if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
		err_quit("inet_pton error for %s", argv[1]);
	if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0)
		err_sys("connect error");
	if ( (n = read(sockfd, rcvline_tcp, MAXLINE)) > 0) {
		rcvline_tcp[n] = 0;	/* null terminate */
		if(strcmp(rcvline_tcp, "Accept") == 0){
			gettimeofday(&tv_id, NULL);
			RAND_ID = tv_id.tv_sec * 1000000 + tv_id.tv_usec;
			pkt_num = 1000;
			duration = 10000;
			meas_mode = 2;
			psize = 0;
			task_metadata->ID = RAND_ID;
			task_metadata->duration = duration;
			task_metadata->pkt_num = pkt_num;
			task_metadata->task_mode = meas_mode;
			Write(sockfd, sendline_tcp, sizeof(struct Task_Meta));  // send metadata
			if( (n2 = read(sockfd, rcvline_tcp, MAXLINE)) > 0){  
				rcvline_tcp[n2] = 0;
				printf("%s\n", rcvline_tcp);   // ack received, measurement can start
				sockfd_udp = Udp_connect(argv[1], "19999");
				int nZero=1024*1024; //set 1MB send buff
				setsockopt(sockfd_udp, SOL_SOCKET,SO_SNDBUF, (char *)&nZero,sizeof(nZero));
				if(meas_mode == 1){
					pthread_t send_t, rcv_t;
					// Create the sending thread and receiving thread
					int a1 = pthread_create(&rcv_t, NULL, rcv_pkt, &sockfd_udp);
					int a2 = pthread_create(&send_t, NULL, send_pkt, &sockfd_udp);
					// us_sleep(interval * pkt_num + 1000000);
					int * receive_pkt_tmp;
					if( (n3 = read(sockfd, rcvline_tcp, MAXLINE)) > 0){ 
						receive_pkt_tmp = rcvline_tcp;
						printf("tot:%d, received:%d\n", pkt_num, receive_pkt_tmp[0]);
					}
					// Kill the sending thread and receiving thread
					pthread_cancel(send_t);
					pthread_cancel(rcv_t);
					close(sockfd_udp);
					printf("end udp.\n");
					print_raw_data(raw_res, pkt_num);
					printf("===============================================\n");
					delay_calc(raw_res, pkt_num);
					printf("===============================================\n");
					jitter_calc(raw_res, pkt_num);
					printf("===============================================\n");
					loss_rate_calc(raw_res, pkt_num, receive_pkt_tmp[0]);
					printf("===============================================\n");
				}else{
					send_pkt(&sockfd_udp);
					close(sockfd_udp);
					int * receive_pkt_tmp;
					if( (n3 = read(sockfd, rcvline_tcp, MAXLINE)) > 0){ 
						receive_pkt_tmp = rcvline_tcp;
						printf("tot:%d, received:%d\n", pkt_num, receive_pkt_tmp[0]);
					}
					printf("end udp.\n");
				}
			}
		}
	}
	if (n < 0)
		err_sys("read error");

	close(sockfd);
	exit(0);
}