#define CALIB_SPEED	"speed"
#define CALIB_AWA	"awa"

#define KEYLEN	50
#define VALUELEN 100

#define MAXITEMS 100

struct calib_item {
  char key[KEYLEN];
  double fixed; 
  int    fixed_isnull;
  double linear_x;

  int linear_x_isnull;
  double linear_k;
  int linear_k_isnull;

};

struct calib {
  unsigned long time; 
  struct calib_item items[MAXITEMS]; 
};

cJSON *get_calib_state();
void
init_calib ();
void
save_calib ();
void
process_calib_state (cJSON * json);
void
process_calib_cmd (cJSON * json);
struct calib_item *
get_calib_item (char *key);

double
get_calib (char *key, double src_value);
