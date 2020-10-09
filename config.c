#include <stdio.h>
#include <string.h>
#include "cJSON.h"
#include "config.h"
#include "epoch.h"

#define SAVEFILE "config.state"
struct config c;

void
init_config ()
{
    memset (&c, 0, sizeof (struct config));
    FILE *in = fopen (SAVEFILE, "r");

    if (in)
      {
	  fread (&c, sizeof (struct config), 1, in);
	  fclose (in);
      }
}

void
save_config ()
{
    FILE *out = fopen (SAVEFILE, "w");
    if (out)
      {
	  fwrite (&c, sizeof (struct config), 1, out);
	  fclose (out);
      }
}

cJSON *
get_config_state ()
{
    cJSON *root = cJSON_CreateObject ();
    int x;

    for (x = 0; x < MAXITEMS; x++)
      {
	  if (!c.items[x].key[0])
	    {
		break;
	    }

	  cJSON_AddItemToObject (root, c.items[x].key,
				 cJSON_CreateString (c.items[x].value));
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
