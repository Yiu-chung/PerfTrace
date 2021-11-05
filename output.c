#include "./lib/udp_owamp.h"

/* Output the raw data */
void print_raw_data(struct Raw_Res * res, int tot){
	printf("SSN       RSN       Delay1    Delay2    RTT       Send_time           Rcv_time            Reply_time\n");
	int i;
	for(i = 0; i < tot; i++){
		printf("%-10d%-10d%-10.3f%-10.3f%-10.3f%-20ld%-20ld%-20ld\n", i+1, res[i].RSN, (res[i].Rcv_time-res[i].Send_time)/1000.0, (res[i].Reply_time-res[i].Rcv_time)/1000.0, (res[i].Reply_time-res[i].Send_time)/1000.0,res[i].Send_time, res[i].Rcv_time, res[i].Reply_time);
	}
}

/* Calculate one_way delay and two_way delay using raw data */
void delay_calc(struct Raw_Res * res, int tot){
	int tw[3], ow1[3], ow2[3]; // min/aver/max
	int cnt = 0;
	int i = 0;
	while(i<tot && res[i].SSN == 0){
		i++;
	}
	if(i<tot){
		tw[0] = tw[1] = tw[2] = res[i].Reply_time-res[i].Send_time;
		ow1[0] = ow1[1] = ow1[2] = res[i].Rcv_time-res[i].Send_time;
		ow2[0] = ow2[1] = ow2[2] = res[i].Reply_time-res[i].Rcv_time;
		cnt ++;
	}	
	i ++;
	for(; i<tot; i++){
		if(res[i].SSN != 0){
			tw[0] = min(tw[0], res[i].Reply_time-res[i].Send_time);
			tw[1] += res[i].Reply_time-res[i].Send_time;	
			tw[2] = max(tw[2], res[i].Reply_time-res[i].Send_time);
			
			ow1[0] = min(ow1[0], res[i].Rcv_time-res[i].Send_time); 
			ow1[1] += res[i].Rcv_time-res[i].Send_time;
			ow1[2] = max(ow1[2], res[i].Rcv_time-res[i].Send_time);
							
			ow2[0] = min(ow2[0], res[i].Reply_time-res[i].Rcv_time); 
			ow2[1] += res[i].Reply_time-res[i].Rcv_time;
			ow2[2] = max(ow2[2], res[i].Reply_time-res[i].Rcv_time);
	
			cnt ++;
		}
	}
	if(cnt == 0){
		printf("All probe packets are lost\n");
	}else{
		printf("Two way delay:\n");
		printf("    min/aver/max = %.3f/%.3f/%.3f ms\n",tw[0]/1000.0, tw[1]/1000.0/cnt, tw[2]/1000.0);
		printf("One way delay:\n");
		printf("    Source->Dest: min/aver/max = %.3f/%.3f/%.3f ms\n",ow1[0]/1000.0, ow1[1]/1000.0/cnt, ow1[2]/1000.0);
		printf("    Dest->Source: min/aver/max = %.3f/%.3f/%.3f ms\n",ow2[0]/1000.0, ow2[1]/1000.0/cnt, ow2[2]/1000.0);
	}
}

/* Old version: Caculate average jitter
void jitter_calc(struct Raw_Res * res, int tot){
	int cnt = 0;
	float twj, owj1, owj2;
	twj = owj1 = owj2 = 0.0;
	int pre_tw, pre_ow1, pre_ow2;
	int i=0;
	while(i < tot && res[i].SSN == 0){
		i++;
	}
	if(i<tot){
		pre_tw = res[i].Reply_time-res[i].Send_time;
        	pre_ow1 = res[i].Rcv_time-res[i].Send_time;
        	pre_ow2 = res[i].Reply_time-res[i].Rcv_time;
		i++;
		cnt++;
	}
	if(cnt){
		for(; i<tot; i++){
			if(res[i].SSN){
				twj += abs((int)(res[i].Reply_time-res[i].Send_time) - pre_tw);
				owj1 += abs((int)(res[i].Rcv_time-res[i].Send_time) - pre_ow1);
				owj2 += abs((int)(res[i].Reply_time-res[i].Rcv_time) - pre_ow2);
				pre_tw = res[i].Reply_time-res[i].Send_time;
				pre_ow1 = res[i].Rcv_time-res[i].Send_time;
				pre_ow2 = res[i].Reply_time-res[i].Rcv_time;
				cnt ++;
			}
		}
	}
	if(cnt>1){
		printf("Two way jitter:\n");
		printf("    %.3f ms\n", twj/(cnt*1000.0-1000.0));
		printf("One way jitter:\n");
		printf("    Source->Dest: %.3f ms\n", owj1/(cnt*1000.0-1000.0));
		printf("    Dest->Source: %.3f ms\n", owj2/(cnt*1000.0-1000.0));
	}

}
*/


/* Calculate one_way jitter and two_way jitter using raw data */
/* According to rfc1889 : https://www.ietf.org/rfc/rfc1889.txt */
void jitter_calc(struct Raw_Res * res, int tot){
	int cnt = 0;
	float twj, owj1, owj2;
	twj = owj1 = owj2 = -1.0;
	int pre_tw, pre_ow1, pre_ow2;
	int i=0;
	while(i < tot && res[i].SSN == 0){
		i++;
	}
	if(i<tot){
		pre_tw = res[i].Reply_time-res[i].Send_time;
        	pre_ow1 = res[i].Rcv_time-res[i].Send_time;
        	pre_ow2 = res[i].Reply_time-res[i].Rcv_time;
		i++;
		cnt++;
	}
	if(cnt){
		for(; i<tot; i++){
			if(res[i].SSN){
				cnt ++;
				if(twj < -0.9){
					twj = abs((int)(res[i].Reply_time-res[i].Send_time) - pre_tw);
					owj1 = abs((int)(res[i].Rcv_time-res[i].Send_time) - pre_ow1);
					owj2 = abs((int)(res[i].Reply_time-res[i].Rcv_time) - pre_ow2);
				}else{
					// J=J+(|D(i-1,i)|-J)/16
					twj = twj + (abs((int)(res[i].Reply_time-res[i].Send_time) - pre_tw) - twj)/16.0;
					owj1 = owj1 + (abs((int)(res[i].Rcv_time-res[i].Send_time) - pre_ow1) - owj1)/16.0;
					owj2 = owj2 + (abs((int)(res[i].Reply_time-res[i].Rcv_time) - pre_ow2) - owj2)/16.0;
				}
				pre_tw = res[i].Reply_time-res[i].Send_time;
				pre_ow1 = res[i].Rcv_time-res[i].Send_time;
				pre_ow2 = res[i].Reply_time-res[i].Rcv_time;
			}
		}
	}
	if(cnt>1){
		printf("Two way jitter:\n");
		printf("    %.3f ms\n", twj/1000.0);
		printf("One way jitter:\n");
		printf("    Source->Dest: %.3f ms\n", owj1/1000.0);
		printf("    Dest->Source: %.3f ms\n", owj2/1000.0);
	}

}

/* Calculate one_way loss rate and two_way loss rate using raw data */
float loss_rate_calc(struct Raw_Res * res, int tot, int received){
	void print_loss_rate(float tw, float ow1, float ow2) 
	{
		printf("Two way packet loss rate:\n    %-4.2f%%\n",tw*100);
		printf("One way packet loss rate:\n");
		printf("    Source->Dest: %-4.2f%%\n    Dest->Source: %-4.2f%%\n", ow1*100, ow2*100);
	}
	float tw_loss_rate, ow_loss_rate1, ow_loss_rate2;
	tw_loss_rate = ow_loss_rate1 = ow_loss_rate2 = 0.0;
	int reply_cnt;
	reply_cnt = 0;
	int i;
	for(i = 0; i < tot; i++){
		if(res[i].SSN != 0){
			reply_cnt ++;
		}
	}
	tw_loss_rate = (float)(tot - reply_cnt) / tot;
	ow_loss_rate1 = (float)(tot - received) / tot;
	if(received > 0) ow_loss_rate2 = (float)(received - reply_cnt) / received;
	print_loss_rate(tw_loss_rate, ow_loss_rate1, ow_loss_rate2);
	return ow_loss_rate1;
}

/* Calculate available bandwidth */
void abw_calc(struct Raw_Res * res, int tot, int probe_size, int receive){
	int i;
	//long min_sendtime = LONG_MAX;
	long min_rcvtime = LONG_MAX;
	long max_rcvtime = 0;
	int cnt = 0;
	for(i = 0; i < tot; i ++){
		if(res[i].SSN != 0){
			cnt ++;
			//min_sendtime = min(min_sendtime, res[i].Send_time);
			min_rcvtime = min(min_rcvtime, res[i].Rcv_time);
			max_rcvtime = max(max_rcvtime, res[i].Rcv_time);
		}
	}
	float ow1_loss_rate = loss_rate_calc(res, tot, receive);
	//printf("%ld  %.3f\n",max_rcvtime-min_rcvtime, (float)(max_rcvtime-min_rcvtime));
	if(cnt > 1){
		printf("Available bandwidth:\n");
		printf("    %.3f bps\n", (cnt-1)*probe_size*8*1000000.0/(float)(max_rcvtime-min_rcvtime)*(1.0-ow1_loss_rate)/(tot*ow1_loss_rate+1.0));
	}

}
