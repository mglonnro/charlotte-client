#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <mqueue.h>
#include <fcntl.h>
#include <signal.h>

#include <wsclient/wsclient.h>
#include "wsclient.h"

#define QUEUE_NAME  "/test_queue" /* Queue name. */
#define QUEUE_PERMS ((int)(0644))
#define QUEUE_MAXMSG  16 /* Maximum number of messages. */
#define QUEUE_MSGSIZE 1024 /* Length of message. */

#define QUEUE_ATTR_INITIALIZER ((struct mq_attr){0, QUEUE_MAXMSG, QUEUE_MSGSIZE, 0, {0}})

//#define WS_SERVER "ws://192.168.8.101:3100"
#define WS_SERVER "wss://community.nakedsailor.blog/timescaledb"

wsclient *ws_client;
int 	is_open = 0;

int onclose(wsclient *c) {
	fprintf(stderr, "Cloud connection closed (sockfd: %d)\n", c->sockfd);
	is_open = 0;
	return 0;
}

int onerror(wsclient *c, wsclient_error *err) {
	fprintf(stderr, "ERROR: Cloud connection: (%d): %s\n", err->code, err->str);
	if(err->extra_code) {
		errno = err->extra_code;
		perror("recv");
	}
	return 0;
}

int onmessage(wsclient *c, wsclient_message *msg) {
	fprintf(stderr, "onmessage: (%llu): %s\n", msg->payload_len, msg->payload);
	return 0;
}

int onopen(wsclient *c) {
	fprintf(stderr, "Cloud connection OK (sockfd: %d)\n", c->sockfd);
	is_open = 1;
	return 0;
}

mqd_t consumer;

int init_ws_client(char *boat_id) {
	char url[256];
 	memset(url, 0, 256);
	sprintf(url, "%s/boat/%s/data", WS_SERVER, boat_id);

        fprintf(stderr, "Opening cloud connection to %s\n", url);
  
	ws_client = libwsclient_new(url);
	wsclient *client = ws_client;

	if (!client) {
		fprintf(stderr, "ERROR: Unable to initialize new websocket client.\n");
	
		// Some retry logic here
		return 0;
	}

	//set callback functions for this client
	libwsclient_onopen(client, &onopen);
	libwsclient_onmessage(client, &onmessage);
	libwsclient_onerror(client, &onerror);
	libwsclient_onclose(client, &onclose);
        // libwsclient_helper_socket(client, "charlotte.sock");

  	// Open MQ
	/* struct mq_attr attr = QUEUE_ATTR_INITIALIZER;
	consumer = mq_open("/data.queue", O_RDONLY | O_NONBLOCK, &attr);
	if(consumer < 0) {
		fprintf(stderr, "[CONSUMER]: Error, cannot open the queue: %s.\n", strerror(errno));
		exit(1);
	} */

	libwsclient_run(client);

	return 1;
}

/*
int poll() {
	if (consumer) {
		unsigned int prio;
		ssize_t bytes_read;
		char buffer[QUEUE_MSGSIZE + 1];
		struct timespec poll_sleep;
	
		memset(buffer, 0x00, sizeof(buffer));
		bytes_read = mq_receive(consumer, buffer, QUEUE_MSGSIZE, &prio);
		if(bytes_read >= 0) {
			printf("[CONSUMER]: Received message: \"%s\"\n", buffer);
		} 

		fflush(stdout);
	}  
}
*/

int ws_send(char *message, int size) {
	if (is_open) {
		char buf[size];
		memset(buf, 0, size);
		memcpy(buf, message, size);

		libwsclient_send(ws_client, buf);
		return 1;
	} else {
		return 0;
	}
}

