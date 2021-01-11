#include "./lib/udp_owamp.h"
#include "./lib/err_handle.c"
#include "./lib/wrapunix.c"
#include "./lib/udp_server_reuseaddr.c"
#include "./lib/sock_ntop.c"
char buff[MAXLINE];

/*
 * struct timeval
 * {
 * __time_t tv_sec;           // Seconds. 
 * __suseconds_t tv_usec;     // Microseconds. 
 * };
 */


/* Record the number of packages received during one measurement */
struct Pkt_Cnt{
	long ID;
	int Tot;
	int Cnt;
} pkt_cnt[2000];

/* struct Probe_Pkt: probe packet structure */

/* struct Reply_Pkt: receive packet structure */

int get_pkt_cnt(long id, int tot){
	int idx = (int)(id % 2000);
	while(id != pkt_cnt[idx].ID && abs(id - pkt_cnt[idx].ID) < 20000000){
		idx = (idx + 1) % 2000;
	}
	if(pkt_cnt[idx].ID != id){
		pkt_cnt[idx].ID = id;
		pkt_cnt[idx].Tot = tot;
		pkt_cnt[idx].Cnt = 1;
		return tot*10000 + 1;
	}else{
		pkt_cnt[idx].Cnt ++;
		if(pkt_cnt[idx].Cnt == tot){
			pkt_cnt[idx].ID = 0;
		}
		return tot*10000 + pkt_cnt[idx].Cnt;
	}
}

int main(int argc, char **argv)
{
	memset(pkt_cnt, 0, sizeof(pkt_cnt));
	int	sockfd, n;
	socklen_t	len;
	struct sockaddr_storage	cliaddr;
	struct timeval tv;
	if (argc == 2)
		sockfd = Udp_server_reuseaddr(NULL, argv[1], NULL);
	else if (argc == 3)
		sockfd = Udp_server_reuseaddr(argv[1], argv[2], NULL);
	else
		err_quit("usage: daytimeudpsrv [ <host> ] <service or port>");
	struct Probe_Pkt * probe_pkt = buff;
	struct Reply_Pkt * reply_pkt = buff;
	for ( ; ; ) {
		len = sizeof(cliaddr);
		n = Recvfrom(sockfd, buff, MAXLINE, 0, (SA *)&cliaddr, &len);
		gettimeofday(&tv, NULL);
		reply_pkt->Rcv_time = tv.tv_sec * 1000000 + tv.tv_usec;
		reply_pkt->RSN = get_pkt_cnt(probe_pkt->ID, (probe_pkt->SSN)/10000);
		Sendto(sockfd, buff, sizeof(struct Reply_Pkt), 0, (SA *)&cliaddr, len);
		printf("reply_pkt->ID : datagram from %s\n", Sock_ntop((SA *)&cliaddr, len));
	}
}
