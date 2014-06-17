#ifndef CTL_H_INCLUDED
#define CTL_H_INCLUDED



void ctl_loop(char *dom);
void *outer_loop_thread(void *args);
int cmd_create_nvi(int soc, int cmd_i, int argc, char *argv[]);



#endif /* CTL_H_INCLUDED */
