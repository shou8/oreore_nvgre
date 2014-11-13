#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <arpa/inet.h>

#include "base.h"
#include "util.h"
#include "cmd.h"
#include "nvgre.h"
#include "config.h"
#include "log.h"



char param[][DEFAULT_BUFLEN] = {
	"multicast_address",
	"multicast_interface",
	"unix_socket",
	"nvgre_instance"
};



static int match_char(char c, char *str, int len);
static int rm_extra_right_char(char *str, char *rm_chars);
static char *rm_extra_left_char(char *str, char *rm_chars);
static char *rm_extra_char(char *str);



int get_config(char *config_path, struct config *conf)
{
	FILE *fp;

	if ((fp = fopen(config_path, "r")) == NULL)
		return -1;

	char bracket;
	char *p, *param_c, *val_c, line[DEFAULT_BUFLEN];
	int param_size = sizeof(param) / sizeof(param[0]);
	int line_no = 0;
	int i;

	int len = 0;

	while (fgets(line, DEFAULT_BUFLEN, fp) != NULL) {

		line_no++;

		// if this line includes '#', the point is regarded as end of line
		bracket = 0;
		for (p=line; *p != '\0'; p++) {
			if ( *p == '"' || *p == '\'' ) {
				bracket ^= *p;
				break;
			} else if ( *p == '#' && bracket == 0 ) {
				*p = '\0';
				break;
			}
		}

		// Remove right extra space and tabs
		if (rm_extra_right_char(line, " \t") == 0)
			continue;

		// Delete left extra space and tabs
		param_c = rm_extra_left_char(line, " \t");

		// Search '=' separator
		val_c = strchr(param_c, '=');
		if (val_c == NULL) {
			log_err("line %d: No '=' separator\n", line_no);
			return -2;
		}

		*val_c = '\0';
		val_c++;

		for (i=0; i<param_size; i++) {
			param_c = rm_extra_char(param_c);
			if (param_c == NULL) return -4;
			if (str_cmp(param[i], param_c)) break;
		}

		if (i == param_size) {
			log_err("line %d: No such parameter\n", line_no);
			return -3;
		}

		val_c = rm_extra_char(val_c);
		if (val_c == NULL) {
			log_err("line %d: Invalid bracket\n", line_no);
			return -4;
		}

		conf[len].param_no = i;
		conf[len].line_no = line_no;
		strncpy(conf[len].value, val_c, DEFAULT_BUFLEN);
		len++;
	}

	return len;
}



static int match_char(char c, char *str, int len)
{
	int i;
	for (i = 0; i < len; i++)
		if (c == str[i]) return 1;

	return 0;
}



static int rm_extra_right_char(char *str, char *rm_chars)
{
	int i = strlen(str) - 1;
	if (str[i] == '\n') i--;

	int rm_chars_len = strlen(rm_chars);
	for ( ; i >= 0; i--)
		if (! match_char(str[i], rm_chars, rm_chars_len)) break;

	str[++i] = '\0';
	return i;
}



static char *rm_extra_left_char(char *str, char *rm_chars)
{
	char *p;
	int rm_chars_len = strlen(rm_chars);
	for (p=str; *p != '\0'; p++)
		if (! match_char(*p, rm_chars, rm_chars_len)) break;
	return p++;
}



static char *rm_extra_char(char *str)
{
	rm_extra_right_char(str, " \t");
	str = rm_extra_left_char(str, " \t");

	// for bracket
	int len = strlen(str);
	if (str[0] == '"' || str[0] == '\'') {
		if (str[len-1] != str[0]) return NULL; // Backet is inconsistency
		str[len-1] = '\0';
		str++;
	}

	return str;
}



int set_config(struct config *conf)
{
	switch (conf->param_no) {
		case 0:
			strncpy(nvgre.cmaddr, conf->value, DEFAULT_BUFLEN);
			break;
		case 1:
			if (nvgre.if_name == NULL)
				nvgre.if_name = (char *)malloc(sizeof(char) * DEFAULT_BUFLEN);
			strncpy(nvgre.if_name, conf->value, DEFAULT_BUFLEN);
			break;
		case 2:
			strncpy(nvgre.udom, conf->value, DEFAULT_BUFLEN);
			break;
		case 3: {
			char *argv[] = {"create", conf->value};
			if (cmd_create_nvi(2, 0, 2, argv) != SUCCESS) return -1;
			break;
		}
	}

	return 0;
}



