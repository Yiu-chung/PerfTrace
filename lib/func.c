#include "udp_owamp.h"

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