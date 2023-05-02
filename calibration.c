#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"
#include "calibration.h"
#include "epoch.h"

#define SAVEFILE "calibration.state"
static struct calib c;

void
init_calib ()
{
    memset (&c, 0, sizeof (struct calib));
    FILE *in = fopen (SAVEFILE, "r");

    if (in)
      {
	  fread (&c, sizeof (struct calib), 1, in);
	  fclose (in);
      }
}

void
save_calib ()
{
    FILE *out = fopen (SAVEFILE, "w");
    if (out)
      {
	  fwrite (&c, sizeof (struct calib), 1, out);
	  fclose (out);
      }
}

struct calib_item *
get_calib_item (char *key)
{
    for (int x = 0; x < MAXITEMS; x++)
      {
	  if (c.items[x].key[0] && !strcmp (c.items[x].key, key))
	    {
		return &c.items[x];
	    }
      }

    return NULL;
}

double
apply_calib_item (struct calib_item *a, double src_value)
{
    if (!a->fixed_isnull)
      {
	  if (!strcmp (a->key, CALIB_SPEED))
	    {
		return src_value * a->fixed;
	    }
	  else if (!strcmp (a->key, CALIB_AWA))
	    {
		return src_value + a->fixed;
	    }
      }
    else if (!a->linear_k_isnull)
      {
	  if (!strcmp (a->key, CALIB_SPEED))
	    {
		/* Ugly hack #fixme */
		if (src_value < 0.1)
		  {
		      return src_value;
		  }
	    }

	  /*fprintf (stderr, "k: %f x: %f src: %f\n", a->linear_k, a->linear_x,
	     src_value);
	   */
	  double KT = 1.94384;
	  double m = a->linear_k + src_value * KT * a->linear_x;
	  return src_value * m;
      }

    return src_value;
}

double
get_calib (char *key, double src_value)
{
    struct calib_item *a = get_calib_item (key);

    if (a != NULL)
      {
	  return apply_calib_item (a, src_value);
      }
    else
      {
	  return src_value;
      }
}

cJSON *
get_calib_state ()
{
    cJSON *root = cJSON_CreateObject ();
    cJSON *calib = cJSON_CreateObject ();
    cJSON_AddItemToObject (root, "calibration", calib);

    int x;

    for (x = 0; x < MAXITEMS; x++)
      {
	  if (!c.items[x].key[0])
	    {
		break;
	    }

	  cJSON *data = cJSON_AddObjectToObject (calib, c.items[x].key);
	  cJSON_AddItemToObject (data, "fixed",
				 cJSON_CreateNumber (c.items[x].fixed));
	  cJSON_AddItemToObject (data, "linear_x",
				 cJSON_CreateNumber (c.items[x].linear_x));
	  cJSON_AddItemToObject (data, "linear_k",
				 cJSON_CreateNumber (c.items[x].linear_k));

      }

    if (x == 0)
      {
	  /* Nothing found */
	  cJSON_Delete (root);
	  return NULL;
      }

    cJSON_AddItemToObject (root, "time", cJSON_CreateNumber (c.time));

    return root;		/* caller responsible for free */
}

void
copy_calib_values (cJSON * obj, struct calib_item *i)
{
    char *strp = cJSON_Print (obj);
    fprintf (stderr, "parent: %s\n", strp);
    free (strp);


    cJSON *s = cJSON_GetObjectItemCaseSensitive (obj,
						 "fixed");
    if (s != NULL)
      {
	  if (!cJSON_IsNull (s))
	    {
		i->fixed = s->valuedouble;
		i->fixed_isnull = 0;

		fprintf (stderr, "Setting %s fixed to %f\n",
			 obj->string, s->valuedouble);
	    }
	  else
	    {
		fprintf (stderr, "Setting %s fixed to null\n", obj->string);
		i->fixed_isnull = 1;
	    }
      }
    s = cJSON_GetObjectItemCaseSensitive (obj, "linear_x");
    if (s != NULL)
      {
	  if (!cJSON_IsNull (s))
	    {
		i->linear_x = s->valuedouble;
		i->linear_x_isnull = 0;
		fprintf (stderr,
			 "Setting %s linear_x to %f\n",
			 obj->string, s->valuedouble);
	    }
	  else
	    {
		fprintf (stderr, "Setting %s linear_x to null\n",
			 obj->string);
		i->linear_x_isnull = 1;
	    }
      }

    s = cJSON_GetObjectItemCaseSensitive (obj, "linear_k");
    if (s != NULL)
      {
	  if (!cJSON_IsNull (s))
	    {
		i->linear_k = s->valuedouble;
		i->linear_k_isnull = 0;
		fprintf (stderr,
			 "Setting %s linear_k to %f\n",
			 obj->string, s->valuedouble);
	    }
	  else
	    {
		fprintf (stderr, "Setting %s linear_k to null\n",
			 obj->string);
		i->linear_k_isnull = 1;
	    }
      }
}


void
process_calib_state (cJSON * json)
{
    cJSON *obj = NULL;

    cJSON_ArrayForEach (obj, json)
    {
	int x, found = 0;
	for (x = 0; x < MAXITEMS; x++)
	  {
	      if (!c.items[x].key[0])
		{
		    break;
		}
	      if (!strcmp (c.items[x].key, obj->string))
		{
		    found = 1;
		    copy_calib_values (obj, &(c.items[x]));

		}
	  }

	if (!found)
	  {
	      fprintf (stderr, "Adding calibration value key %s %d\n",
		       obj->string, obj->child != NULL);
	      strcpy (c.items[x].key, obj->string);
	      copy_calib_values (obj, &(c.items[x]));

	  }

    }
}

/*
void
process_cmd (cJSON * json)
{
    cJSON *s = cJSON_GetObjectItemCaseSensitive (json, "cmd");
    if (s != NULL)
      {
	  int set = 0, found = 0;

	  if (!strcmp (s->valuestring, "set"))
	    {
		set = 1;
	    }

	  s = cJSON_GetObjectItemCaseSensitive (json, "key");
	  cJSON *v = cJSON_GetObjectItemCaseSensitive (json, "value");

	  int x;
	  for (x = 0; x < MAXITEMS; x++)
	    {
		if (!c.items[x].key[0])
		  {
		      break;
		  }
		if (!strcmp (c.items[x].key, s->valuestring))
		  {
		      found = 1;

		      if (set)
			{
			    strcpy (c.items[x].value, v->valuestring);
			}
		      else
			{
			    memset (c.items[x].value, 0, VALUELEN);
			}
		  }
	    }

	  if (!found && set)
	    {
		strcpy (c.items[x].key, s->valuestring);
		strcpy (c.items[x].value, v->valuestring);
	    }

	  c.time = getMillisecondsSinceEpoch ();
	  save_config ();
      }
}
*/
