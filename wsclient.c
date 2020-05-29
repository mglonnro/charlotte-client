#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <wsclient/wsclient.h>
#include "wsclient.h"

//#define WS_SERVER "ws://192.168.8.101:3100"
#define WS_SERVER "wss://community.nakedsailor.blog/timescaledb"

wsclient *ws_client;

int onclose(wsclient *c) {
	fprintf(stderr, "Cloud connection closed (sockfd: %d)\n", c->sockfd);
	libwsclient_finish(ws_client);
	ws_client = NULL;
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
	return 0;
}

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
	libwsclient_run(client);

	return 1;
}

int ws_send(char *message) {
	if (ws_client) {
		libwsclient_send(ws_client, message);
		return 1;
	} else {
		return 0;
	}
}

int finish_ws_client() {
	//libwsclient_close(ws_client);
	//libwsclient_finish(ws_client);
	return 0;
}

