#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"
#include "nmea_parser.h"

struct nmea_state state;
struct claim_state c_state;

int
get_nmea_state (char *message)
{
    struct nmea_state newstate;
    struct claim_state c_newstate;
    memset (&newstate, 0, sizeof (struct nmea_state));
    memset (&c_newstate, 0, sizeof (struct claim_state));

    cJSON *diff = build_diff (&newstate, &state, &c_newstate, &c_state);
    if (diff)
      {
	  char *p = cJSON_Print (diff);
	  sprintf (message, "%s", p);
	  free (p);
	  trim_message (message);
	  cJSON_Delete (diff);
	  return 1;
      }
    else
      {
	  return 0;
      }
}

int
parse_nmea (char *line, char *message)
{
    cJSON *json = cJSON_Parse (line);
    if (json == NULL)
      {
	  cJSON_Delete (json);
	  fprintf (stderr, "Couldn't parse json\n");
	  return 0;
      }
    cJSON *pgn = cJSON_GetObjectItemCaseSensitive (json, "pgn");
    if (pgn == NULL)
      {
	  cJSON_Delete (json);
	  fprintf (stderr, "Couldn't parse json\n");
	  return 0;
      }
    cJSON *ais = NULL;

    struct nmea_state newstate;
    memcpy (&newstate, &state, sizeof (state));
    struct claim_state c_newstate;
    memcpy (&c_newstate, &c_state, sizeof (c_state));

    /*
     * { name: "position", pgn: 129025, src: 127, parse: (o) => { return { lat:
     * o.fields.Latitude, lng: o.fields.Longitude } } }, // Position, Rapid
     * Update { name: "cogsog", pgn: 129026, src: 127, parse: (o) => { return {
     * COG: o.fields.COG, SOG: o.fields.SOG } } }, // COG & SOG, Rapid Update {
     * name: "depth", pgn: 128267, parse: (o) => { return { depth:
     * o.fields.Depth } } }, { name: "sea.temperature", pgn: 130311, parse: (o)
     * => { return { seatemp: o.fields.Temperature } } }, { name: "ais.static",
     * pgn: 129794, parse: (o) => { return { ais: "static", data: o.fields } }
     * }, { name: "ais", pgn: 129038, parse: (o) => { return { ais: "report",
     * data: o.fields } } }, { name: "ais", pgn: 129039, parse: (o) => { return
     * { ais: "report", data: o.fields } } },
     */

    if (pgn->valueint == 127250)
      {
	  //printf("PGN: %d %d\n", cJSON_IsString(pgn), pgn->valueint);
	  char *reference = get_field_value_string (json, "Reference");
	  if (reference)
	    {
		if (!strcmp (reference, "Magnetic"))
		  {
		      update_nmea_value (json, newstate.heading, "Heading");
		  }
		else
		  {
		      update_nmea_value (json, newstate.trueheading,
					 "Heading");
		  }
	    }
      }
    else if (pgn->valueint == 127258)
      {
	  update_nmea_value (json, newstate.variation, "Variation");
      }
    else if (pgn->valueint == 129025)
      {
	  update_nmea_value (json, newstate.lng, "Longitude");
	  update_nmea_value (json, newstate.lat, "Latitude");
      }
    else if (pgn->valueint == 129026)
      {
	  update_nmea_value (json, newstate.cog, "COG");
	  update_nmea_value (json, newstate.sog, "SOG");
      }
    else if (pgn->valueint == 128267)
      {
	  update_nmea_value (json, newstate.depth, "Depth");
      }
    else if (pgn->valueint == 127257)
      {
	  update_nmea_value (json, newstate.pitch, "Pitch");
	  update_nmea_value (json, newstate.roll, "Roll");
      }
    else if (pgn->valueint == 128259)
      {
	  update_nmea_value (json, newstate.speed, "Speed Water Referenced");
      }
    else if (pgn->valueint == 130306)
      {
	  update_nmea_value (json, newstate.awa, "Wind Angle");
	  update_nmea_value (json, newstate.aws, "Wind Speed");
      }
    else if (pgn->valueint == 60928)
      {
	  //printf("Got claim for src: %d\n", get_src(json));
	  update_nmea_claim (json, c_newstate.claims);

	  /*
	   * printf("CLAIM TABLE:\n"); for (int x = 0; x < MAXCLAIMS; x++) { if
	   * (c_newstate.claims[x].src != 0) { printf("  SRC %d => %s\n",
	   * c_newstate.claims[x].src, c_newstate.claims[x].unique_number); } }
	   */
	  save_state (&c_newstate);
      }
    else if (pgn->valueint == 126996)
      {
	  int src = get_src (json);
	  printf ("Device data a coming!\n");
	  char unique_number[DLENGTH];
	  int found = get_unique_number (src, &c_newstate, unique_number);

	  if (found)
	    {
		printf ("Updating device with unique number %s\n",
			unique_number);
		update_nmea_device (json, unique_number, c_newstate.devices);
	    }
	  else
	    {
		printf ("Got device but no unique number for src: %d\n", src);

	    }
	  printf ("DEVICE TABLE:\n");

	  for (int x = 0; x < MAXDEVICES; x++)
	    {
		if (c_newstate.devices[x].isactive)
		  {
		      printf ("  DEVICES %s => %s\n",
			      c_newstate.devices[x].unique_number,
			      c_newstate.devices[x].model_id);
		  }
	    }

	  save_state (&c_newstate);
      }
    cJSON *diff = build_diff (&state, &newstate, &c_state, &c_newstate);

    if (pgn->valueint == 129038 || pgn->valueint == 129039
	|| pgn->valueint == 129794)
      {
	  if (!diff)
	    {
		diff = cJSON_CreateObject ();
	    }
	  cJSON *values = cJSON_GetObjectItemCaseSensitive (json, "fields");
	  int user_id = get_ais_user_id (json);

	  if (user_id == -1)
	    {
		printf ("ERROR!");
		exit (1);
	    }
	  char key[256];
	  memset (key, 0, 256);
	  sprintf (key, "%d", user_id);

	  ais = cJSON_CreateObject ();

	  char *v = cJSON_Print (values);
	  cJSON *aisvalues = cJSON_Parse (v);

	  if (aisvalues)
	    {
		cJSON_AddItemToObject (ais, key, aisvalues);
	    }
	  free (v);

	  cJSON_AddItemToObject (diff, "ais", ais);
      }
    memcpy (&state, &newstate, sizeof (newstate));
    memcpy (&c_state, &c_newstate, sizeof (c_newstate));

    cJSON_Delete (json);

    if (diff)
      {
	  char *p = cJSON_Print (diff);
	  sprintf (message, "%s", p);
	  free (p);
	  trim_message (message);
	  cJSON_Delete (diff);

	  if (ais)
	    {
		ais = NULL;
	    }
	  return 1;
      }
    else
      {
	  return 0;
      }
}

void
trim_message (char *m)
{
    char buf[strlen (m) + 1];
    int i = 0;

    for (int x = 0; x < strlen (m); x++)
      {
	  if (m[x] != 10 && m[x] != 13 && m[x] != 9)
	    {
		buf[i++] = m[x];
	    }
      }

    buf[i] = 0;
    strcpy (m, buf);
}

cJSON *
build_diff (struct nmea_state *old_state, struct nmea_state *new_state,
	    struct claim_state *c_old_state, struct claim_state *c_new_state)
{
    cJSON *ret = cJSON_CreateObject ();
    int changes =
	diff_values (ret, old_state->heading, new_state->heading, "heading");
    changes +=
	diff_values (ret, old_state->variation, new_state->variation,
		     "variation");
    changes +=
	diff_values (ret, old_state->trueheading, new_state->trueheading,
		     "trueheading");
    changes += diff_values (ret, old_state->cog, new_state->cog, "cog");
    changes += diff_values (ret, old_state->sog, new_state->sog, "sog");
    changes += diff_values (ret, old_state->aws, new_state->aws, "aws");
    changes += diff_values (ret, old_state->awa, new_state->awa, "awa");
    changes += diff_values (ret, old_state->lng, new_state->lng, "lng");
    changes += diff_values (ret, old_state->lat, new_state->lat, "lat");
    changes += diff_values (ret, old_state->speed, new_state->speed, "speed");
    changes += diff_values (ret, old_state->depth, new_state->depth, "depth");
    changes += diff_values (ret, old_state->pitch, new_state->pitch, "pitch");
    changes += diff_values (ret, old_state->roll, new_state->roll, "roll");

    changes +=
	diff_claim_values (ret, c_old_state->claims, c_new_state->claims,
			   "claims");
    changes +=
	diff_device_values (ret, c_old_state->devices, c_new_state->devices,
			    "devices");

    if (changes)
      {
	  //printf("JSON: %s\n", cJSON_Print(ret));
	  return ret;
      }
    else
      {
	  cJSON_Delete (ret);
	  return NULL;
      }
}

int
diff_claim_values (cJSON * root, struct claim *old_arr, struct claim *new_arr,
		   char *fieldname)
{
    int diff = 0;
    cJSON *src = cJSON_CreateObject ();

    for (int n = 0; n < MAXCLAIMS; n++)
      {
	  if (new_arr[n].src == 0)
	    {
		continue;
	    }
	  int found = 0;
	  for (int o = 0; o < MAXCLAIMS; o++)
	    {
		if (new_arr[n].src == old_arr[o].src
		    && !strcmp (new_arr[n].unique_number,
				old_arr[o].unique_number))
		  {
		      found = 1;
		  }
	    }

	  if (!found)
	    {
		char key[10];
		sprintf (key, "%d", new_arr[n].src);
		cJSON_AddStringToObject (src, key, new_arr[n].unique_number);
		diff = 1;
	    }
      }

    if (diff)
      {
	  cJSON_AddItemToObject (root, fieldname, src);
      }
    else
      {
	  cJSON_Delete (src);
      }

    return diff;
}

int
diff_device_values (cJSON * root, struct device *old_arr,
		    struct device *new_arr, char *fieldname)
{
    int diff = 0;
    cJSON *src = cJSON_CreateObject ();

    for (int n = 0; n < MAXDEVICES; n++)
      {
	  int found = 0;
	  for (int o = 0; o < MAXDEVICES; o++)
	    {
		/* TODO: check for changes in data */
		if (!strcmp
		    (new_arr[n].unique_number, old_arr[o].unique_number))
		  {
		      found = 1;
		      break;
		  }
	    }

	  if (!found)
	    {
		cJSON *v = cJSON_CreateObject ();
		cJSON_AddStringToObject (v, "model_id", new_arr[n].model_id);
		cJSON_AddStringToObject (v, "model_version",
					 new_arr[n].model_version);
		cJSON_AddStringToObject (v, "software_version_code",
					 new_arr[n].software_version_code);
		cJSON_AddStringToObject (v, "model_serial_code",
					 new_arr[n].model_serial_code);

		char key[256];
		memset (key, 0, 256);
		sprintf (key, "%s", new_arr[n].unique_number);

		cJSON_AddItemToObject (src, key, v);
		diff = 1;
	    }
      }

    if (diff)
      {
	  cJSON_AddItemToObject (root, fieldname, src);
      }
    else
      {
	  cJSON_Delete (src);
      }

    return diff;
}



int
diff_values (cJSON * root, struct nmea_value *old_arr,
	     struct nmea_value *new_arr, char *fieldname)
{
    int diff = 0;
    cJSON *src = cJSON_CreateObject ();

    for (int n = 0; n < MAXSOURCES; n++)
      {
	  int found = 0;
	  for (int o = 0; o < MAXSOURCES; o++)
	    {
		if (new_arr[n].src == old_arr[o].src
		    && new_arr[n].value == old_arr[o].value)
		  {
		      found = 1;
		  }
	    }

	  if (!found)
	    {
		char key[10];
		memset (key, 0, 10);
		sprintf (key, "%d", new_arr[n].src);
		cJSON_AddNumberToObject (src, key, new_arr[n].value);
		diff = 1;
	    }
      }

    if (diff)
      {
	  cJSON_AddItemToObject (root, fieldname, src);
      }
    else
      {
	  cJSON_Delete (src);
      }

    return diff;
}

char *
get_field_value_string (cJSON * json, char *fieldname)
{
    cJSON *fields = cJSON_GetObjectItemCaseSensitive (json, "fields");
    if (fields)
      {
	  cJSON *v = cJSON_GetObjectItemCaseSensitive (fields, fieldname);
	  if (v)
	    {
		return v->valuestring;
	    }
      }
    return NULL;
}

int
get_src (cJSON * json)
{
    cJSON *src = cJSON_GetObjectItemCaseSensitive (json, "src");

    return src->valueint;
}

int
get_ais_user_id (cJSON * json)
{
    cJSON *fields = cJSON_GetObjectItemCaseSensitive (json, "fields");
    cJSON *v = cJSON_GetObjectItemCaseSensitive (fields, "User ID");

    if (v)
      {
	  return v->valueint;
      }
    return -1;
}

int
update_nmea_claim (cJSON * json, struct claim *arr)
{
    cJSON *fields = cJSON_GetObjectItemCaseSensitive (json, "fields");
    cJSON *src = cJSON_GetObjectItemCaseSensitive (json, "src");

    if (fields)
      {
	  cJSON *value =
	      cJSON_GetObjectItemCaseSensitive (fields, "Unique Number");
	  if (value)
	    {
		char unique_number[DLENGTH];
		strcpy (unique_number, value->valuestring);
		int s = src->valueint;

		//printf("Claim with unique number: %s\n", unique_number);

		/* Delete old value if any ,return if already exists */
		for (int x = 0; x < MAXCLAIMS; x++)
		  {
		      if (!strcmp (arr[x].unique_number, unique_number))
			{
			    if (arr[x].src == s)
			      {
				  return 0;
			      }
			    else
			      {
				  arr[x].src = 0;
			      }
			}
		  }

		for (int x = 0; x < MAXCLAIMS; x++)
		  {
		      if (arr[x].src == 0)
			{
			    arr[x].src = s;
			    strcpy (arr[x].unique_number, unique_number);
			    return 1;
			}
		  }
	    }
      }
    return 0;
}

int
get_unique_number (int src, struct claim_state *c, char *unique_number)
{
    for (int x = 0; x < MAXCLAIMS; x++)
      {
	  if (c->claims[x].src == src)
	    {
		strcpy (unique_number, c->claims[x].unique_number);
		return 1;
	    }
      }

    return 0;
}

int
update_nmea_device (cJSON * json, char *unique_number, struct device *arr)
{
    cJSON *fields = cJSON_GetObjectItemCaseSensitive (json, "fields");

    if (fields)
      {
	  cJSON *model_id =
	      cJSON_GetObjectItemCaseSensitive (fields, "Model ID");
	  cJSON *software_version_code =
	      cJSON_GetObjectItemCaseSensitive (fields,
						"Software Version Code");
	  cJSON *model_version =
	      cJSON_GetObjectItemCaseSensitive (fields, "Model Version");
	  cJSON *model_serial_code =
	      cJSON_GetObjectItemCaseSensitive (fields, "Model Serial Code");

	  if (model_id)
	    {
		/* Delete old value if any ,return if already exists */
		for (int x = 0; x < MAXDEVICES; x++)
		  {
		      if (arr[x].isactive
			  && !strcmp (arr[x].unique_number, unique_number))
			{
			    return 0;
			}
		  }

		for (int x = 0; x < MAXDEVICES; x++)
		  {
		      if (arr[x].isactive == 0)
			{
			    arr[x].isactive = 1;

			    strcpy (arr[x].unique_number, unique_number);
			    if (cJSON_IsString (model_id)
				&& model_id->valuestring != NULL)
			      {
				  strcpy (arr[x].model_id,
					  model_id->valuestring);
			      }
			    printf ("Adding! 2\n");

			    if (cJSON_IsString (model_version)
				&& model_version->valuestring != NULL)
			      {
				  strcpy (arr[x].model_version,
					  model_version->valuestring);
			      }
			    printf ("Adding! 2\n");

			    if (cJSON_IsString (software_version_code)
				&& software_version_code->valuestring != NULL)
			      {
				  strcpy (arr[x].software_version_code,
					  software_version_code->valuestring);
			      }
			    printf ("Adding! 2\n");

			    if (cJSON_IsString (model_serial_code)
				&& model_serial_code->valuestring != NULL)
			      {
				  strcpy (arr[x].model_serial_code,
					  model_serial_code->valuestring);
			      }
			    printf ("Adding! 2\n");

			    return 1;
			}
		  }
	    }
      }
    return 0;
}

/*
 * int nmea_ais_value(cJSON *json) { cJSON          *fields =
 * cJSON_GetObjectItemCaseSensitive(json, "fields"); cJSON          *user_id
 * = cJSON_GetObjectItemCaseSensitive(fields, "User ID");
 * 
 * cJSON *ret = cJSON_CreateObject();
 * 
 * const char * fs_string[] = { "Type of ship", "Destination",	"Name" "IMO
 * Number", "Callsign" };
 * 
 * const char * fs_double[] = { "Latitude", "Longitude", "SOG", "COG", "Length",
 * "Beam", "Draft", "Position reference from Starboard", "Position reference
 * from Bow" };
 * 
 * for (int x = 0; x < sizeof(fs_string); x++) {
 * 
 * } }
 */


int
update_nmea_value (cJSON * json, struct nmea_value *arr, char *fieldname)
{
    cJSON *fields = cJSON_GetObjectItemCaseSensitive (json, "fields");
    cJSON *src = cJSON_GetObjectItemCaseSensitive (json, "src");

    if (fields)
      {
	  cJSON *value = cJSON_GetObjectItemCaseSensitive (fields, fieldname);
	  if (value)
	    {
		//printf("  value %f\n", value->valuedouble);
		insert_or_replace (arr, src->valueint, value->valuedouble);

		for (int x = 0; x < MAXSOURCES; x++)
		  {
		      //printf("   %d src %d value %f\n", x, arr[x].src, arr[x].value);
		  }
	    }
      }
    return 1;
}

void
insert_or_replace (struct nmea_value *arr, int src, float value)
{
    int x = 0;

    for (x = 0; x < MAXSOURCES && arr[x].src != 0; x++)
      {
	  if (arr[x].src == src)
	    {
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

int
save_state (struct claim_state *c)
{
    FILE *outfile;

    //open file for writing
    outfile = fopen ("claim.state", "w");
    if (outfile == NULL)
      {
	  fprintf (stderr, "\nError opening file claim.state for writing!\n");
	  return 0;
      }
    else
      {
	  fwrite (c, sizeof (struct claim_state), 1, outfile);
	  fclose (outfile);
	  return 1;
      }
}

int
init_nmea_parser ()
{
    FILE *infile;
    infile = fopen ("claim.state", "r");
    if (infile != NULL)
      {
	  fread (&c_state, sizeof (struct claim_state), 1, infile);
	  fclose (infile);
      }
    return 1;
}
