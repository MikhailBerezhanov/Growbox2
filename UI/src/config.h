#ifndef _MY_CONF_H_ 
#define _MY_CONF_H_

#include <libconfig.h>

#define ADD_SIGN 	" + "
#define NOT_FOUND_SIGN	" - "

int conf_init(const char* config_path);
void conf_deinit();

void conf_str(const char *name, char *dest, const char *dflt) ;
void conf_int(const char *name, int *dest, int dflt, const char *dim);
void conf_int64(const char *name, long long int *dest, long long int dflt);
void conf_bool(const char *name, bool *dest, bool dflt);

#endif


