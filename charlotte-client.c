#include <stdio.h>
#include <strings.h>
#include "wsclient.h"

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
  

  strcpy(boat_id, argv[1]);

  init_ws_client(boat_id);
  init_nmea_parser();

  time(&last_sync_state);

  while (fgets(str, sizeof str, stdin) != NULL) {

    int hasdiff = parse_nmea(str, message);
    if (hasdiff) {
      //printf("sending %s\n", message);

      if (!ws_send(message)) {
	time(&now);

        printf("time elapsed since last init: %ld\n", (now-last_init));	
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
