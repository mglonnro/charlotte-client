#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <time.h>
#include "wsclient.h"
#include "nmea_parser.h"

#define BUFSIZE 4096
#define FULL_SYNC_INTERVAL 10
#define RETRY_INTERVAL	5

int
main(int argc, char **argv)
{
  char str[BUFSIZE], message[BUFSIZE];
  char boat_id[256];
  long last_init = 0, last_sync_state;
  long now;
  
  if (argc != 2) {
    fprintf(stderr, "usage: %s <boatId>\n", argv[0]);
    exit(1);
  }

  memset(&str, 0, BUFSIZE);
  memset(&message, 0, BUFSIZE);

  strcpy(boat_id, argv[1]);

  init_ws_client(boat_id);
  init_nmea_parser();

  time(&last_sync_state);

  int counter = 0;

  while (fgets(str, BUFSIZE, stdin) != NULL) {
    counter ++;

    /*if (counter == 100) {
      exit(0);
    }*/

    int hasdiff = parse_nmea(str, message);
    if (hasdiff) {
      //printf("sending %s\n", message);

      if (!ws_send(message)) {
	time(&now);

        if ((now-last_init)>= RETRY_INTERVAL) {
	  printf("Trying to init new connection!");
	  time(&last_init);
	  init_ws_client(boat_id);
        }  
      } 
    }

    time(&now);
    if ( (now - last_sync_state) >= FULL_SYNC_INTERVAL) {
      if (get_nmea_state(message)) {
	//printf("Sending full state: %s\n", message);
	ws_send(message);
      }
      time(&last_sync_state);
    }
  }
}
