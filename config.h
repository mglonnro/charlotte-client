#define KEYLEN	50
#define VALUELEN 100

#define MAXITEMS 100

struct config_item {
  char key[KEYLEN];
  char value[VALUELEN];
};

struct config {
  unsigned long time; 
  struct config_item items[MAXITEMS]; 
};

cJSON *get_config_state();
