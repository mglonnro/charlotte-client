/*
 * lws-minimal-ws-client
 * 
 * Written in 2010-2020 by Andy Green <andy@warmcat.com>
 * 
 * This file is made available under the Creative Commons CC0 1.0 Universal
 * Public Domain Dedication.
 * 
 * This demonstrates a ws client that connects by default to libwebsockets.org
 * dumb increment ws server.
 */

#include <libwebsockets.h>
#include <string.h>
#include <signal.h>
#include <uv.h>
#include "cJSON.h"
#include "ws.h"
#include "config.h"
#include "nmea_parser.h"
#include "epoch.h"

/*
 * This represents your object that "contains" the client connection and has
 * the client connection bound to it
 */

struct msg
{
    void *payload;		/* is malloc'd */
    size_t len;
    char binary;
    char first;
    char final;
};

static struct my_conn
{
    lws_sorted_usec_list_t sul;	/* schedule connection retry */
    struct lws *wsi;		/* related wsi if any */
    uint16_t retry_count;	/* count of consequetive retries */

    struct lws_ring *ring;
    uint32_t tail;
    char flow_controlled;
    uint8_t completed:1;
    uint8_t write_consume_pending:1;

    // struct per_session_data__minimal *pss_list;

    struct msg amsg;		/* the one pending message... */
    int current;		/* the current message number we are caching */
} mco;

/* one of these is created for each client connecting to us */

struct per_local_session_data__minimal
{
    struct per_local_session_data__minimal *pss_list;
    struct lws *wsi;
    struct lws_ring *client_ring;
    int last;			/* the last message number we sent */
    uint32_t tail;
    char flow_controlled;
    uint8_t completed:1;
    uint8_t write_consume_pending:1;
};

int
get_connected_count (struct per_local_session_data__minimal *pss_list)
{
    if (!pss_list)
      {
	  return 0;
      }

    int count = 1;
    struct per_local_session_data__minimal *next = pss_list;

    while (next->pss_list != NULL)
      {
	  count++;
	  next = next->pss_list;
      }

    return count;
}

/* one of these is created for each vhost our protocol is used with */

struct per_vhost_data__minimal
{
    struct lws_context *context;
    struct lws_vhost *vhost;
    const struct lws_protocols *protocol;

    struct per_local_session_data__minimal *pss_list;	/* linked-list of live pss */

    struct lws_ring *s_ring;
    uint32_t tail;
    char flow_controlled;
    uint8_t completed:1;
    uint8_t write_consume_pending:1;

    struct msg amsg;		/* the one pending message... */
    int current;		/* the current message number we are caching */
};

static struct per_vhost_data__minimal *vhost;

void
clear_lws_ring (struct per_vhost_data__minimal *vhd, struct lws_ring *ring)
{

    size_t count = lws_ring_get_count_waiting_elements (ring, &vhd->tail);
    lws_ring_consume (ring, &vhd->tail, NULL, count);
#ifdef CHAR_DEBUG2
    fprintf (stderr, "C");
    fflush (stderr);
#endif
    return;
}

#define BOAT_ID_SIZE	50
static struct lws_context *context;
static int interrupted, port = 443, ssl_connection = LCCSCF_USE_SSL;
static const char *server_address = "community.nakedsailor.blog", *pro =
    "charlotte-data";
static char boat_id[BOAT_ID_SIZE];

#define CLIENT_MSG_BUFFER	32768
static char receive_message[CLIENT_MSG_BUFFER];

/*
 * The retry and backoff policy we want to use for our client connections
 */

static const uint32_t backoff_ms[] = { 1000, 2000, 3000, 4000, 5000 };

static const lws_retry_bo_t retry = {
    .retry_ms_table = backoff_ms,
    .retry_ms_table_count = LWS_ARRAY_SIZE (backoff_ms),
    .conceal_count = LWS_ARRAY_SIZE (backoff_ms) + 1,

    .secs_since_valid_ping = 3,	/* force PINGs after secs idle */
    .secs_since_valid_hangup = 10,	/* hangup after secs idle */

    .jitter_percent = 20,
};

/*
 * Scheduled sul callback that starts the connection attempt
 */

static void
connect_client (lws_sorted_usec_list_t * sul)
{
    struct my_conn *mco = lws_container_of (sul, struct my_conn, sul);
    struct lws_client_connect_info i;

    memset (&i, 0, sizeof (i));

    i.context = context;
    i.port = port;
    i.address = server_address;

    static char path[256];
    sprintf (path, "/api/boat/%s/data", boat_id);

    i.path = path;
    i.host = i.address;
    i.origin = i.address;
    i.ssl_connection = ssl_connection;
    i.protocol = pro;
    i.local_protocol_name = "charlotte-client";
    i.pwsi = &mco->wsi;
    i.retry_and_idle_policy = &retry;
    i.userdata = mco;

    if (!lws_client_connect_via_info (&i))
      {
	  /*
	   * Failed... schedule a retry... we can't use the _retry_wsi()
	   * convenience wrapper api here because no valid wsi at this point.
	   */
	  mco->retry_count = 0;	/* HACK! */
	  if (lws_retry_sul_schedule (context, 0, sul, &retry,
				      connect_client, &mco->retry_count))
	    {
		lwsl_err ("%s: XXX connection attempts exhausted\n",
			  __func__);
	    }
      }
}

#define RING_DEPTH 512

static void
__minimal_destroy_message (void *_msg)
{
    struct msg *msg = _msg;

    free (msg->payload);
    msg->payload = NULL;
    msg->len = 0;
}

#define MSGBUFSIZE 16384

static int
callback_minimal (struct lws *wsi, enum lws_callback_reasons reason,
		  void *user, void *in, size_t len)
{
    struct my_conn *mco = (struct my_conn *) user;

    struct per_local_session_data__minimal *pss =
	(struct per_local_session_data__minimal *) user;
    struct per_vhost_data__minimal *vhd =
	(struct per_vhost_data__minimal *)
	lws_protocol_vh_priv_get (lws_get_vhost (wsi),
				  lws_get_protocol (wsi));


#ifdef CHAR_DEBUG
    fprintf (stderr, "#");
    fflush (stderr);
#endif

    const struct msg *pmsg;
    char *reply;
    char message[MSGBUFSIZE];

    int m, flags;

    switch (reason)
      {
/* One-time call per protocol, per-vhost using it, so it can do initial setup / allocations etc */
      case LWS_CALLBACK_PROTOCOL_INIT:
	  vhd = lws_protocol_vh_priv_zalloc (lws_get_vhost (wsi),
					     lws_get_protocol (wsi),
					     sizeof (struct
						     per_vhost_data__minimal));
	  vhd->context = lws_get_context (wsi);
	  vhd->protocol = lws_get_protocol (wsi);
	  vhd->vhost = lws_get_vhost (wsi);
	  vhd->s_ring = lws_ring_create (sizeof (struct msg), RING_DEPTH,
					 __minimal_destroy_message);
	  if (!vhd->s_ring)
	      return 1;
	  vhd->tail = 0;
	  vhost = vhd;
	  break;


/* (VH) after the server completes a handshake with an incoming client. If you built the library with ssl support, in is a pointer to the ssl struct associated with the connection or NULL. */

      case LWS_CALLBACK_ESTABLISHED:
	  /* add ourselves to the list of live pss held in the vhd */
	  lws_ll_fwd_insert (pss, pss_list, vhd->pss_list);
	  pss->wsi = wsi;
	  pss->last = vhd->current;
	  pss->client_ring =
	      lws_ring_create (sizeof (struct msg), RING_DEPTH,
			       __minimal_destroy_message);
	  if (!pss->client_ring)
	    {
		return 1;
	    }
	  pss->tail = 0;

	  fprintf (stderr, "Connected count: %d\n",
		   get_connected_count (vhd->pss_list));

	  /* Send config & state */
	  /* Will this send it to all clients? */
	  memset (message, 0, MSGBUFSIZE);
	  get_nmea_state (message);

	  if (message[0])
	    {
		fprintf (stderr, "Sending state: %s\n", message);
		ws_write_client (message, strlen (message));
	    }
	  else
	    {
		fprintf (stderr, "No state to send!\n");
	    }

	  cJSON *config = get_config_state ();
	  char *p = cJSON_Print (config);
	  if (p)
	    {
		trim_message (p);

		fprintf (stderr, "Sending config: %s\n", p);

		ws_write_client (p, strlen (p));
		free (p);
	    }
	  cJSON_Delete (config);

	  break;

      case LWS_CALLBACK_CLOSED:
	  /* remove our closing pss from the list of live pss */
	  lws_ll_fwd_remove (struct per_local_session_data__minimal, pss_list,
			     pss, vhd->pss_list);
	  fprintf (stderr, "Connected count: %d\n",
		   get_connected_count (vhd->pss_list));
	  break;

      case LWS_CALLBACK_SERVER_WRITEABLE:

	  /*
	     fprintf (stderr, "IN SERVER_WRITEABLE %d\n",
	     lws_ring_get_count_waiting_elements (pss->client_ring,
	     &pss->tail));
	   */

#ifdef CHAR_DEBUG2
	  fprintf (stderr, "w1");
	  fflush (stderr);
#endif

	  pmsg = lws_ring_get_element (pss->client_ring, &pss->tail);
	  if (!pmsg)
	    {
		lwsl_user (" (nothing in s_ring)\n");
		break;
	    }
	  else
	    {
		/*
		   char *ptr = pmsg->payload + LWS_PRE;
		   if (strstr (ptr, "aws"))
		   {
		   char tbuf[31];
		   memset (tbuf, 0, 31);
		   getUTCTimestamp (tbuf);
		   fprintf (stderr, "%s SENDING: %s\n", tbuf,
		   (char *) (pmsg->payload + LWS_PRE));
		   }
		 */
	    }

	  /* notice we allowed for LWS_PRE in the payload already */
	  m = lws_write (wsi, ((unsigned char *) pmsg->payload) +
			 LWS_PRE, pmsg->len, LWS_WRITE_TEXT);


	  if (m < (int) pmsg->len)
	    {
		lwsl_err ("ERROR %d writing to ws socket\n", m);
		lws_ring_consume_single_tail (pss->client_ring, &pss->tail,
					      1);
		fprintf (stderr, "OUT.b SERVER_WRITEABLE %ld\n",
			 lws_ring_get_count_waiting_elements
			 (pss->client_ring, &pss->tail));
		return -1;
	    }

	  lws_ring_consume_single_tail (pss->client_ring, &pss->tail, 1);
	  pss->completed = 1;

	  if (lws_ring_get_count_waiting_elements
	      (pss->client_ring, &pss->tail) > 0)
	    {

		lws_callback_on_writable (wsi);
	    }

	  break;
      case LWS_CALLBACK_RECEIVE:
	  /* Inbound message from client */
	  reply = process_inbound_message (in, len);

	  /* Forward it to cloud */
	  ws_write_cloud (in, len);

	  /* Write processed reply if we have any */
	  if (reply)
	    {
		ws_write_client (reply, strlen (reply));
		free (reply);
	    }
	  break;

	  if (vhd->amsg.payload)
	      __minimal_destroy_message (&vhd->amsg);

	  vhd->amsg.len = len;
	  /* notice we over-allocate by LWS_PRE */
	  vhd->amsg.payload = malloc (LWS_PRE + len);
	  if (!vhd->amsg.payload)
	    {
		lwsl_user ("OOM: dropping\n");
		break;
	    }

	  memcpy ((char *) vhd->amsg.payload + LWS_PRE, in, len);
	  vhd->current++;

	  /*
	   * let everybody know we want to write something on them
	   * as soon as they are ready
	   */
	  lws_start_foreach_llp (struct per_local_session_data__minimal **,
				 ppss, vhd->pss_list)
	  {
	      lws_callback_on_writable ((*ppss)->wsi);
	  } lws_end_foreach_llp (ppss, pss_list);
	  break;

      case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
	  lwsl_err ("CLIENT_CONNECTION_ERROR: %s\n",
		    in ? (char *) in : "(null)");
	  if (mco->ring)
	    {
		lws_ring_destroy (mco->ring);
		mco->ring = NULL;
	    }
	  goto do_retry;
	  break;

      case LWS_CALLBACK_CLIENT_RECEIVE:
	  if (strlen (receive_message) + len + 1 > CLIENT_MSG_BUFFER)
	    {
		fprintf (stderr, "buffer overrun, dropping %ld bytes\n", len);
	    }
	  else
	    {
		memcpy (receive_message + strlen (receive_message), in, len);
		receive_message[strlen (receive_message) + len] = 0;
	    }

	  if (!lws_remaining_packet_payload (wsi))
	    {
		char *reply = process_inbound_message (receive_message,
						       strlen
						       (receive_message));
		if (reply)
		  {
		      free (reply);
		  }
		memset (receive_message, 0, CLIENT_MSG_BUFFER);
		fprintf (stderr, "processed:\n");
		lwsl_hexdump_notice (in, len);
	    }
	  else
	    {

		fprintf (stderr, "more to come:\n");
		lwsl_hexdump_notice (in, len);
	    }

	  break;

      case LWS_CALLBACK_CLIENT_ESTABLISHED:
	  lwsl_user ("%s: established\n", __func__);
	  mco->ring = lws_ring_create (sizeof (struct msg), RING_DEPTH,
				       __minimal_destroy_message);
	  if (!mco->ring)
	      return 1;
	  mco->tail = 0;
	  break;

      case LWS_CALLBACK_CLIENT_WRITEABLE:
#ifdef CHAR_DEBUG
	  fprintf (stderr, "W");
	  fflush (stderr);
#endif

	  lwsl_user ("LWS_CALLBACK_CLIENT_WRITEABLE\n");

	  if (mco->write_consume_pending)
	    {
		/* perform the deferred fifo consume */
		lws_ring_consume_single_tail (mco->ring, &mco->tail, 1);
		mco->write_consume_pending = 0;
	    }
	  pmsg = lws_ring_get_element (mco->ring, &mco->tail);
	  if (!pmsg)
	    {
		lwsl_user (" (nothing in ring)\n");
#ifdef CHAR_DEBUG
		fprintf (stderr, "0");
		fflush (stderr);
#endif
		break;
	    }
	  flags =
	      lws_write_ws_flags (pmsg->binary ? LWS_WRITE_BINARY :
				  LWS_WRITE_TEXT, pmsg->first, pmsg->final);

	  /* notice we allowed for LWS_PRE in the payload already */
	  m = lws_write (wsi, ((unsigned char *) pmsg->payload) +
			 LWS_PRE, pmsg->len, flags);
	  if (m < (int) pmsg->len)
	    {
		lwsl_err ("ERROR %d writing to ws socket\n", m);
#ifdef CHAR_DEBUG
		fprintf (stderr, "e");
		fflush (stderr);
#endif
		return -1;
	    }
	  mco->completed = 1;

	  /*
	   * Workaround deferred deflate in pmd extension by only consuming the
	   * fifo entry when we are certain it has been fully deflated at the
	   * next WRITABLE callback.  You only need this if you're using pmd.
	   */
	  mco->write_consume_pending = 1;
	  lws_callback_on_writable (wsi);

	  if (mco->flow_controlled &&
	      (int) lws_ring_get_count_free_elements (mco->ring) >
	      RING_DEPTH - 5)
	    {
		lws_rx_flow_control (wsi, 1);
		mco->flow_controlled = 0;
	    }
#ifdef CHAR_DEBUG
	  fprintf (stderr, "w");
	  fflush (stderr);
#endif
	  break;

      case LWS_CALLBACK_CLIENT_CLOSED:
	  if (mco->ring)
	    {
		lws_ring_destroy (mco->ring);
		mco->ring = NULL;
	    }
	  goto do_retry;

      default:
	  break;
      }

    return lws_callback_http_dummy (wsi, reason, user, in, len);

  do_retry:
    /*
     * retry the connection to keep it nailed up
     * 
     * For this example, we try to conceal any problem for one set of backoff
     * retries and then exit the app.
     * 
     * If you set retry.conceal_count to be larger than the number of elements
     * in the backoff table, it will never give up and keep retrying at the
     * last backoff delay plus the random jitter amount.
     */
    if (lws_retry_sul_schedule_retry_wsi (wsi, &mco->sul, connect_client,
					  &mco->retry_count))
      {
	  lwsl_err ("%s: YYY connection attempts exhausted\n", __func__);
	  interrupted = 1;
	  mco->retry_count = 0;
	  goto do_retry;
      }
    return 0;
}

static const struct lws_protocols protocols[] = {
    {"lws-minimal-client", callback_minimal,
     sizeof (struct per_local_session_data__minimal), 128, 0, NULL, 0},
    {NULL, NULL, 0, 0}
};

int
ws_init (char *id, uv_loop_t * loop)
{
    struct lws_context_creation_info info;

    strcpy (boat_id, id);

    memset (&info, 0, sizeof (struct lws_context_creation_info));

    lwsl_user ("LWS minimal ws client\n");

    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.options |= LWS_SERVER_OPTION_LIBUV;
    info.options |=
	LWS_SERVER_OPTION_HTTP_HEADERS_SECURITY_BEST_PRACTICES_ENFORCE;
    info.port = 7681;
    info.protocols = protocols;
    info.foreign_loops = (void *[])
    {
	loop
    };

#if defined(LWS_WITH_MBEDTLS) || defined(USE_WOLFSSL)
    /*
     * OpenSSL uses the system trust store.  mbedTLS has to be told which CA
     * to trust explicitly.
     */
    info.client_ssl_ca_filepath = "./libwebsockets.org.cer";
#endif

    ssl_connection |= LCCSCF_ALLOW_SELFSIGNED;
    ssl_connection |= LCCSCF_ALLOW_INSECURE;
    ssl_connection |= LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
    ssl_connection |= LCCSCF_ALLOW_EXPIRED;

    info.fd_limit_per_thread = 10;
    //1 + 1 + 1;

    context = lws_create_context (&info);
    if (!context)
      {
	  lwsl_err ("lws init failed\n");
	  return 1;
      }
    /* schedule the first client connection attempt to happen immediately */
    lws_sul_schedule (context, 0, &mco.sul, connect_client, 1);
    return 0;
}

static int debug_flag = 0;

int
ws_write (char *buf, int len)
{
    int ret = ws_write_cloud (buf, len);
    ret += ws_write_client (buf, len);
    return ret;
}

int
ws_write_cloud (char *buf, int len)
{
    if (debug_flag)
      {
#ifdef CHAR_DEBUG
	  fprintf (stderr, "!1");
	  fflush (stderr);
#endif
      }
    struct my_conn *pss = (struct my_conn *) &mco;
    struct msg amsg;

    if (!pss->ring)
      {
#ifdef CHAR_DEBUG
	  fprintf (stderr, "!2");
	  fflush (stderr);
#endif
	  /* ws_service (); */
	  return 0;
      }
    int n = (int) lws_ring_get_count_free_elements (pss->ring);
    if (!n)
      {
#ifdef CHAR_DEBUG
	  fprintf (stderr, "!3");
	  fflush (stderr);
#endif
	  lwsl_user ("dropping!\n");
	  return 0;
      }
    amsg.first = 1;
    amsg.final = 1;
    amsg.binary = 0;
    amsg.len = len;
    /* notice we over-allocate by LWS_PRE */
    amsg.payload = malloc (LWS_PRE + len);
    if (!amsg.payload)
      {
	  lwsl_user ("OOM: dropping\n");
	  return 0;
      }
    memset (amsg.payload, 0, LWS_PRE + len);

    memcpy ((char *) amsg.payload + LWS_PRE, buf, len);
#ifdef CHAR_DEBUG
    fprintf (stderr, "s");
    fflush (stderr);
#endif

    if (!lws_ring_insert (pss->ring, &amsg, 1))
      {
#ifdef CHAR_DEBUG
	  fprintf (stderr, "X");
	  fflush (stderr);
#endif
	  lwsl_user ("dropping!\n");
	  __minimal_destroy_message (&amsg);
	  return 0;
      }
    /*
     * If this is 0x0 we 'll get a seg fault. Happens when the connection is
     * / has closed.
     */

    /* fprintf (stderr, "Asking for write callback: %s\n", buf);  */

    if (mco.ring)
      {
	  lws_callback_on_writable (mco.wsi);
      }

    return 1;
}

int
ws_write_client (char *buf, int len)
{
    struct per_vhost_data__minimal *vhd =
	(struct per_vhost_data__minimal *) vhost;

    if (!vhd || !vhd->s_ring)
      {
	  return 0;
      }

    if (get_connected_count (vhd->pss_list) == 0)
      {
	  clear_lws_ring (vhd, vhd->s_ring);
	  return 0;
      }

    /*
     * Insert a separate message into all clients' rings, and
     * notify them that there is something to be sent.
     */

    lws_start_foreach_llp (struct per_local_session_data__minimal **,
			   ppss, vhd->pss_list)
    {
	struct msg amsg;
	amsg.first = 1;
	amsg.final = 1;
	amsg.binary = 0;
	amsg.len = len;
	amsg.payload = malloc (LWS_PRE + len);
	if (!amsg.payload)
	  {
	      fprintf (stderr, "ws_write_client: cannot alloc for payload\n");
	      lwsl_user ("OOM: dropping\n");
	      return 0;
	  }
	memset (amsg.payload, 0, LWS_PRE + len);
	memcpy ((char *) amsg.payload + LWS_PRE, buf, len);

	int n = (int) lws_ring_get_count_free_elements ((*ppss)->client_ring);
	if (!n)
	  {
	      lwsl_user ("No space on wring, dropping!\n");
	  }
	else
	  {

	      if (!lws_ring_insert ((*ppss)->client_ring, &amsg, 1))
		{
		    fprintf (stderr,
			     "ws_write_client: cannot insert into ring!\n");
		    lwsl_user ("dropping!\n");
		    __minimal_destroy_message (&amsg);
		    return 0;
		}
	  }
	lws_callback_on_writable ((*ppss)->wsi);
    }
    lws_end_foreach_llp (ppss, pss_list);

    return 1;
}

void
ws_service ()
{
    lws_service (context, 0);
}

void
ws_destroy ()
{
    lws_context_destroy (context);
    lwsl_user ("Completed\n");
}
