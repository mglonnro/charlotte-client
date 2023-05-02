void
make_speed (char *buf, double spd);

void
make_nmea_speed_sentence (char *buf, char *timestamp, double spd);
void
make_nmea_wind_sentence (char *buf, char *timestamp, double awa, double aws, int is_true);
void
make_light_onoff(char *buf, char *timestamp, int on);
void
make_light_color (char *buf, char *timestamp, int r, int g, int b);
void
make_light_system_onoff(char *buf, char *timestamp, int on);

