#include "./lib/udp_owamp.h"
#include "./lib/err_handle.c"
#include "./lib/wrapunix.c"
#include "./lib/udp_connect.c"
#include "./lib/func.c"
#include "./lib/udp_server_reuseaddr.c"

int					listenfd, n_tcp, n_udp, connfd, sockfd_udp, pkt_cnt;
long				RAND_ID;
pid_t				childpid;
socklen_t			clilen_tcp, clilen_udp;
struct sockaddr_in	servaddr, cliaddr;
struct sockaddr_storage	cliaddr_udp;
char				buff_tcp[MAXLINE], buff_udp[MAXLINE];
struct OWD_Record		OWDS[MAXPKT2];


void * response1(void * send_sd){
	struct Probe_Pkt * probe_pkt = buff_udp;
	struct Reply_Pkt * reply_pkt = buff_udp;
	struct timeval tv;
	pkt_cnt = 0;
	int sd = *(int *) send_sd;
	for ( ; ; ) {
		clilen_udp = sizeof(cliaddr_udp);
		n_udp = Recvfrom(sd, buff_udp, MAXLINE, 0, (SA *)&cliaddr_udp, &clilen_udp);
		if(RAND_ID == probe_pkt->ID){
			pkt_cnt ++;
			gettimeofday(&tv, NULL);
			reply_pkt->Send_arrive_time = tv.tv_sec * 1000000 + tv.tv_usec;
			reply_pkt->RSN = pkt_cnt;
			Sendto(sd, buff_udp, sizeof(struct Reply_Pkt), 0, (SA *)&cliaddr_udp, clilen_udp);
		}
	}
}

void * response2(void * send_sd){
	struct Probe_Pkt * probe_pkt = buff_udp;
	struct timeval tv;
	pkt_cnt = 0;
	int sd = *(int *) send_sd;
	for ( ; ; ) {
		clilen_udp = sizeof(cliaddr_udp);
		n_udp = Recvfrom(sd, buff_udp, MAXLINE, 0, (SA *)&cliaddr_udp, &clilen_udp);
		if(RAND_ID == probe_pkt->ID){
			pkt_cnt ++;
			gettimeofday(&tv, NULL);
			OWDS[pkt_cnt].SSN = probe_pkt->SSN;
			OWDS[pkt_cnt].OWD = (int)(tv.tv_sec * 1000000 + tv.tv_usec - probe_pkt->Send_time);
		}
	}
}

int main(int argc, char **argv)
{
	//int pageSize = sysconf(_SC_PAGE_SIZE);
	void *shared_addr = mmap(NULL, sizeof(int)*10, PROT_READ | PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0);
	int *isOccupied = 	shared_addr;
	isOccupied[0]   = 	0;
	void				sig_chld(int);

	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(19999);	/* TCP server */

	Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

	Listen(listenfd, LISTENQ);

	for ( ; ; ) {
		clilen_tcp = sizeof(cliaddr);
		if ( (connfd = accept(listenfd, (SA *) &cliaddr, &clilen_tcp)) < 0) {
			if (errno == EINTR)
				continue;		/* back to for() */
			else
				err_sys("accept error");
		}

		if ( (childpid = Fork()) == 0) {	/* child process */
			Close(listenfd);	/* close listening socket */
			if (isOccupied[0] == 0){
				isOccupied[0] = 1;
				snprintf(buff_tcp, sizeof(buff_tcp), ACCEPT);
				Write(connfd, buff_tcp, strlen(buff_tcp));
				if ((n_tcp = Read(connfd, buff_tcp, MAXLINE)) > 0){
					struct Task_Meta *	task_metadata = buff_tcp;
					int duration = task_metadata->duration;
					printf("ID=%ld\tpkt_num=%d\tduration=%d\tmode=%d\n", task_metadata->ID, task_metadata->pkt_num, task_metadata->duration, task_metadata->task_mode);
					RAND_ID = task_metadata->ID;
					sockfd_udp = Udp_server_reuseaddr(NULL, "19999", NULL);
					if(task_metadata->task_mode == 1){
						int nRecvBuf=32*1024; //set 32KB rcv buff
						setsockopt(sockfd_udp, SOL_SOCKET, SO_RCVBUF,(const char*)&nRecvBuf,sizeof(int));
						pthread_t response1_t;
						pthread_create(&response1_t, NULL, response1, &sockfd_udp);
						printf("I'm OK. Please send packet\n");
						snprintf(buff_tcp, sizeof(buff_tcp), "I'm OK. Please send packet\n");
						Write(connfd, buff_tcp, strlen(buff_tcp));
						us_sleep(duration + 1000000);
						// Kill the sending thread and receiving thread
						pthread_cancel(response1_t);
						int *pkt_cnt_received = buff_tcp;
						pkt_cnt_received[0] = pkt_cnt;
						//pkt_cnt_tmp[1] = 0;
						Write(connfd, buff_tcp, sizeof(int)); // send the number of received pkt.
					}else{
						int nRecvBuf=1024*1024; //set 1MB rcv buff
						setsockopt(sockfd_udp, SOL_SOCKET, SO_RCVBUF,(const char*)&nRecvBuf,sizeof(int));
						pthread_t response2_t;
						pthread_create(&response2_t, NULL, response2, &sockfd_udp);
						printf("I'm OK. Please send packet\n");
						snprintf(buff_tcp, sizeof(buff_tcp), "I'm OK. Please send packet\n");
						Write(connfd, buff_tcp, strlen(buff_tcp));
						us_sleep(duration + 1000000);
						// Kill the sending thread and receiving thread
						pthread_cancel(response2_t);
						int *pkt_cnt_received = buff_tcp;
						pkt_cnt_received[0] = pkt_cnt;
						//pkt_cnt_tmp[1] = 0;
						Write(connfd, pkt_cnt_received, sizeof(int)); // send the number of received pkt.
						/*
						int i;
						for(i=1;i<pkt_cnt+1;i++){
							printf("%d\t%d\n", OWDS[i].SSN, OWDS[i].OWD);
						}
						*/


					}
					close(sockfd_udp);
				}
				isOccupied[0] = 0;
			}else{
				snprintf(buff_tcp, sizeof(buff_tcp), DENY);
				Write(connfd, buff_tcp, strlen(buff_tcp));
			}
			exit(0);
		}
		Close(connfd);			/* parent closes connected socket */
	}
}
