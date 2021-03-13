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

#include <uv.h>
#include "nmea_parser.h"
#include "ws.h"
#include "config.h"

#define BUFSIZE 4096
#define FULL_SYNC_INTERVAL 10
#define RETRY_INTERVAL	2

char *buf_getline ();
int addbuf (char *buf, int len);
int interrupted = 0;
void process_buffer ();

static uv_pipe_t stdin_pipe;

#define BUFFER_SIZE	16384
static char buffer[BUFFER_SIZE + 1];	/* include space for null */

void
sigint_handler ()
{
    interrupted = 1;
}

void
alloc_buffer (uv_handle_t * handle, size_t suggested_size, uv_buf_t * buf)
{
    char *mem = malloc (suggested_size);
    memset (mem, 0, suggested_size);

    *buf = uv_buf_init (mem, suggested_size);
}

void
read_stdin (uv_stream_t * stream, ssize_t nread, const uv_buf_t * buf)
{
    if (nread < 0)
      {
	  if (nread == UV_EOF)
	    {
		// end of file
		fprintf (stderr, "Closing file!\n");
		fflush (stderr);
		uv_close ((uv_handle_t *) & stdin_pipe, NULL);
	    }
      }
    else if (nread > 0)
      {
#ifdef CHAR_DEBUG2
	  fprintf (stderr, "r0");
	  fflush (stderr);
#endif
	  addbuf (buf->base, nread);

#ifdef CHAR_DEBUG
	  fprintf (stderr, "\rr1-- ");
	  fflush (stderr);
#endif

	  process_buffer ();

#ifdef CHAR_DEBUG
	  fprintf (stderr, " --r2");
	  fflush (stderr);
#endif
      }

    if (buf->base)
	free (buf->base);
}

void
process_buffer ()
{
    char str[BUFSIZE], message[BUFSIZE], message_nosrc[BUFSIZE];
    char *line;

#ifdef CHAR_DEBUG
    fprintf (stderr, "p0");
    fflush (stderr);
#endif

    while ((line = buf_getline ()) != NULL)
      {

#ifdef CHAR_DEBUG
	  fprintf (stderr, "p1");
	  fflush (stderr);
#endif
	  memset (str, 0, BUFSIZE);
	  strcpy (str, line);
	  memset (message, 0, BUFSIZE);
	  memset (message_nosrc, 0, BUFSIZE);

	  int hasdiff = parse_nmea (str, message, message_nosrc);

#ifdef CHAR_DEBUG
	  fprintf (stderr, "p2");
	  fflush (stderr);
#endif

	  if (hasdiff)
	    {
#ifdef CHAR_DEBUG
		fprintf (stderr, "1");
		fflush (stderr);
#endif
		ws_write_cloud (message, strlen (message));
		ws_write_client (message_nosrc, strlen (message_nosrc));
	    }
      }
}

int
main (int argc, char **argv)
{
    long last_sync_state;

    if (argc != 2)
      {
	  fprintf (stderr, "usage: %s <boatId>\n", argv[0]);
	  exit (1);
      }
    fprintf (stderr, "charlotte-client v%s\n", VERSION);

    uv_loop_t *loop = uv_default_loop ();
    ws_init (argv[1], loop);

    signal (SIGPIPE, SIG_IGN);
    signal (SIGINT, sigint_handler);

    /* init */
    memset (buffer, 0, BUFFER_SIZE + 1);

    init_nmea_parser ();
    init_config ();		/* boat configuration, sails etc! */
    read_nmea_sources ();	/* read nmea sources state */

    time (&last_sync_state);	/* Will maybe use later */

    uv_pipe_init (loop, &stdin_pipe, 0);
    uv_pipe_open (&stdin_pipe, 0);
    uv_read_start ((uv_stream_t *) & stdin_pipe, alloc_buffer, read_stdin);
    uv_run (loop, UV_RUN_DEFAULT);

    fprintf (stderr, "Out of loop!");

    /*
     * time (&now); if ((now - last_sync_state) >= FULL_SYNC_INTERVAL) { if
     * (get_nmea_state (message)) { //printf("Sending full state: %s\n",
     * message); //ws_send(message); } time (&last_sync_state); }
     */

    ws_destroy ();
    uv_loop_close (loop);
}

/*
 * Get the next line of text, keeping a buffer of the incoming stdin input
 */


int
addbuf (char *buf, int len)
{
    if (!buf)
      {
	  return 0;
      }
    int cur_len = strlen (buffer);

    if (len >= BUFFER_SIZE)
      {
	  fprintf (stderr, "\nDropping %d bytes\n", (len - BUFFER_SIZE));
	  memcpy (buffer, buf + (len - BUFFER_SIZE), BUFFER_SIZE);
	  buffer[BUFFER_SIZE] = 0;
      }
    else if ((len + cur_len + 1) > BUFFER_SIZE)
      {
	  memcpy (buffer, buffer + (len + 1), cur_len - (len + 1));
	  memcpy (buffer + (len + 1), buf, (len + 1));
	  return (len + cur_len + 1) - BUFFER_SIZE;
      }
    else
      {
	  memcpy (buffer + strlen (buffer), buf, len);
	  buffer[strlen (buffer) + len] = 0;
	  return 0;
      }

    return 0;			/* unreachable, but warning  still */
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


	  if (strlen (ret))
	    {
		return ret;
	    }
	  else
	    {
		return NULL;
	    }
      }
    return NULL;
}
