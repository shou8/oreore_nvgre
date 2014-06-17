#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED



/*
 * You must use this function to initialize
 * logging information.
 */
void init_log(void);

void enable_debug(void);
void disable_debug(void);
int get_status(void);

void log_stderr(const char *fmt, ...);

void enable_syslog(void);
void disable_syslog(void);

void log_debug(const char *fmt, ...);
void log_info(const char *fmt, ...);
void log_warn(const char *fmt, ...);
void log_err(const char *fmt, ...);
void log_crit(const char *fmt, ...);

/*
 * Using perror
 */
void log_pwarn(const char *str);
void log_perr(const char *str);
void log_pcrit(const char *str);

/*
 * Logging and Copy to "buf"
 */
void log_berr(char *buf, const char *fmt, ...);
void log_bperr(char *buf, const char *str);

void log_iexit(const char *fmt, ...);
void log_cexit(const char *fmt, ...);
void log_pcexit(const char *str);



#endif /* LOG_H_INCLUDED */

