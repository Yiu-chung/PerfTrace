#ifndef	__udp_owamp_h
#define	__udp_owamp_h

#include	<sys/types.h>	
#include	<stdio.h>	
#include	<time.h>
#include 	<pthread.h>
#include 	<sys/select.h>
#include 	<math.h>
#include	<stdlib.h>	
#include	<string.h>	
#include	<unistd.h>	
#include	<errno.h>		/* for definition of errno */
#include	<stdarg.h>		/* ANSI C header file */
#include 	<sys/socket.h>
#include 	<netinet/in.h>
#include	<arpa/inet.h>
#include 	<netdb.h>
#include	<sys/un.h>
#include	<limits.h>

#define MAXLINE	65536
#define	SA	struct sockaddr

/* Probe packet structure */
struct Probe_Pkt
{
	long ID;
	long Send_time;
	int TOT;
	int SSN;
};

/* Receive packet structure */
struct Reply_Pkt
{
	long ID;
	long Send_time;
	int TOT;
	int SSN;
	long Rcv_time;
	int RSN;
};

/* Raw data obtained by measurement */
struct Raw_Res
{
	long Send_time;
	long Rcv_time;
	long Reply_time;
	int TOT;
	int SSN;
	int RSN;
};

#define	min(a,b)	((a) < (b) ? (a) : (b))
#define	max(a,b)	((a) > (b) ? (a) : (b))

#endif  /* __udp_owamp_h */
