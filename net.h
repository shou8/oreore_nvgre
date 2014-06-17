#ifndef NET_H_INCLUDED
#define NET_H_INCLUDED

#include "nvgre.h"



int outer_loop(int soc);
int inner_loop(nvgre_i *nvi);



#endif /* NET_H_INCLUDED */

