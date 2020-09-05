#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>

#include "nmea_parser.h"
#include "ws.h"

#define BUFSIZE 4096
#define FULL_SYNC_INTERVAL 10
#define RETRY_INTERVAL	2

char *buf_getline ();
int addbuf (char *buf);

int
main (int argc, char **argv)
{
    char str[BUFSIZE], message[BUFSIZE];
    char boat_id[256];
    long last_sync_state;
    long now;

    if (argc != 2)
      {
	  fprintf (stderr, "usage: %s <boatId>\n", argv[0]);
	  exit (1);
      }
    fprintf (stderr, "charlotte-client v%s\n", VERSION);

    ws_init (argv[1]);

    signal (SIGPIPE, SIG_IGN);

    memset (&str, 0, BUFSIZE);
    memset (&message, 0, BUFSIZE);

    strcpy (boat_id, argv[1]);

    init_nmea_parser ();

    time (&last_sync_state);

    /* Setting stdin to non - blocking. */
    fcntl (0, F_SETFL, fcntl (0, F_GETFL) | O_NONBLOCK);

    while (1)
      {
	  int size = read (0, str, BUFSIZE);

	  if (size > 0)
	    {
		str[size] = 0;
		addbuf (str);

#ifdef CHAR_DEBUG
		fprintf (stderr, "R");
		fflush (stderr);
#endif
		char *line;

		while ((line = buf_getline ()) != NULL)
		  {
#ifdef CHAR_DEBUG
		      fprintf (stderr, ".");
		      fflush (stderr);
#endif

		      strcpy (str, line);
		      /* fprintf (stderr, "Processing: %s\n", str); */
		      memset (message, 0, BUFSIZE);
		      int hasdiff = parse_nmea (str, message);

		      if (hasdiff)
			{
#ifdef CHAR_DEBUG
			    fprintf (stderr, "S");
			    fflush (stderr);
#endif
			    /* printf ("sending %s\n", message); */
			    ws_write (message, strlen (message));
#ifdef CHAR_DEBUG
			    fprintf (stderr, "1");
			    fflush (stderr);
#endif
			    ws_service ();
#ifdef CHAR_DEBUG
			    fprintf (stderr, "2");
			    fflush (stderr);
#endif
			}
		  }
		time (&now);
		if ((now - last_sync_state) >= FULL_SYNC_INTERVAL)
		  {
		      if (get_nmea_state (message))
			{
			    //printf("Sending full state: %s\n", message);
			    //ws_send(message);
			}
		      time (&last_sync_state);
		  }
	    }
	  //websockets loop
      }
}

/*
 * Get the next line of text, keeping a buffer of the incoming stdin input
 */

#define BUFFER_SIZE	16384
static char buffer[BUFFER_SIZE];

int
addbuf (char *buf)
{
    if (buf)
      {
	  int len = strlen (buf);
	  int cur_len = strlen (buffer);

	  if ((len + cur_len + 1) > BUFFER_SIZE)
	    {
		memcpy (buffer, buffer + (len + 1), cur_len - (len + 1));
		memcpy (buffer + (len + 1), buf, (len + 1));
		return (len + cur_len + 1) - BUFFER_SIZE;
	    }
	  else
	    {
		strcat (buffer, buf);
		return 0;
	    }
      }
    return 0;
}

char *
buf_getline ()
{
    static char ret[BUFFER_SIZE];

    memset (ret, 0, BUFFER_SIZE);
    int line_found = 0, pos = 0;

    for (int i = 0; i < BUFFER_SIZE && buffer[i]; i++)
      {
	  char c = buffer[i];

	  //fprintf(stderr, "Char %c %d %s\n", c, c, ret);

	  if (c == '\n' || c == '\r')
	    {
		line_found = 1;
		ret[pos++] = 0;
		break;
	    }
	  else
	    {
		ret[pos++] = c;
	    }
      }

    if (line_found)
      {

	  if (pos < (BUFFER_SIZE - 1)
	      && (buffer[pos] == '\r' || buffer[pos] == '\n'))
	    {
		pos++;
	    }
	  //Remove found stuff from beginning of buffer
	  memcpy (buffer, buffer + pos, BUFFER_SIZE - pos);
	  memset (buffer + (BUFFER_SIZE - pos), 0, pos);

	  return ret;
      }
    return NULL;
}
