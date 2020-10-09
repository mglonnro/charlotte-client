int 	ws_init(char *id, uv_loop_t *loop);
int	ws_write(char *buf, int len);
void	ws_service();
void	ws_destroy();

int
ws_write_client (char *buf, int len);
int
ws_write_cloud (char *buf, int len);
