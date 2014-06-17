#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED



struct config {
	int line_no;
	int param_no;
	char value[DEFAULT_BUFLEN];
};



int get_config(char *config_path, struct config *conf);
int set_config(struct config *conf);



#endif /* CONFIG_H_INCLUDED */
