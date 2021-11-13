#include "./lib/udp_owamp.h"
#include "./lib/err_handle.c"
#include "./lib/wrapunix.c"
#include "./lib/udp_connect.c"
#include "./lib/func.c"
#include "output.c"

char sendline_udp[MAXLINE], rcvline_udp[MAXLINE], sendline_tcp[MAXLINE], rcvline_tcp[MAXLINE];  // Package content
int meas_mode; // measurement mode (1:basic, 2:Available bandwidth measurement)
int pkt_num_send, pkt_num_reply_arrive;   // The number of packets sent and reply
int * pkt_num_send_arrive; 
long RAND_ID;  // Each measurement task has a unique ID 
int interval;  // Packet sending interval (us)
int psize;     // probe packet size (no more than 1400 bytes)
double rate;   // sending rate (bps)
int duration;  // mesurement duration (us)
int RTT; // (us)
char meas_type[10];  // measure or test in mode2

/* struct Raw_Res1 for mode1: raw data obtained by measurement */
struct Raw_Res1 raw_res1[MAXPKT1+1];
/* struct Raw_Res2 for mode2 */
struct Raw_Res2 raw_res2[MAXPKT2+1];

/* Send probe packet */
void * send_pkt(void * send_sd){
	int sd = *(int *) send_sd;
	long send_time, start_time, cur_time;
	struct timeval tv;
	struct Probe_Pkt * probe_pkt = sendline_udp;
	probe_pkt->ID = RAND_ID;
	int wait_time;
	gettimeofday(&tv, NULL);
	start_time = tv.tv_sec * 1000000 + tv.tv_usec;
	int i;
	for(i=0; i<pkt_num_send; i++){
		gettimeofday(&tv, NULL);
		cur_time = tv.tv_sec * 1000000 + tv.tv_usec;
		if(duration+100 < cur_time-start_time){
			duration = (int)(cur_time-start_time);
			break;  // 0.1ms overtime, no longer send
		}
		wait_time = (int)(duration * i/(pkt_num_send - 1)) - (int)(cur_time - start_time);
		if (wait_time > 0) us_sleep(wait_time);
		gettimeofday(&tv, NULL);
		send_time = tv.tv_sec * 1000000 + tv.tv_usec;
		probe_pkt->Send_time = send_time;
		probe_pkt->SSN = i + 1;
		if(meas_mode == 2){
			raw_res2[i+1].Send_time = send_time;
		}
		Write(sd, sendline_udp, max(sizeof(struct Probe_Pkt), max(0, psize-28)));	/* send probe datagram */
	}
	pkt_num_send = i;
}

/* Receive reply packet */
void * rcv_pkt(void * rcv_sd){
	int sd = *(int *) rcv_sd;
	int SN;
	struct Reply_Pkt * reply_pkt = rcvline_udp;
	struct timeval tv;
	for(; ; ){
		Read(sd, rcvline_udp, MAXLINE);             /* receive reply datagram */
		SN = reply_pkt->SSN;
		gettimeofday(&tv, NULL); 
		raw_res1[SN].Reply_arrive_time = tv.tv_sec * 1000000 + tv.tv_usec;
		raw_res1[SN].Send_time = reply_pkt->Send_time;
		raw_res1[SN].Send_arrive_time = reply_pkt->Send_arrive_time;
		raw_res1[SN].SSN = reply_pkt->SSN;
		raw_res1[SN].RSN = reply_pkt->RSN;
	}
}

int main(int argc, char **argv)
{
	int					sockfd_tcp, sockfd_udp, n;
	struct sockaddr_in	servaddr;
	struct timeval		tv_id, tv;

	if (argc != 2)
		err_quit("usage: a.out <IPaddress>");

	if ( (sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err_sys("socket error");
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port   = htons(19999);	/* measurement server */
	if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
		err_quit("inet_pton error for %s", argv[1]);
	if (connect(sockfd_tcp, (SA *) &servaddr, sizeof(servaddr)) < 0)
		err_sys("connect error");
	if ( (n = read(sockfd_tcp, rcvline_tcp, MAXLINE)) > 0) {
		rcvline_tcp[n] = 0;	/* null terminate */
		if(strcmp(rcvline_tcp, ACCEPT) == 0){  /* server accept this measurement */
			gettimeofday(&tv_id, NULL);
			RAND_ID = tv_id.tv_sec * 1000000 + tv_id.tv_usec;
			/* get task parameter. */
			pkt_num_send = 2000;
			duration = 20000;
			meas_mode = 2;
			psize = MAXPSIZE;
			strcpy(meas_type, "measure");
			
			struct Task_Meta *	task_metadata = sendline_tcp;
			task_metadata->ID = RAND_ID;
			task_metadata->duration = duration;
			task_metadata->pkt_num = pkt_num_send;
			task_metadata->task_mode = meas_mode;
			int RTT;
			struct timeval tv1, tv2;
			gettimeofday(&tv1, NULL);
			Write(sockfd_tcp, sendline_tcp, sizeof(struct Task_Meta));  // send metadata
			if( (n = Read(sockfd_tcp, rcvline_tcp, MAXLINE)) > 0){  // wait for ack. When ack received, start sending probe packets.
				gettimeofday(&tv2, NULL);
				RTT = (int)((tv2.tv_sec-tv1.tv_sec)*1000000 + tv2.tv_usec-tv1.tv_usec);
				printf("RTT=%d\n",RTT);
				rcvline_tcp[n] = 0;
				sockfd_udp = Udp_connect(argv[1], "19999");
				int nZero=2*1024*1024; //set 2MB send buff
				setsockopt(sockfd_udp, SOL_SOCKET,SO_SNDBUF, (char *)&nZero,sizeof(nZero));
				int i;
				for (i=0; i<MAXLINE; i++) sendline_udp[i]=(char)(random()&0x000000ff);
				if(meas_mode == 1){
					if(pkt_num_send > MAXPKT1) err_quit("error: the number of probe packets shouldn't be bigger than 1000 in basic mode");
					memset(raw_res1, 0, sizeof(raw_res1));
					pthread_t send_t, rcv_t;
					// Create the sending thread and receiving thread
					pthread_create(&rcv_t, NULL, rcv_pkt, &sockfd_udp);
					pthread_create(&send_t, NULL, send_pkt, &sockfd_udp);

					if( (n = Read(sockfd_tcp, rcvline_tcp, MAXLINE)) > 0){  // Wait for the server to receive and process probe packet.
						pkt_num_send_arrive = rcvline_tcp;  // get the number of packets arrived at server.
						printf("Send:%d, Arrived:%d\n", pkt_num_send, pkt_num_send_arrive[0]);
						gettimeofday(&tv, NULL);
						printf("%ld\n", tv.tv_sec * 1000000 + tv.tv_usec);
					}
					// Kill the sending thread and receiving thread
					pthread_cancel(send_t);
					pthread_cancel(rcv_t);

					Close(sockfd_udp);
					printf("end udp.\n");
					//print_raw_data(raw_res1, pkt_num_send);
					printf("===============================================\n");
					delay_calc(raw_res1, pkt_num_send);
					printf("===============================================\n");
					jitter_calc(raw_res1, pkt_num_send);
					printf("===============================================\n");
					loss_rate_calc(raw_res1, pkt_num_send, pkt_num_send_arrive[0]);
					printf("===============================================\n");
				}else{
					if(pkt_num_send > MAXPKT2) err_quit("error: the number of probe packets shouldn't be bigger than 4096 in basic mode");
					//int rate_and_duration[2] = {5,5};
					int iternum = 0;
					vsnd_init();
					while(iternum < MAXITER){
						snprintf(sendline_tcp, sizeof(sendline_tcp), MODE2_MEASURE);
						Write(sockfd_tcp, sendline_tcp, strlen(sendline_tcp));
						us_sleep(RTT/2 + 1000); // wait
						memset(raw_res2, 0, sizeof(raw_res2));
						send_pkt(&sockfd_udp);
						us_sleep(10000); // wait
						struct Mode2_Send_Meta *Sendmeta_tcp = sendline_tcp;
						Sendmeta_tcp->duration = duration;
						Sendmeta_tcp->pkt_num_send = pkt_num_send;
						Sendmeta_tcp->psize = max(psize, sizeof(struct Probe_Pkt)+28);
						Write(sockfd_tcp, sendline_tcp, sizeof(struct Mode2_Send_Meta));
						if( (n = read(sockfd_tcp, rcvline_tcp, MAXLINE)) > 0 ){
							struct Mode2_Result *Result_tcp = rcvline_tcp;
							printf("========================ABW========================\n");
							printf("Iter %d:\n", iternum + 1);
							printf("Loss rate: %f\n", Result_tcp->loss_rate);
							printf("Specified sending rate: %fbps\n", rate);
							printf("Actual sending rate: %fbps\n", (float)pkt_num_send*psize/duration*1000000*8);
							printf("Receiving rate of all packets: %fbps\n",Result_tcp->rate1);
							printf("Receiving rate in duration: %fbps\n",Result_tcp->rate2);
						}
						iternum ++;
						/*
						int offset, record_cnt = 0;
						struct Mode2_Reply_Header * head_tcp = rcvline_tcp;
						struct OWD_Record * payload_tcp = rcvline_tcp + sizeof(struct Mode2_Reply_Header);
						while(1){
							if( (n = read(sockfd_tcp, rcvline_tcp, MAXLINE)) > 0 && head_tcp->ID == RAND_ID){ 
								offset = head_tcp->offset;
								record_cnt += head_tcp->size;
								for(i=0; i<head_tcp->size;i++){
									raw_res2[payload_tcp[i].SSN].RSN = offset + i;
									raw_res2[payload_tcp[i].SSN].OWD = payload_tcp[i].OWD;
									//printf("SSN=%d, RSN=%d, OWD=%d\n", payload_tcp[i].SSN, offset + i, payload_tcp[i].OWD);
								}
								printf("record_cnt=%d\n", record_cnt);
							}
							if(n>0 && record_cnt == head_tcp->pkt_tot)break;
						}
						
						psize = max(psize, sizeof(struct Probe_Pkt)+28);
						abw_calc(rate, pkt_num_send, psize, duration, raw_res2);
						*/
						if( strcmp(meas_type, "test")==0 ) break;
					}
					printf("end udp.\n");
					snprintf(sendline_tcp, sizeof(sendline_tcp), MODE2_END);
					Write(sockfd_tcp, sendline_tcp, strlen(sendline_tcp));
					Close(sockfd_udp);
				}
			}
		}
	}
	if (n < 0)
		err_sys("read error");

	close(sockfd_tcp);
	exit(0);
}