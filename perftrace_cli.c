#include "./lib/udp_owamp.h"
#include "./lib/err_handle.c"
#include "./lib/wrapunix.c"
#include "./lib/udp_connect.c"
#include "./lib/func.c"
#include "./lib/sql_func.c"
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
char database_path[PATH_LEN];

/* struct Raw_Res1 for mode1: raw data obtained by measurement */
struct Raw_Res1 raw_res1[MAXPKT1+1];
/* struct Raw_Res2 for mode2 */
struct Raw_Res2 raw_res2[MAXPKT2+1];

int get_database_path(char * database_path, int path_len){
    int count;
    count = readlink( "/proc/self/exe", database_path, path_len);
    if ( count < 0 || count >= path_len){ 
		return 1;
	}
    else{
		char *lastslash;
		lastslash = strrchr(database_path, '/');
		lastslash[1] = '\0';
		strcat(database_path,DATA_BASE);
        return 0;
    }
}


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
		if(pkt_num_send>1) wait_time = (int)(duration * i/(pkt_num_send - 1)) - (int)(cur_time - start_time);
		else wait_time = -1;
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
	int path_code = get_database_path(database_path, PATH_LEN);
	if(path_code == 1){
		err_quit("can't get the absolute path to the database");
	}

	printf("%s\n", database_path);
	int					sockfd_tcp, sockfd_udp, n;
	struct sockaddr_in	servaddr;
	struct timeval		tv_id, tv;
	struct Measurement	meas_res;
	memset(&meas_res, 0, sizeof(meas_res));
	/* create sqlite table */
	if(table_is_exist(database_path, TABLE_NAME) == 0) create_table(database_path, create_sql);
	pkt_num_send = 10;
	char *src_ip = "0.0.0.0";
	char *task_name = "test";
	char *m_val = "1", *c_val = "10", *i_val = "1000us", *l_val = "0", *serv_ip = "0.0.0.0";
	char *d_val = DURATION, *r_val = "-1";  //ABM mode
	if (argc < 3){
		err_quit("usage: ./perftrace_cli -s <IPaddress>");
	}else{
		int opt;
		char *optstring = "s:m:c:i:l:d:r:h:n:";
		while ((opt = getopt(argc, argv, optstring)) != -1) {
			switch(opt) {
				case 's': // measurement server IP
					serv_ip = optarg;
					break;
				case 'm': // measurement mode, basic(1) or ABM(2)
					m_val = optarg;
					break;
				case 'c': //count of sending packets
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
				case 'n': // task name
					task_name = optarg;
					break;
				default:
					break;

			}
		}
	}
	if ( (sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err_sys("socket error");
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port   = htons(TCP_PORT);	/* measurement server */
	if (inet_pton(AF_INET, serv_ip, &servaddr.sin_addr) <= 0)
		err_quit("inet_pton error for %s", serv_ip);
	if (connect(sockfd_tcp, (SA *) &servaddr, sizeof(servaddr)) < 0)
		err_sys("connect error");
	if ( (n = read(sockfd_tcp, rcvline_tcp, MAXLINE)) > 0) {
		rcvline_tcp[n] = 0;	/* null terminate */
		if(strcmp(rcvline_tcp, ACCEPT) == 0){  /* server accept this measurement */
			gettimeofday(&tv_id, NULL);
			RAND_ID = tv_id.tv_sec * 1000000 + tv_id.tv_usec;
			/* get task parameter. */
			pkt_num_send = atoi(c_val);
			duration = duration_atoi(d_val);
			meas_mode = atoi(m_val);
			if(meas_mode==1) duration = (int)duration_atoi(i_val)*(pkt_num_send - 1);
			psize = atoi(l_val);
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
			meas_res.mode = meas_mode;
			meas_res.time_stamp = tv1.tv_sec * 1000000L + tv1.tv_usec;
			if( (n = Read(sockfd_tcp, rcvline_tcp, MAXLINE)) > 0){  // wait for ack. When ack received, start sending probe packets.
				gettimeofday(&tv2, NULL);
				RTT = (int)((tv2.tv_sec-tv1.tv_sec)*1000000 + tv2.tv_usec-tv1.tv_usec);
				printf("RTT=%d\n",RTT);
				rcvline_tcp[n] = 0;
				sockfd_udp = Udp_connect(serv_ip, UDP_PORT_STR);
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
					delay_calc(raw_res1, pkt_num_send, &meas_res);
					printf("===============================================\n");
					jitter_calc(raw_res1, pkt_num_send, &meas_res);
					printf("===============================================\n");
					loss_rate_calc(raw_res1, pkt_num_send, pkt_num_send_arrive[0], &meas_res);
					printf("===============================================\n");
					insert_mode1(task_name, src_ip, serv_ip, meas_res);
				}else{
					int iternum = 0;
					rate = rate_atof(r_val);
					send_rate_init(&rate, src_ip,serv_ip);
					printf("initial rate = %f\n", rate);
					construct_send_args(rate, &duration, &pkt_num_send, &psize);
					int step = 0;
					int sub_step = 0;
					//int rate_and_duration[2] = {5,5};
					float mid_res[4];
					double rcv_rate = 0;
					int tot_pkt_snd = 0;
					int tot_pkt_arv = 0;
					long OWD_sd_sum = 0;
					while(iternum < MAXITER){
						printf("rate=%f, duration=%d, pkt_num_send=%d, psize=%d, step=%d\n", rate, duration, pkt_num_send, psize, step);
						if(pkt_num_send > MAXPKT2) err_quit("error: the number of probe packets shouldn't be bigger than 10240 in ABM mode");
						snprintf(sendline_tcp, sizeof(sendline_tcp), MODE2_MEASURE);
						Write(sockfd_tcp, sendline_tcp, strlen(sendline_tcp));
						us_sleep(RTT/2 + 1000); // wait
						memset(raw_res2, 0, sizeof(raw_res2));
						send_pkt(&sockfd_udp);
						us_sleep(10000); // wait
						struct Mode2_Send_Meta *Sendmeta_tcp = sendline_tcp;
						Sendmeta_tcp->duration = duration;
						Sendmeta_tcp->pkt_num_send = pkt_num_send;
						tot_pkt_snd += pkt_num_send;
						Sendmeta_tcp->psize = max(psize, sizeof(struct Probe_Pkt)+28);
						Write(sockfd_tcp, sendline_tcp, sizeof(struct Mode2_Send_Meta));
						if( (n = read(sockfd_tcp, rcvline_tcp, MAXLINE)) > 0 ){
							struct Mode2_Result *Result_tcp = rcvline_tcp;
							tot_pkt_arv += Result_tcp->arv_pkt_cnt;
							meas_res.Jitter_sd += Result_tcp->jitter * Result_tcp->arv_pkt_cnt;
							OWD_sd_sum += Result_tcp->aver_owd * Result_tcp->arv_pkt_cnt;
							printf("========================ABW========================\n");
							printf("Iter %d:\n", iternum + 1);
							printf("Loss rate: %f\n", Result_tcp->loss_rate);
							printf("Specified sending rate: %fbps\n", rate);
							double actual_rate = (double)pkt_num_send*psize/duration*1000000*8;
							printf("Actual sending rate: %fbps\n", actual_rate);
							printf("Receiving rate of all packets: %fbps\n",Result_tcp->rate1);
							printf("Receiving rate in duration: %fbps\n",Result_tcp->rate2);
							rcv_rate = (Result_tcp->rate1+Result_tcp->rate2)/2;
							if(step == 0){
								if(Result_tcp->loss_rate > LOSS_RATE_THRESHOLD && sub_step == 0){
									rate = rcv_rate / FACTOR;
									construct_send_args(rate, &duration, &pkt_num_send, &psize);
								}else if(rcv_rate*RATE_THRESHOLD < actual_rate){
									rate = rcv_rate;
									construct_send_args(rate, &duration, &pkt_num_send, &psize);
									step = 1;
								}else{
									if(actual_rate * 1.01 < rate){ // use 1.01 to increases robustness
										rate = min(actual_rate, rcv_rate);
										construct_send_args(rate, &duration, &pkt_num_send, &psize);
										step = 1;
									}else{
										sub_step = 1;
										rate = rate * FACTOR;
										construct_send_args(rate, &duration, &pkt_num_send, &psize);
									}
								}
							}else if(step == 1){
								mid_res[0] = actual_rate;
								mid_res[1] = rcv_rate;
								rate = rcv_rate;
								construct_send_args(rate, &duration, &pkt_num_send, &psize);
								step = 2;
							}else{
								mid_res[2] = actual_rate;
								mid_res[3] = rcv_rate;

								construct_send_args(rate, &duration, &pkt_num_send, &psize);
								step = 3;
							}
						}
						iternum ++;
						if( strcmp(meas_type, MODE2_TEST)==0 || step == 3) break;
					}
					printf("\n======================Final Result======================\n");
					double final_res = 0.0;
					if(step == 3){
						double s1,r1,s2,r2;
						s1 = mid_res[0];
						r1 = mid_res[1];
						s2 = mid_res[2];
						r2 = mid_res[3];
						printf("Vsnd1=%f\tVrcv1=%f\tVsnd2=%f\tVrcv2=%f\n", s1,r1,s2,r2);
						if(r1<=s1 && r2<=s2){
							if (s2-r2 < s1 -r1) printf("final result: %fbps.\n",final_res = max(r2-s2+r2, (s1*r2*(r1+s2)-r1*s2*(r2+s1))/(r2*s1-s2*r1)));
							else printf("final result: %fbps.\n", final_res = r2);
						}else printf("final result: %fbps.\n", final_res = min(r2,min(s1,r1)) );
					}else{
						printf("final result: %fbps.\n", final_res = rcv_rate);
					}
					printf("end udp.\n");
					snprintf(sendline_tcp, sizeof(sendline_tcp), MODE2_END);
					Write(sockfd_tcp, sendline_tcp, strlen(sendline_tcp));
					Close(sockfd_udp);
					meas_res.Jitter_sd = meas_res.Jitter_sd / tot_pkt_arv;
					meas_res.OWD_sd = OWD_sd_sum / tot_pkt_arv;
					meas_res.LossRate_sd = (float)(tot_pkt_snd - tot_pkt_arv) / (float)tot_pkt_snd;
					meas_res.ABW_sd = final_res;
					insert_mode2(task_name, src_ip, serv_ip, meas_res);
				}
			}
		}
	}
	if (n < 0)
		err_sys("read error");

	close(sockfd_tcp);
	exit(0);
}
