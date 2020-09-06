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
#include "ws.h"

/*
 * This represents your object that "contains" the client connection and has
 * the client connection bound to it
 */

static struct my_conn {
    lws_sorted_usec_list_t sul;	/* schedule connection retry */
    struct lws     *wsi;	/* related wsi if any */
    uint16_t	    retry_count;/* count of consequetive retries */
    struct lws_ring *ring;
    uint32_t	    tail;
    char	    flow_controlled;
    uint8_t	    completed:1;
    uint8_t	    write_consume_pending:1;
}		mco;

struct msg {
    void           *payload;	/* is malloc'd */
    size_t	    len;
    char	    binary;
    char	    first;
    char	    final;
};

#define BOAT_ID_SIZE	50

static struct lws_context *context;
static int	interrupted, port = 443, ssl_connection = LCCSCF_USE_SSL;
static const char *server_address = "community.nakedsailor.blog", *pro =
"charlotte-data";
static char	boat_id[BOAT_ID_SIZE];


/*
 * The retry and backoff policy we want to use for our client connections
 */

static const uint32_t backoff_ms[] = {1000, 2000, 3000, 4000, 5000};

static const lws_retry_bo_t retry = {
    .retry_ms_table = backoff_ms,
    .retry_ms_table_count = LWS_ARRAY_SIZE(backoff_ms),
    .conceal_count = LWS_ARRAY_SIZE(backoff_ms) + 1,

    .secs_since_valid_ping = 3,	/* force PINGs after secs idle */
    .secs_since_valid_hangup = 10,	/* hangup after secs idle */

    .jitter_percent = 20,
};

/*
 * Scheduled sul callback that starts the connection attempt
 */

static void
connect_client(lws_sorted_usec_list_t * sul)
{
    struct my_conn *mco = lws_container_of(sul, struct my_conn, sul);
    struct lws_client_connect_info i;


    memset(&i, 0, sizeof(i));

    i.context = context;
    i.port = port;
    i.address = server_address;

    static char	    path[256];
    sprintf(path, "/timescaledb/boat/%s/data", boat_id);

    i.path = path;
    i.host = i.address;
    i.origin = i.address;
    i.ssl_connection = ssl_connection;
    i.protocol = pro;
    i.local_protocol_name = "charlotte-client";
    i.pwsi = &mco->wsi;
    i.retry_and_idle_policy = &retry;
    i.userdata = mco;

    if (!lws_client_connect_via_info(&i)) {
	/*
	 * Failed... schedule a retry... we can't use the _retry_wsi()
	 * convenience wrapper api here because no valid wsi at this point.
	 */
	mco->retry_count = 0;	/* HACK! */
	if (lws_retry_sul_schedule(context, 0, sul, &retry,
				   connect_client, &mco->retry_count)) {
	    lwsl_err("%s: XXX connection attempts exhausted\n",
		     __func__);
	}
    }
}

#define RING_DEPTH 1024

static void
__minimal_destroy_message(void *_msg)
{
    struct msg     *msg = _msg;

    free(msg->payload);
    msg->payload = NULL;
    msg->len = 0;
}

static int
callback_minimal(struct lws *wsi, enum lws_callback_reasons reason,
		 void *user, void *in, size_t len)
{
    struct my_conn *mco = (struct my_conn *)user;
    const struct msg *pmsg;
    int		    m     , flags;

    switch (reason) {

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
	lwsl_err("CLIENT_CONNECTION_ERROR: %s\n",
		 in ? (char *)in : "(null)");
	if (mco->ring) {
	    lws_ring_destroy(mco->ring);
	    mco->ring = NULL;
	}
	goto do_retry;
	break;

    case LWS_CALLBACK_CLIENT_RECEIVE:
	/* lwsl_hexdump_notice(in, len); */
	break;

    case LWS_CALLBACK_CLIENT_ESTABLISHED:
	lwsl_user("%s: established\n", __func__);
	mco->ring = lws_ring_create(sizeof(struct msg), RING_DEPTH,
				    __minimal_destroy_message);
	if (!mco->ring)
	    return 1;
	mco->tail = 0;
	break;

    case LWS_CALLBACK_CLIENT_WRITEABLE:
#ifdef CHAR_DEBUG
	fprintf(stderr, "W");
	fflush(stderr);
#endif

	/* lwsl_user ("LWS_CALLBACK_CLIENT_WRITEABLE\n"); */

	if (mco->write_consume_pending) {
	    /* perform the deferred fifo consume */
	    lws_ring_consume_single_tail(mco->ring, &mco->tail, 1);
	    mco->write_consume_pending = 0;
	}
	pmsg = lws_ring_get_element(mco->ring, &mco->tail);
	if (!pmsg) {
	    lwsl_user(" (nothing in ring)\n");
#ifdef CHAR_DEBUG
	    fprintf(stderr, "-");
	    fflush(stderr);
#endif
	    break;
	}
	flags =
	    lws_write_ws_flags(pmsg->binary ? LWS_WRITE_BINARY :
			       LWS_WRITE_TEXT, pmsg->first, pmsg->final);

	/* notice we allowed for LWS_PRE in the payload already */
	m = lws_write(wsi, ((unsigned char *)pmsg->payload) +
		      LWS_PRE, pmsg->len, flags);
	if (m < (int)pmsg->len) {
	    lwsl_err("ERROR %d writing to ws socket\n", m);
#ifdef CHAR_DEBUG
	    fprintf(stderr, "e");
	    fflush(stderr);
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
	lws_callback_on_writable(wsi);

	if (mco->flow_controlled &&
	    (int)lws_ring_get_count_free_elements(mco->ring) >
	    RING_DEPTH - 5) {
	    lws_rx_flow_control(wsi, 1);
	    mco->flow_controlled = 0;
	}
#ifdef CHAR_DEBUG
	fprintf(stderr, "w");
	fflush(stderr);
#endif
	break;

    case LWS_CALLBACK_CLIENT_CLOSED:
	goto do_retry;

    default:
	break;
    }

    return lws_callback_http_dummy(wsi, reason, user, in, len);

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
    if (lws_retry_sul_schedule_retry_wsi(wsi, &mco->sul, connect_client,
					 &mco->retry_count)) {
	lwsl_err("%s: YYY connection attempts exhausted\n", __func__);
	interrupted = 1;
	mco->retry_count = 0;
	goto do_retry;
    }
    return 0;
}

static const struct lws_protocols protocols[] = {
    {"lws-minimal-client", callback_minimal, 0, 0,},
    {NULL, NULL, 0, 0}
};

int
ws_init(char *id, uv_loop_t * loop)
{
    struct lws_context_creation_info info;

    strcpy(boat_id, id);

    memset(&info, 0, sizeof info);

    lwsl_user("LWS minimal ws client\n");

    info.options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    info.port = CONTEXT_PORT_NO_LISTEN;	/* we do not run any server */
    info.protocols = protocols;
    info.foreign_loops = (void *[]){
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

    context = lws_create_context(&info);
    if (!context) {
	lwsl_err("lws init failed\n");
	return 1;
    }
    /* schedule the first client connection attempt to happen immediately */
    lws_sul_schedule(context, 0, &mco.sul, connect_client, 1);
    return 0;
}

static int	debug_flag = 0;

int
ws_write(char *buf, int len)
{
    if (debug_flag) {
	return 0;
    }
    struct my_conn *pss = (struct my_conn *)&mco;
    struct msg	    amsg;

    if (!pss->ring) {
	ws_service();
	return 0;
    }
    int		    n = (int)lws_ring_get_count_free_elements(pss->ring);
    if (!n) {
	lwsl_user("dropping!\n");
	return 0;
    }
    amsg.first = 1;
    amsg.final = 1;
    amsg.binary = 0;
    amsg.len = len;
    /* notice we over-allocate by LWS_PRE */
    amsg.payload = malloc(LWS_PRE + len);
    if (!amsg.payload) {
	lwsl_user("OOM: dropping\n");
	return 0;
    }
    memset(amsg.payload, 0, LWS_PRE + len);

    memcpy((char *)amsg.payload + LWS_PRE, buf, len);
#ifdef CHAR_DEBUG
    fprintf(stderr, "s");
    fflush(stderr);
#endif

    if (!lws_ring_insert(pss->ring, &amsg, 1)) {
	lwsl_user("dropping!\n");
	__minimal_destroy_message(&amsg);
	return 0;
    }
#ifdef CHAR_DEBUG
    fprintf(stderr, "c");
    fflush(stderr);
#endif
    /*
     * fprintf (stderr, "Checking flow control");
     * 
     * if (!pss->flow_controlled && n < 3) { pss->flow_controlled = 1;
     * lws_rx_flow_control (mco.wsi, 0); }
     */

    return 1;
}

void
ws_service()
{
    lws_service(context, 0);
}

void
ws_destroy()
{
    lws_context_destroy(context);
    lwsl_user("Completed\n");
}
