#ifndef _AISBOAT_H
#define _AISBOAT_H

#include "cJSON.h"

#define SLEN 256

struct aisboat {
  int mmsi;
  char name[SLEN];
  double heading, sog, cog, rot, latitude, longitude;
  double beam, length;
  char time[SLEN]; 
  unsigned long ms;
};

struct ais_list {
  struct aisboat boat;
  struct ais_list *prev;
  struct ais_list *next;
};

struct ais_list * create_ais_list() ;
void destroy_ais_list(struct ais_list *root);
cJSON *get_ais_state(struct ais_list *root);

#endif
