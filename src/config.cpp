
#include <stdio.h>
#include <string.h> 

#include "config.h"

#define DEBUG_MSG   1
#define DEBUG_MODULE_NAME	"[ CONF ] "
#include "dbgmsg.h"


static config_t cfg;

int conf_init(const char* config_path)
{
	config_init(&cfg);

	if( !config_read_file(&cfg, config_path) )
    {
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Failed to open config file '%s'", config_path);
        config_destroy(&cfg);
        return -1;
    }

    return 0;
}

void conf_deinit()
{
	config_destroy(&cfg);
}


void conf_str(const char *name, char *dest, const char *dflt) 
{ 
	const char *str;

	if(config_lookup_string(&cfg, name, &str)){
    	strcat(dest, str);
    	dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, ADD_SIGN " Setting '%s' to: '%s'\n", name, dest);
	}
	else if(dflt){
	  strcat(dest, dflt);
	  dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, NOT_FOUND_SIGN " '%s' not found. Using default instead: '%s'\n", name, dflt);
	}
	else dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, NOT_FOUND_SIGN " '%s' not found in config file\n", name);
}


void conf_int(const char *name, int *dest, int dflt, const char *dim)
{
	if(config_lookup_int(&cfg, name, dest)){ \
		dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, ADD_SIGN " Setting '%s' to: %d %s\n", name, *dest, dim);
	}
	else{
		*dest = dflt;
		dbgmsg_v(MSG_DEBUG| MSG_TO_FILE, NOT_FOUND_SIGN " '%s' not found. Using default instead: %d %s\n", name, *dest, dim);
	}
}


void conf_int64(const char *name, long long int *dest, long long int dflt)
{
	if(config_lookup_int64(&cfg, name, dest)){
		dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, ADD_SIGN " Setting '%s' to: %lli \n", name, *dest);
	}
	else{
		*dest = dflt;
		dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, NOT_FOUND_SIGN " '%s' not found. Using default instead: %lli \n", name, *dest);
	}
}


void conf_bool(const char *name, bool *dest, bool dflt)
{
	int tmp = 0;
  if(config_lookup_bool(&cfg, name, &tmp)){
  	*dest = tmp ? true : false;
    dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, ADD_SIGN " Setting '%s' to: %s\n", name,  *dest ? "true" : "false");
	}
  else{
    *dest = dflt;
    dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, NOT_FOUND_SIGN " '%s' not found. Using default instead: %s\n", name, *dest ? "true" : "false"); \
  }
}