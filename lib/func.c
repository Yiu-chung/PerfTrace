#include "udp_owamp.h"

void us_sleep(int us)
{
	struct timeval ST;
	ST.tv_sec    = us / 1000000;
	ST.tv_usec   = us;
	select(0, NULL, NULL, NULL, &ST);
}

double    rate_atof(const char *s)
    {
	double    n;
	char      suffix = '\0';

	/* scan the number and any suffices */
	sscanf(s, "%lf%c", &n, &suffix);

	/* convert according to [Gg Mm Kk] */
	switch    (suffix)
	{
        case 'g': case 'G':
            n *= GIGA_RATE_UNIT;
            break;
        case 'm': case 'M':
            n *= MEGA_RATE_UNIT;
            break;
        case 'k': case 'K':
            n *= KILO_RATE_UNIT;
            break;
        default:
            break;
	}
	return n;
    }				/* end rate_atof */

int     duration_atoi(const char *s)
    {
	double    n;
	char      suffix = '\0';

	/* scan the number and any suffices */
	sscanf(s, "%lf%c", &n, &suffix);

	/* convert according to [s ms us] */
	switch    (suffix)
	{
        case 'm':
            n *= 1000;
            break;
        case 's':
            n *= 1000000;
            break;
        default:
            break;
	}
	return (int)n;
    }				/* end duration_atoi */


int  get_rate_and_duration(int * rate_and_duration){
    rate_and_duration[0] += 1;
    rate_and_duration[1] = 1;
    if(rate_and_duration[0] > 8) return 1;
    return 0;
}

int vsnd_init(){
    return 0;
}