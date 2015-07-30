#ifndef IFTAP_H_INCLUDED
#define IFTAP_H_INCLUDED



int tap_alloc(char *dev);
int tap_up(char *dev);
#ifndef __linux__
void tap_destroy(char *dev);
#ifdef __FreeBSD__
int tap_rename(char *oldName, char *newName);
#endif /* __FreeBSD__ */
#endif /* __linux__ */



#endif /* IFTAP_H_INCLUDED */
