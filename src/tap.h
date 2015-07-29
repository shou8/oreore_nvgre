#ifndef IFTAP_H_INCLUDED
#define IFTAP_H_INCLUDED



int tap_alloc(char *dev);
int tap_up(char *dev);
#ifndef OS_LINUX
void tap_destroy(char *dev);
#ifndef OS_DARWIN
int tap_rename(char *oldName, char *newName);
#endif /* OS_DARWIN */
#endif /* OS_LINUX */



#endif /* IFTAP_H_INCLUDED */
