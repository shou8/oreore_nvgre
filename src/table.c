#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <limits.h>
//#include <malloc.h>

#include "netutil.h"
#include "table.h"
#include "util.h"

#ifdef DEBUG
#include "log.h"
#endif



#define TABLE_MIN	1024
//#define TABLE_MIN	1



static unsigned int table_size = 0;



static void to_head(list **root, list *lp);
static list *find_list(list **table, uint8_t *eth_addr);
static list **recreate_table(list **table);
static void del_after(list *lp, int *i);



list **init_table(unsigned int size) // hash table size 
{
	table_size = size % UINT_MAX;
	if (table_size < TABLE_MIN) table_size = TABLE_MIN;
	unsigned int mem_size = table_size * sizeof(list *);
	list **table = (list **)malloc(sizeof(list *) * mem_size);
	if (table == NULL) return NULL;
	memset(table, 0, mem_size);

	return table;
}



static void to_head(list **root, list *lp)
{
	list *head = *root;
	if ( lp != head ) {

		list *pre = lp->pre;
		pre->next = lp->next;
		if (pre->next != NULL)
			(pre->next)->pre = pre;

		lp->pre = NULL;
		lp->next = head;
		head->pre = lp;
		*root = lp;
	}
}



static list *find_list(list **table, uint8_t *eth_addr)
{
	unsigned int key = *((unsigned int *)eth_addr) % table_size;
	list **root = table + key;
	list *p = *root;

	for ( ; p != NULL; p = p->next) {
		mac_tbl *mac_t = p->data;
		uint8_t *eth_p = mac_t->hw_addr;
		if (cmp_mac(eth_p, eth_addr) == 0) {
			to_head(root, p);
			return p;
		}
	}

	return NULL;
}



mac_tbl *find_data(list **table, uint8_t *eth)
{
	list *p = find_list(table, eth);
	if (p != NULL)
		return p->data;

	return NULL;
}



int add_data(list **table, uint8_t *hw_addr, struct sockaddr_storage *addr)
{
	mac_tbl *mt;
	list *lp = find_list(table, hw_addr);
	unsigned int key = *((unsigned int *)hw_addr) % table_size;
	list **root = table + key;
	list *head = *root;

#ifdef DEBUG
	if (get_status()) {
		printf("MAC: %02X%02X:%02X%02X:%02X%02X\n", hw_addr[0], hw_addr[1], hw_addr[2],
				hw_addr[3], hw_addr[4], hw_addr[5]);
	}
#endif /* DEBUG */

	if (lp == NULL) {

		*root = (list *)malloc(sizeof(list));
		if (*root == NULL) return -1;
		memset(*root, 0, sizeof(list));

		if (head != NULL) {
			(*root)->next = head;
			head->pre = *root;
		}

		head = *root;
		head->pre = NULL;
		head->data = (mac_tbl *)malloc(sizeof(mac_tbl));
		if (head->data == NULL) return -1;
		memset(head->data, 0, sizeof(mac_tbl));

		mt = head->data;
		memcpy(mt->hw_addr, hw_addr, MAC_LEN);
		memcpy(&(mt->addr), addr, sizeof(struct sockaddr_storage));
		mt->time = time(NULL);

		head = *root;

	} else {

		mt = lp->data;
		memcpy(mt->hw_addr, hw_addr, MAC_LEN);
		memcpy(&(mt->addr), addr, sizeof(struct sockaddr_storage));
		mt->time = time(NULL);

		if ( lp != head )
			to_head(root, lp);
	}

	return 0;
}



int del_data(list **table, uint8_t *hw_addr)
{
	list *lp = find_list(table, hw_addr);
	if (lp == NULL) return -1;

	list *pre = lp->pre;
	list *next = lp->next;

	if (pre == NULL) {
		unsigned int key = *((unsigned int *)hw_addr) % table_size;
		*(table + key) = next;
	} else {
		pre->next = next;
	}

	if (next != NULL) next->pre = pre;
	free(lp->data);
	free(lp);

	return 0;
}



list **clear_table_all(list **table)
{
	list **root;
	list *p;

	for (root = table; root-table < table_size; root++) {
		for (p = *root; p != NULL; p = p->next) {
			del_after(p, NULL);
			if (p == *root) *root = NULL;
			break;
		}
	}

	return recreate_table(table);
}



static list **recreate_table(list **table)
{
	free(table);
	return init_table(table_size);
}



static void del_after(list *lp, int *i)
{
	if (lp->next != NULL) del_after(lp->next, i);

	list *pre = lp->pre;
	if (pre != NULL) pre->next = NULL;
	free(lp->data);
	free(lp);
	if (i != NULL) (*i)++;
}



int clear_table_timeout(list **table, int cache_time)
{
	list **root;
	list *p;
	int entry_num = 0;
	time_t now = time(NULL);

	for (root = table; root-table < table_size; root++) {
		for (p = *root; p != NULL; p = p->next) {
			if ((p->data)->time + cache_time < now) {
				del_after(p, &entry_num);
				if (p == *root) *root = NULL;
				break;
			}
		}
	}

	return entry_num;
}



unsigned int get_table_size(void)
{
	return table_size;
}
