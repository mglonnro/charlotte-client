/*
 * This example shows how to write a client that subscribes to a topic and does
 * not do anything other than handle the messages that are received.
 */

#include <mosquitto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cJSON.h"
#include "epoch.h"
#include "mqtt.h"

void (*mqtt_callback) (char *);

/* Callback called when the client receives a CONNACK message from the broker. */
void
on_connect (struct mosquitto *mosq, void *obj, int reason_code)
{
    int rc;
    /* Print out the connection result. mosquitto_connack_string() produces an
     * appropriate string for MQTT v3.x clients, the equivalent for MQTT v5.0
     * clients is mosquitto_reason_string().
     */
    printf ("on_connect: %s\n", mosquitto_connack_string (reason_code));
    if (reason_code != 0)
      {
	  /* If the connection fails for any reason, we don't want to keep on
	   * retrying in this example, so disconnect. Without this, the client
	   * will attempt to reconnect. */
	  mosquitto_disconnect (mosq);
      }

    /* Making subscriptions in the on_connect() callback means that if the
     * connection drops and is automatically resumed by the client, then the
     * subscriptions will be recreated when the client reconnects. */

    char *topics[] = {
	"zigbee2mqtt/aft-cabin-port",
	"zigbee2mqtt/aft-cabin-stb",
	"zigbee2mqtt/master-cabin",
	"zigbee2mqtt/companionway",
	"zigbee2mqtt/saloon-stb",
	"zigbee2mqtt/saloon-port",
	"zigbee2mqtt/output",
	NULL
    };

    for (int x = 0; topics[x] != NULL; x++)
      {

	  rc = mosquitto_subscribe (mosq, NULL, topics[x], 1);
	  if (rc != MOSQ_ERR_SUCCESS)
	    {
		fprintf (stderr, "Error subscribing: %s\n",
			 mosquitto_strerror (rc));
		/* We might as well disconnect if we were unable to subscribe */
		mosquitto_disconnect (mosq);
	    }
      }
}


/* Callback called when the broker sends a SUBACK in response to a SUBSCRIBE. */
void
on_subscribe (struct mosquitto *mosq, void *obj, int mid, int qos_count,
	      const int *granted_qos)
{
    int i;
    bool have_subscription = false;

    /* In this example we only subscribe to a single topic at once, but a
     * SUBSCRIBE can contain many topics at once, so this is one way to check
     * them all. */
    for (i = 0; i < qos_count; i++)
      {
	  printf ("on_subscribe: %d:granted qos = %d\n", i, granted_qos[i]);
	  if (granted_qos[i] <= 2)
	    {
		have_subscription = true;
	    }
      }
    if (have_subscription == false)
      {
	  /* The broker rejected all of our subscriptions, we know we only sent
	   * the one SUBSCRIBE, so there is no point remaining connected. */
	  fprintf (stderr, "Error: All subscriptions rejected.\n");
	  mosquitto_disconnect (mosq);
      }
}


/* Callback called when the client receives a message. */
void
on_message (struct mosquitto *mosq, void *obj,
	    const struct mosquitto_message *msg)
{
    /* This blindly prints the payload, but the payload can be anything so take care. */
    printf ("%s %d %s\n", msg->topic, msg->qos, (char *) msg->payload);

    /* We'll construct a JSON object here. */
    cJSON *root = cJSON_CreateObject ();

    /* Add timestamp */
    char tbuf[31];
    memset (tbuf, 0, 31);
    getUTCTimestamp (tbuf);
    cJSON_AddStringToObject (root, "time", tbuf);
    cJSON *mqtt_payload = cJSON_Parse ((char *) msg->payload);
    // cJSON *mqtt_topic = cJSON_CreateObject ();
    // cJSON_AddItemToObject (mqtt_topic, msg->topic, mqtt_payload);
    cJSON_AddItemToObject (root, msg->topic, mqtt_payload);

    char message[4096];

    char *p = cJSON_Print (root);
    if (p)
      {
	  sprintf (message, "%s", p);
	  trim_message (message);
	  if (mqtt_callback)
	    {
		mqtt_callback (message);
	    }

	  fprintf (stderr, "message: %s\n", message);
	  free (p);
      }

    cJSON_Delete (root);
}

static struct mosquitto *mosq;

int
init_mqtt (void *cb)
{
    mqtt_callback = cb;

    int rc;

    /* Required before calling other mosquitto functions */
    mosquitto_lib_init ();

    /* Create a new client instance.
     * id = NULL -> ask the broker to generate a client id for us
     * clean session = true -> the broker should remove old sessions when we connect
     * obj = NULL -> we aren't passing any of our private data for callbacks
     */
    mosq = mosquitto_new (NULL, true, NULL);
    if (mosq == NULL)
      {
	  fprintf (stderr, "Error: Out of memory.\n");
	  return 1;
      }

    /* Configure callbacks. This should be done before connecting ideally. */
    mosquitto_connect_callback_set (mosq, on_connect);
    mosquitto_subscribe_callback_set (mosq, on_subscribe);
    mosquitto_message_callback_set (mosq, on_message);

    /* Connect to test.mosquitto.org on port 1883, with a keepalive of 60 seconds.
     * This call makes the socket connection only, it does not complete the MQTT
     * CONNECT/CONNACK flow, you should use mosquitto_loop_start() or
     * mosquitto_loop_forever() for processing net traffic. */
    rc = mosquitto_connect (mosq, "localhost", 1883, 60);
    if (rc != MOSQ_ERR_SUCCESS)
      {
	  mosquitto_destroy (mosq);
	  fprintf (stderr, "Error: %s\n", mosquitto_strerror (rc));
	  return 1;
      }

    return 0;
}

void
mqtt_loop ()
{
    mosquitto_loop (mosq, 0, 1);
}

void
mqtt_cleanup ()
{
    mosquitto_lib_cleanup ();
}
