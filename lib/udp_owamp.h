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
#define MAXPSIZE 1392
#define	LISTENQ 1024
#define MAXPKT1	1000
#define MAXPKT2 10240
#define MAXITER 10
#define MAXRATE 2 * 1000 * 1000 * 1000
#define DURATION 20000
#define FACTOR 1.5
#define THRESHOLD 1.05
#define MAX_LIMIT 200

#define	SA	struct sockaddr

const long KILO_RATE_UNIT = 1000;
const long MEGA_RATE_UNIT = 1000 * 1000;
const long GIGA_RATE_UNIT = 1000 * 1000 * 1000;
const char * ACCEPT = "accept";
const char * DENY = "deny";
const char * MODE2_MEASURE = "measure";
const char * MODE2_END = "end";
const char * DATA_BASE = "./data/";

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
	int RSN;
	int OWD;
};

struct OWD_Record
{
	long send_time;
	int SSN;
	int OWD;
};

struct Mode2_Reply_Header
{
	long ID;
	int pkt_tot;
	int offset;
	int size;
};

struct Mode2_Reply_Meta
{
	long ID;
	int pkt_num;
	int reply_num;
};

struct Mode2_Send_Meta
{
	int pkt_num_send;
	int psize;
	int duration;
};

struct Mode2_Result
{
	float loss_rate;
	float rate1; //Receiving rate of all packets
	float rate2; //Receiving rate in duration
};




#define	min(a,b)	((a) < (b) ? (a) : (b))
#define	max(a,b)	((a) > (b) ? (a) : (b))

#endif  /* __udp_owamp_h */
