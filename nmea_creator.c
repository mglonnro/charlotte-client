#include <stdio.h>
#include <string.h>
#include <math.h>
#include "nmea_creator.h"

#define TMPBUFSIZE	256

void
make_speed (char *buf, double spd)
{
    spd = spd * 100;
    int dec = (int) spd % 256;

    sprintf (buf, "%02X,%02X", dec, (int) ((spd - dec) / 256));
}

void
make_angle (char *buf, double angle)
{
    angle = angle * (M_PI / 180.0);
    angle = angle * 10000;

    int dec = (int) angle % 256;

    sprintf (buf, "%02X,%02X", dec, (int) ((angle - dec) / 256));
}

void
make_nmea_speed_sentence (char *buf, char *timestamp, double spd)
{
    char tmp_buf[TMPBUFSIZE];
    memset (tmp_buf, 0, TMPBUFSIZE);
    make_speed (tmp_buf, spd);

    sprintf (buf, "%s,2,128259,0,255,8,ff,%s,ff,ff,00,ff,ff", timestamp,
	     tmp_buf);
}

void
make_nmea_wind_sentence (char *buf, char *timestamp, double wa, double ws,
			 int is_true)
{
    char tmp1[TMPBUFSIZE], tmp2[TMPBUFSIZE];
    memset (tmp1, 0, TMPBUFSIZE);
    memset (tmp2, 0, TMPBUFSIZE);
    make_angle (tmp1, wa);
    make_speed (tmp2, ws);

    sprintf (buf, "%s,2,130306,0,255,8,00,%s,%s,%s,ff,ff", timestamp,
	     tmp2, tmp1, (is_true ? "fb" : "fa"));
}

void
make_light_onoff (char *buf, char *timestamp, int on)
{
    /* 2023-02-15T21:02:06.747Z,2,126208,0,92,13,01,01,fe,01,ff,03,01,02,02,02,01,0d,1 */
    /* 2023-02-15T21:02:06.747Z,2,126208,0,92,13,01,01,fe,01,ff,03,01,02,02,02,01,0d,0 */

    sprintf (buf,
	     "%s,2,126208,0,92,13,01,01,fe,01,ff,03,01,02,02,02,01,0d,%d",
	     timestamp, on);
}

void
make_light_color (char *buf, char *timestamp, int r, int g, int b)
{
    /* 2,126208,0,92,19,01,01,fe,01,ff,04,01,02,02,02,01,03,ff,04,ff,05,ff,0d,1 */

    sprintf (buf,
	     "%s,2,126208,0,92,19,01,01,fe,01,ff,04,01,02,02,02,01,03,%02X,04,%02X,05,%02X,0d,1",
	     timestamp, r, g, b);
}

void
make_light_system_onoff (char *buf, char *timestamp, int on)
{
    /* 2023-02-15T21:02:06.747Z,2,126208,0,92,08,01,1a,fd,01,ff,01,01,01 */

    sprintf (buf,
	     "%s,2,126208,0,92,08,01,1a,fd,01,ff,01,01,%d", timestamp, on);
}
