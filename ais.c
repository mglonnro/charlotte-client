#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "ais.h"
#include "cJSON.h"
#include "epoch.h"

struct ais_list *
create_ais_list ()
{
    struct ais_list *root =
	(struct ais_list *) malloc (sizeof (struct ais_list));
    memset (root, 0, sizeof (struct ais_list));
    return root;
}

void
destroy_ais_list (struct ais_list *root)
{
    struct ais_list *ptr = root;

    while (ptr != NULL)
      {
	  struct ais_list *next = ptr->next;
	  free (ptr);
	  ptr = next;
      }
}

int
get_ais_list_count (struct ais_list *root)
{
    int c = 0;

    struct ais_list *ptr = root;
    while (ptr != NULL)
      {
	  c++;
	  ptr = ptr->next;
      }

    return c;
}

struct ais_list *
add_or_update_ais (struct ais_list *root, int user_id, cJSON * values)
{
    struct ais_list *ptr = root;

    if (root == NULL)
      {
	  root = create_ais_list ();
	  ptr = root;
      }
    else
      {

	  int c = 0, updating = 0;
	  while (1)
	    {
		if (ptr->boat.mmsi == user_id)
		  {
		      updating = 1;
		      break;
		  }

		c++;
		if (ptr->next != NULL)
		  {
		      ptr = ptr->next;
		  }
		else
		  {
		      break;
		  }
	    }

	  if (!updating)
	    {
		struct ais_list *item =
		    (struct ais_list *) malloc (sizeof (struct ais_list));
		memset (item, 0, sizeof (struct ais_list));
		ptr->next = item;
		ptr = item;
	    }
      }

    ptr->boat.mmsi = user_id;

    char buf[50];
    sprintf (buf, "%d", user_id);
    update_ais_value (values, "User ID", buf);
    update_ais_value (values, "Name", ptr->boat.name);
    update_ais_value (values, "Heading", &(ptr->boat.heading));
    update_ais_value (values, "SOG", &(ptr->boat.sog));
    update_ais_value (values, "COG", &(ptr->boat.cog));
    update_ais_value (values, "Rate of Turn", &(ptr->boat.rot));
    update_ais_value (values, "Beam", &(ptr->boat.beam));
    update_ais_value (values, "Length", &(ptr->boat.length));
    update_ais_value (values, "Latitude", &(ptr->boat.latitude));
    update_ais_value (values, "Longitude", &(ptr->boat.longitude));

    ptr->boat.ms = getMillisecondsSinceEpoch ();

    /* Add current timestmap */
    char tbuf[31];
    memset (tbuf, 0, 31);
    getUTCTimestamp (tbuf);
    strcpy (ptr->boat.time, tbuf);

    cJSON *tmp = get_ais_state (root);
    if (tmp)
      {
	  char *v = cJSON_Print (tmp);
	  /* fprintf (stderr, "AIS STATE: %s\n", v); */
	  free (v);
	  cJSON_Delete (tmp);
      }
    return root;
}

cJSON *
get_ais_state (struct ais_list *root)
{
    cJSON *state = cJSON_CreateObject ();

    struct ais_list *ptr = root;
    while (ptr != NULL)
      {
	  cJSON *boat = cJSON_CreateObject ();

	  cJSON_AddNumberToObject (boat, "User ID", ptr->boat.mmsi);

	  if (ptr->boat.name[0])
	    {
		cJSON_AddStringToObject (boat, "Name", ptr->boat.name);
	    }

	  if (ptr->boat.beam != 0 && ptr->boat.length != 0)
	    {
		cJSON_AddNumberToObject (boat, "Beam", ptr->boat.beam);
		cJSON_AddNumberToObject (boat, "Length", ptr->boat.length);
	    }

	  cJSON_AddNumberToObject (boat, "Heading", ptr->boat.heading);
	  cJSON_AddNumberToObject (boat, "COG", ptr->boat.cog);
	  cJSON_AddNumberToObject (boat, "SOG", ptr->boat.sog);
	  cJSON_AddNumberToObject (boat, "Rate of Turn", ptr->boat.rot);
	  cJSON_AddNumberToObject (boat, "Latitude", ptr->boat.latitude);
	  cJSON_AddNumberToObject (boat, "Longitude", ptr->boat.longitude);
	  cJSON_AddStringToObject (boat, "time", ptr->boat.time);

	  char key[256];
	  memset (key, 0, 256);
	  sprintf (key, "%d", ptr->boat.mmsi);
	  cJSON_AddItemToObject (state, key, boat);
	  ptr = ptr->next;
      }

    return state;
}

void
update_ais_value (cJSON * fields, char *field_name, void *dest)
{
    cJSON *value = cJSON_GetObjectItemCaseSensitive (fields, field_name);
    if (value)
      {
	  if (cJSON_IsNumber (value))
	    {
		*((double *) dest) = value->valuedouble;
	    }
	  else if (cJSON_IsString (value))
	    {
		strcpy ((char *) dest, value->valuestring);
	    }
	  else
	    {
		fprintf (stderr, "WHAT?!\n");
	    }
      }
}
