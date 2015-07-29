#ifndef TABLE_H_INCLUDED
#define TABLE_H_INCLUDED

#include <stdlib.h>
#include <sys/types.h>



#define TABLE_SIZE 4096
//#define TABLE_SIZE 1
#define DEFAULT_TABLE_SIZE TABLE_SIZE



typedef struct _macaddr_table_
{
	int usoc;
	uint8_t hw_addr[MAC_LEN];
	struct sockaddr_storage addr;
	time_t time;
} mac_tbl;



typedef struct _list_
{
	mac_tbl *data;
	struct _list_ *pre;
	struct _list_ *next;
} list;



list **init_table(unsigned int size);
mac_tbl *find_data(list **table, uint8_t *eth_addr);
int add_data(list **table, uint8_t *hw_addr, struct sockaddr_storage *vtep_addr);
int del_data(list **table, uint8_t *hw_addr);
list **clear_table_all(list **table);
int clear_table_timeout(list **table, int cache_time);
unsigned int get_table_size(void);



#endif /* UTIL_H_INCLUDED */

