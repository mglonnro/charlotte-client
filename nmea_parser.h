#include "cJSON.h"

struct nmea_value {
        float           value;
        int             src;
};


#define MAXSOURCES      5

struct nmea_state {
        struct nmea_value heading[MAXSOURCES];
        struct nmea_value variation[MAXSOURCES];
        struct nmea_value trueheading[MAXSOURCES];
        struct nmea_value sog[MAXSOURCES];
        struct nmea_value cog[MAXSOURCES];
        struct nmea_value aws[MAXSOURCES];
        struct nmea_value awa[MAXSOURCES];
        struct nmea_value lng[MAXSOURCES];
        struct nmea_value lat[MAXSOURCES];
        struct nmea_value speed[MAXSOURCES];
        struct nmea_value depth[MAXSOURCES];
        struct nmea_value pitch[MAXSOURCES];
        struct nmea_value roll[MAXSOURCES];
};

#define DLENGTH 256

struct nmea_sources {
	char	position[DLENGTH];
	char	attitude[DLENGTH];
	char	heading[DLENGTH];
	char	aws[DLENGTH];
	char	awa[DLENGTH];
};

struct claim {
        int     src;
        char    unique_number[DLENGTH];
};


struct device {
        int     isactive;
        char    unique_number[DLENGTH];
        char    model_id[DLENGTH];
        char    model_version[DLENGTH];
        char    software_version_code[DLENGTH];
        char    model_serial_code[DLENGTH];
};

#define MAXDEVICES      200
#define MAXCLAIMS       200

struct claim_state {
        struct device   devices[MAXDEVICES];
        struct claim    claims[MAXCLAIMS];
};

int	parse_nmea(char *line, char *message);
int     init_nmea_parser();

int             update_nmea_value(cJSON * json, struct nmea_value *arr, char *fieldname);
void            insert_or_replace(struct nmea_value *arr, int src, float value);
char           * get_field_value_string(cJSON * json, char *fieldname);
cJSON           *build_diff(struct nmea_state *old_state, struct nmea_state *new_state, struct claim_state *c_old_state, struct claim_state *c_new_state);
int diff_values(cJSON *root, struct nmea_value *old_arr, struct nmea_value *new_arr, char *fieldname);
void            trim_message(char *m);
int             get_unique_number(int src, struct claim_state *c, char *unique_number);
int             save_state(struct claim_state *c);
int      get_ais_user_id(cJSON *json);
int
get_nmea_state(char *message);
int
update_nmea_claim(cJSON * json, struct claim *arr);
int
get_src(cJSON *json);
int
update_nmea_device(cJSON * json, char *unique_number, struct device *arr);
int diff_claim_values(cJSON *root, struct claim *old_arr, struct claim *new_arr, char *fieldname);
int diff_device_values(cJSON *root, struct device *old_arr, struct device *new_arr, char *fieldname);
char *process_inbound_message(char *m);
