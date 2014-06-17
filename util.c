#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#include "base.h"



#define CTL_BUFLEN DEFAULT_BUFLEN



void split_32to8(uint32_t val, uint8_t *arr) {

	arr[0] = (uint8_t)(val >> 16);
	arr[1] = (uint8_t)(val >> 8);
	arr[2] = (uint8_t)(val);
}



uint32_t str2uint8arr(char *str, uint8_t *arr) {

	uint32_t val = strtoul(str, NULL, 0);
	split_32to8(val, arr);
	return val;
}



int str_cmp(const char *p1, const char *p2) {

	int len = strlen(p1);
	return (len == strlen(p2) && strncmp(p1, p2, len) == 0); 
}



/*
char *pad_str(char *buf, const char *str) {

	strncpy(buf, str, CTL_BUFLEN);
	return buf + strlen(str);
}
*/



int get32and8arr(char *buf, char *str, uint32_t *val, uint8_t *arr) {

	errno = 0;
	*val = str2uint8arr(str, arr);

	switch (errno) {
		case EINVAL:
			snprintf(buf, CTL_BUFLEN, "Cannot convert, Not number: %s.\n", str);
			return CMD_FAILED;
			break;
		case ERANGE:
			snprintf(buf, CTL_BUFLEN, "Invalid range: %s\n", str);
			return CMD_FAILED;
			break;
	}

	if (*val == 0 && !(str_cmp(str, "0") || str_cmp(str, "0x0"))) {
		snprintf(buf, CTL_BUFLEN, "Invalid number: %s\n", str);
		return CMD_FAILED;
	}

	return SUCCESS;
}
