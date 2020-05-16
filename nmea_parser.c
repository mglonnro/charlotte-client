#include <stdio.h>
#include <string.h>
#include "cJSON.h"


struct nmea_value {
	float		value;
	int		src;
};

#define MAXSOURCES	5

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
};

int		update_nmea_value(cJSON * json, struct nmea_value *arr, char *fieldname);
void		insert_or_replace(struct nmea_value *arr, int src, float value);
char           * get_field_value_string(cJSON * json, char *fieldname);
cJSON *build_diff(struct nmea_state *old_state, struct nmea_state *new_state);
int diff_values(cJSON *root, struct nmea_value *old_arr, struct nmea_value *new_arr, char *fieldname);
void 		trim_message(char *m);


struct nmea_state state;

int
parse_nmea(char *line, char *message)
{
	cJSON          *json = cJSON_Parse(line);

	//printf("JSON: %s\n", cJSON_Print(json));

	cJSON          *pgn = cJSON_GetObjectItemCaseSensitive(json, "pgn");

	struct nmea_state newstate;
	memcpy(&newstate, &state, sizeof(state));

 	/*
 	 { name: "position", pgn: 129025, src: 127, parse: (o) => { return { lat: o.fields.Latitude, lng: o.fields.Longitude } } }, // Position, Rapid Update
        { name: "cogsog", pgn: 129026, src: 127, parse: (o) => { return { COG: o.fields.COG, SOG: o.fields.SOG } } }, // COG & SOG, Rapid Update
        { name: "depth", pgn: 128267, parse: (o) => { return { depth: o.fields.Depth } } },
        { name: "sea.temperature", pgn: 130311, parse: (o) => { return { seatemp: o.fields.Temperature } } },
        { name: "ais.static", pgn: 129794, parse: (o) => { return { ais: "static", data: o.fields } } },
        { name: "ais", pgn: 129038, parse: (o) => { return { ais: "report", data: o.fields } } },
        { name: "ais", pgn: 129039, parse: (o) => { return { ais: "report", data: o.fields } } },
	*/

	if (pgn->valueint == 127250) {
		//printf("PGN: %d %d\n", cJSON_IsString(pgn), pgn->valueint);
		char           *reference = get_field_value_string(json, "Reference");
		if (reference) {
			if (!strcmp(reference, "Magnetic")) {
				update_nmea_value(json, newstate.variation, "Variation");
				update_nmea_value(json, newstate.heading, "Heading");
			} else {
				update_nmea_value(json, newstate.trueheading, "Heading");
			}
		}
	} else if (pgn->valueint == 129025) {
		update_nmea_value(json, newstate.lng, "Longitude");
		update_nmea_value(json, newstate.lat, "Latitude");
	} else if (pgn->valueint == 129026) {
		update_nmea_value(json, newstate.cog, "COG");
		update_nmea_value(json, newstate.sog, "SOG");
	} else if (pgn->valueint == 128267) {
		update_nmea_value(json, newstate.depth, "Depth");
	} else if (pgn->valueint == 128259) {
		update_nmea_value(json, newstate.speed, "Speed Water Referenced");
	} else if (pgn->valueint == 130306) {
		update_nmea_value(json, newstate.awa, "Wind Angle");
		update_nmea_value(json, newstate.aws, "Wind Speed");
	}

	cJSON *diff = build_diff(&state, &newstate);

	memcpy(&state, &newstate, sizeof(newstate));

	cJSON_Delete(json);

	if (diff) {
		sprintf(message, "%s", cJSON_Print(diff));
		trim_message(message);
		cJSON_Delete(diff);
		return 1;
	} else {
		return 0;
	}
}

void
trim_message(char *m) 
{
  char buf[strlen(m)+1];
  int i = 0;

  for (int x = 0; x < strlen(m); x++) {
    if (m[x] != 10 && m[x] != 13 && m[x] != 9) {
	buf[i++] = m[x];
    }
  }

  buf[i] = 0;
  strcpy(m, buf);
}

cJSON *build_diff(struct nmea_state *old_state, struct nmea_state *new_state) {
	cJSON *ret = cJSON_CreateObject();
	int changes = diff_values(ret, old_state->heading, new_state->heading, "heading");
	changes += diff_values(ret, old_state->variation, new_state->variation, "variation");
	changes += diff_values(ret, old_state->trueheading, new_state->trueheading, "trueheading");
	changes += diff_values(ret, old_state->cog, new_state->cog, "cog");
	changes += diff_values(ret, old_state->sog, new_state->sog, "sog");
	changes += diff_values(ret, old_state->aws, new_state->aws, "aws");
	changes += diff_values(ret, old_state->awa, new_state->awa, "awa");
	changes += diff_values(ret, old_state->lng, new_state->lng, "lng");
	changes += diff_values(ret, old_state->lat, new_state->lat, "lat");
	changes += diff_values(ret, old_state->speed, new_state->speed, "speed");
	changes += diff_values(ret, old_state->depth, new_state->depth, "depth");

	if (changes) {
	  printf("JSON: %s\n", cJSON_Print(ret));
	  return ret;
	}

	return NULL;
}

int diff_values(cJSON *root, struct nmea_value *old_arr, struct nmea_value *new_arr, char *fieldname) {
  int diff = 0;
  cJSON *src = cJSON_CreateObject();
  
  for (int n = 0; n < MAXSOURCES; n++) {
    int found = 0;
    for (int o = 0; o < MAXSOURCES; o++) {
      if (new_arr[n].src == old_arr[o].src && new_arr[n].value == old_arr[o].value) {
	found = 1;
      }
    }

    if (!found) {
      char key[10];
      sprintf(key, "%d", new_arr[n].src);
      cJSON_AddNumberToObject(src, key, new_arr[n].value);
      diff = 1;
    }
  }

  if (diff) {
      cJSON_AddItemToObject(root, fieldname, src);
  }

  return diff;
}

char           *
get_field_value_string(cJSON * json, char *fieldname)
{
	cJSON          *fields = cJSON_GetObjectItemCaseSensitive(json, "fields");
	if (fields) {
		cJSON          *v = cJSON_GetObjectItemCaseSensitive(fields, fieldname);
		if (v) {
			return v->valuestring;
		}
	}
	return NULL;
}


int 
update_nmea_value(cJSON * json, struct nmea_value *arr, char *fieldname)
{
	cJSON          *fields = cJSON_GetObjectItemCaseSensitive(json, "fields");
	cJSON          *src = cJSON_GetObjectItemCaseSensitive(json, "src");

	if (fields) {
		cJSON          *value = cJSON_GetObjectItemCaseSensitive(fields, fieldname);
		if (value) {
			//printf("  value %f\n", value->valuedouble);
			insert_or_replace(arr, src->valueint, value->valuedouble);

			for (int x = 0; x < MAXSOURCES; x++) {
				//printf("   %d src %d value %f\n", x, arr[x].src, arr[x].value);
			}
		}
	}
	return 1;
}

void 
insert_or_replace(struct nmea_value *arr, int src, float value)
{
	int		found = 0;
	int		x = 0;

	for (x = 0; x < MAXSOURCES && arr[x].src != 0; x++) {
		if (arr[x].src == src) {
			//printf("  update existing %d\n", src);
			arr[x].value = value;
			return;
		}
	}

	//Add
		static struct nmea_value v;
	v.src = src;
	v.value = value;
	arr[x] = v;
}
