#include "./lib/udp_owamp.h"
#include "./lib/err_handle.c"
#include "./lib/wrapunix.c"
#include "./lib/udp_connect.c"
#include "./lib/func.c"
#include "output.c"

char sendline[MAXLINE], rcvline[MAXLINE];  // Package content
int meas_mode; // measurement mode (1:basic, 2:Available bandwidth measurement)
int pkt_num;   // The number of packets sent in the detection
long RAND_ID;  // Each measurement task has a unique ID 
int interval;  // Packet sending interval (us)
int psize;     // probe packet size (no more than 1400 bytes)
double rate;   // sending rate (bps)
int duration;  // mesurement duration (us)

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

void us_sleep(int us)
{
	struct timeval ST;
	ST.tv_sec    = us / 1000000;
	ST.tv_usec   = us;
	select(0, NULL, NULL, NULL, &ST);
}

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
		if(meas_mode == 1){
			wait_time = interval * i - (int)(cur_time - start_time);
			if (wait_time > 0) us_sleep(wait_time);
		}else if(meas_mode == 2){
			wait_time = (int)(duration * i/(pkt_num - 1)) - (int)(cur_time - start_time);
			if (wait_time > 0) us_sleep(wait_time);
		}
		gettimeofday(&tv, NULL);
		send_time = tv.tv_sec * 1000000 + tv.tv_usec;
		probe_pkt->Send_time = send_time;
		probe_pkt->TOT = pkt_num;
		probe_pkt->SSN = i + 1;
		write(sd, sendline, max(sizeof(struct Probe_Pkt), psize-28));	/* send probe datagram */
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
	srandom(getpid()); /* Create random payload; does it matter? */
	int i;
	for (i=0; i<MAXLINE-1; i++) sendline[i]=(char)(random()&0x000000ff);
	memset(raw_res, 0, sizeof(raw_res));
	int sockfd;
	struct timeval tv_id;
	pkt_num = 10;
	interval = 100;
	char *m_val = "basic", *c_val = "10", *i_val = "1000us", *l_val = "0";
	char *d_val = "20ms", *r_val = "10M";  //ABM mode
	if (argc >= 3){
		int opt, flags;
		flags = 0;
		char *optstring = "m:c:i:l:d:r:h:";
		while ((opt = getopt(argc-2, argv+2, optstring)) != -1) {
			switch(opt) {
				case 'm': // measurement mode, basic or ABM
					m_val = optarg;
					break;
				case 'c': //count
					c_val = optarg;
					break;
				case 'i': //interval (microsecond)
					i_val = optarg;
					break;
				case 'l': //length of probe packet
					l_val = optarg;
					break;
				case 'd': // duration of a measurement
					d_val = optarg;
					break;
				case 'r': // send rate
					r_val = optarg;
					break;
				default:
					break;

			}
		}
		if(strcmp(m_val, "basic") == 0) meas_mode = 1;
		else meas_mode = 2;
		pkt_num = atoi(c_val);
		interval = duration_atoi(i_val);
		psize = atoi(l_val);
		rate = rate_atof(r_val);
		duration = duration_atoi(d_val);
		if (meas_mode == 2 && psize == 0) psize = 1400;
		pkt_num = (int)(duration * rate / 1000000 / psize);

		if(meas_mode == 1 && pkt_num > 1000){
			err_quit("error: count should be smaller than 1000 in basic mode");
		}
		if(meas_mode == 1 && pkt_num * interval>20000000){
			err_quit("error: count*interval should be smaller than 20000000us in basic mode");
		}
		
	}else{
		err_quit("usage: udp_owamp_client <hostname/IPaddress> <service/port#> [-m basic] [-c count] [-i interval/us] [-l probe_pkt size]\n\tOr: udp_owamp_client <hostname/IPaddress> <service/port#> -m ABM [-d duration] [-r sending_rate]");
	}
	sockfd = Udp_connect(argv[1], argv[2]);
	int nZero=1024*1024; //set 1MB send buff
	setsockopt(sockfd, SOL_SOCKET,SO_SNDBUF, (char *)&nZero,sizeof(nZero));
	gettimeofday(&tv_id, NULL);
	RAND_ID = tv_id.tv_sec * 1000000 + tv_id.tv_usec;
	/*if(m_val == 1){

	}else if(m_val == 2){

	}*/
	pthread_t send_t, rcv_t;

	// Create the sending thread and receiving thread
	pthread_create(&rcv_t, NULL, rcv_pkt, &sockfd);
	pthread_create(&send_t, NULL, send_pkt, &sockfd);
	us_sleep(interval * pkt_num + 1000000);

	// Kill the sending thread and receiving thread
	pthread_cancel(send_t);
	pthread_cancel(rcv_t);

	close(sockfd);
	/*
	print_raw_data(raw_res, pkt_num);
	printf("===============================================\n");
	delay_calc(raw_res, pkt_num);
	printf("===============================================\n");
	jitter_calc(raw_res, pkt_num);
	printf("===============================================\n");
	loss_rate_calc(raw_res, pkt_num);
	printf("===============================================\n");
	if(m_val == 2){
		abw_calc(raw_res, pkt_num, psize);
	}*/
	exit(0);
}

