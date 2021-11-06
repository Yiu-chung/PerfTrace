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
#include	<getopt.h>
#include	<errno.h>		/* for definition of errno */
#include	<stdarg.h>		/* ANSI C header file */
#include 	<sys/socket.h>
#include 	<netinet/in.h>
#include	<arpa/inet.h>
#include 	<netdb.h>
#include	<sys/un.h>
#include	<limits.h>
#include	<sys/mman.h>    /* for mmap */

#define MAXLINE	1600
#define	LISTENQ 1024
#define MAXPKT1	1000
#define MAXPKT2 4096
#define	SA	struct sockaddr

const long KILO_RATE_UNIT = 1000;
const long MEGA_RATE_UNIT = 1000 * 1000;
const long GIGA_RATE_UNIT = 1000 * 1000 * 1000;
const char * ACCEPT = "accept";
const char * DENY = "deny";

/* Task Metadata */
struct Task_Meta
{
	long ID;
	int pkt_num;
	int duration; // unit: us
	int task_mode; // 1-basic; 2-ABM.
};


/* Probe packet structure */
struct Probe_Pkt
{
	long ID;
	long Send_time;
	int SSN;
};

/* Receive packet structure */
struct Reply_Pkt
{
	long ID;
	long Send_time;
	int SSN;  // send serial No.
	int RSN;  // reply serial No.
	long Send_arrive_time;
};

/* Raw data obtained by measurement */
struct Raw_Res1
{
	long Send_time;
	long Send_arrive_time;
	long Reply_arrive_time;
	int SSN;
	int RSN;
};

struct Raw_Res2
{
	long Send_time;
	int SSN;
	int OWD;
};

struct OWD_Record
{
	int SSN;
	int OWD;
};

struct Mode2_Reply_Header
{
	long ID;
	int offset;
	int size;
};

struct Mode2_Reply_Meta
{
	long ID;
	int pkt_num;
	int reply_num;
};




#define	min(a,b)	((a) < (b) ? (a) : (b))
#define	max(a,b)	((a) > (b) ? (a) : (b))

#endif  /* __udp_owamp_h */
