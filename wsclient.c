#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <wsclient/wsclient.h>
#include "wsclient.h"

int onclose(wsclient *c) {
	fprintf(stderr, "onclose called: %d\n", c->sockfd);
	return 0;
}

int onerror(wsclient *c, wsclient_error *err) {
	fprintf(stderr, "onerror: (%d): %s\n", err->code, err->str);
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
	fprintf(stderr, "onopen called: %d\n", c->sockfd);
	libwsclient_send(c, "Hello onopen");
	return 0;
}

#define WS_SERVER "ws://localhost:3100"

wsclient *ws_client;

int init_ws_client(char *boat_id) {

	char url[256];
	sprintf(url, "%s/boat/%s/data", WS_SERVER, boat_id);

        printf("init ws %s\n", url);
  
	ws_client = libwsclient_new(url);
	wsclient *client = ws_client;

	if (!client) {
		fprintf(stderr, "Unable to initialize new WS client.\n");
		// Some retry logic here
		exit(1);
	}
	//set callback functions for this client
	libwsclient_onopen(client, &onopen);
	libwsclient_onmessage(client, &onmessage);
	libwsclient_onerror(client, &onerror);
	libwsclient_onclose(client, &onclose);

	//bind helper UNIX socket to "test.sock"
	//One can then use netcat (nc) to send data to the websocket server end on behalf of the client, like so:
	// $> echo -n "some data that will be echoed by echo.websocket.org" | nc -U test.sock
	//libwsclient_helper_socket(client, "test2.sock");
	//starts run thread.
	libwsclient_run(client);
	//blocks until run thread for client is done.
	//while (1) { 
	//  sleep(1);
	//  libwsclient_send(client, "Yoyoyo test");
        //}
	return 0;
}

int ws_send(char *message) {
	libwsclient_send(ws_client, message);
}

int finish_ws_client() {
	libwsclient_finish(ws_client);
	return 0;
}

