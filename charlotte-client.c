#include <stdio.h>
#include <strings.h>
#include "wsclient.h"

#define BUFSIZE 4096

int
main(int argc, char **argv)
{
  char str[BUFSIZE], message[BUFSIZE];
  char boat_id[256];

  strcpy(boat_id, argv[1]);

  int status = init_ws_client(boat_id);

  /* while (1) {
    sleep(1);
    ws_send(boat_id);
  } */

  while (fgets(str, sizeof str, stdin) != NULL) {
    //printf("IN:%s\n", str);
    int hasdiff = parse_nmea(str, message);
    if (hasdiff) {
      printf("sending %s\n", message);
      ws_send(message);
    }
  }
}
