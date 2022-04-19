#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <cJSON/cJSON.h>

#include "utils.h"
#include "jsont.h"

#define DEBUG_MSG   1
#define DEBUG_MODULE_NAME   "[ JSONT ] "
#include "dbgmsg.h"


#define GET_JSON_OBJ_STR(root, name, dest)  ( get_json_obj(root, name, &dest, sizeof(dest), _string) ) 
#define GET_JSON_OBJ_INT(root, name, dest)  ( get_json_obj(root, name, &dest, sizeof(dest), _integer) )
#define GET_JSON_OBJ_UINT32(root, name, dest)  ( get_json_obj(root, name, &dest, sizeof(dest), _uinteger32) )
#define GET_JSON_OBJ_UINT64(root, name, dest)  ( get_json_obj(root, name, &dest, sizeof(dest), _uinteger64) ) 
#define GET_JSON_OBJ_BOOL(root, name, dest)  ( get_json_obj(root, name, &dest, sizeof(dest), _boolean) )      
#define GET_JSON_OBJ_FLOAT(root, name, dest)  ( get_json_obj(root, name, &dest, sizeof(dest), _floating) )            
                    
#define PUT_JSON_STRING(root, name, value)  ( add_val_to_json_obj(root, name, &value, _string) ) 
#define PUT_JSON_UINT32(root, name, value)  ( add_val_to_json_obj(root, name, &value, _uinteger32) )
#define PUT_JSON_BOOL(root, name, value)    ( add_val_to_json_obj(root, name, &value, _boolean) )
#define PUT_JSON_INT(root, name, value)    ( add_val_to_json_obj(root, name, &value, _integer) )
#define PUT_JSON_FLOAT(root, name, value)   ( add_val_to_json_obj(root, name, &value, _floating) )

static bool get_json_obj(cJSON *root, const char *name, void* dest, size_t dest_size, obj_t type)
{
	cJSON *obj = NULL;

	obj = cJSON_GetObjectItemCaseSensitive(root, name);
	if (obj)
	{
		switch(type)
		{
			case _string: 
    			if(cJSON_IsString(obj))
    			{
    				if(dest_size){
                        if(dest_size < (strlen(obj->valuestring) + 1)){
                            errmsg_v(MSG_DEBUG | MSG_TO_FILE, "JSON String overflow. Buf size: '%zu', Needed: '%zu'\n", dest_size, (strlen(obj->valuestring) + 1));
                            memcpy(dest, obj->valuestring, dest_size-1);
                            return true;
                        }
                    }

                    memcpy(dest, obj->valuestring, strlen(obj->valuestring) + 1);  
                    return true;
    			}
    			dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "WARNING: Object '%s' is not string\n", name);
    			break;

            case _integer:
                if (cJSON_IsNumber(obj))
                {
                    memcpy(dest, &obj->valueint, sizeof(int));
                    return true;
                }
                dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "WARNING: Object '%s' is not integer\n", name);
                break;

			case _uinteger32: 
    			if (cJSON_IsNumber(obj))
    			{
                    *(uint32_t*)dest = (uint32_t)obj->valuedouble;
    				return true;
    			}
    			dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "WARNING: Object '%s' is not uinteger32\n", name);
    			break;

			case _uinteger64: 
    			if (cJSON_IsNumber(obj))
    			{
                    *(uint64_t*)dest = (uint64_t)obj->valuedouble;
    				return true;
    			}
    			dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "WARNING: Object '%s' is not uinteger64\n", name);
    			break;

            case _boolean:
                if (cJSON_IsBool(obj))
                {
                    if (cJSON_IsTrue(obj)) {
                         dbgmsg_v(MSG_TRACE, "[ JSONT ] Got 'true' value\n");
                        *(bool*)dest = true;
                    }
                    else {
                         dbgmsg_v(MSG_TRACE, "[ JSONT ] Got 'false' value\n");
                        *(bool*)dest = false;
                    }

                    return true;
                }
                dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "WARNING: Object '%s' is not boolean\n", name);
                break;

            case _floating:
                if (cJSON_IsNumber(obj))
                {
                    *(float*)dest = (float)obj->valuedouble;
                    return true;
                }
                dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "WARNING: Object '%s' is not float\n", name);
                break;

			default: dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "WARNING: Object '%s' has unsopperted type\n", name);
		}
	}

	dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "WARNING: Object '%s' not found\n", name);
	return false;
}

static bool add_val_to_json_obj(cJSON *obj, const char* name, void *data, obj_t type)
{
	cJSON *value = NULL;

	switch(type)
	{
		case _string:
			if ((value = cJSON_CreateString((char*)data)) == NULL) 
		    {
		        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "cJSON_CreateString() failed\n");
		        return false;
		    }
		    break;

        case _integer:
            if ((value = cJSON_CreateNumber(*(int*)data)) == NULL) 
            {
                errmsg_v(MSG_DEBUG | MSG_TO_FILE, "cJSON_CreateNumber() int failed\n");
                return false;
            }
            break;

		case _uinteger32:
		{
			if ((value = cJSON_CreateNumber(*(uint32_t*)data)) == NULL) 
		    {
		        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "cJSON_CreateNumber() uint32_t failed\n");
		        return false;
		    }
		    break;
		}

		case _uinteger64:
		{
			if ((value = cJSON_CreateNumber(*(uint64_t*)data)) == NULL) 
		    {
		        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "cJSON_CreateNumber() uint64_t failed\n");
		        return false;
		    }
		    break;
		}

        case _boolean:
        {
            cJSON_bool tmp = 0;
            if(*(bool*)data == true) {
                dbgmsg_v(MSG_TRACE, "[ JSONT ] Settings 'true' value\n");
                tmp = 1;
            }
            else { dbgmsg_v(MSG_TRACE, "[ JSONT ] Settings 'false' value\n"); }

            if ((value = cJSON_CreateBool(tmp)) == NULL) 
            {
                errmsg_v(MSG_DEBUG | MSG_TO_FILE, "cJSON_CreateNumber() uint64_t failed\n");
                return false;
            }
            break;
        }

        case _floating:
            if ((value = cJSON_CreateNumber(*(float*)data)) == NULL) 
            {
                errmsg_v(MSG_DEBUG | MSG_TO_FILE, "cJSON_CreateNumber() float failed\n");
                return false;
            }
            break;

		default: 
			dbgmsg_v(MSG_DEBUG | MSG_TO_FILE, "WARNING: Unsupported JSON objest type\n");
			return false;
	}

    cJSON_AddItemToObject(obj, name, value);

    return true;
}

const char* jsont_get_err_text(jerr_t err)
{
    switch(err)
    {
        case J_OK: return("JSON: OK");
        case J_INVALID_FORMAT: return("JSON: Invalid data format");
        case J_FIELD_MISSING: return("JSON: Data field missing");
        case J_PARSER_ERR: return("JSON: Parser error");
        case J_OUT_OF_RANGE: return("JSON: Data out of range");

        case J_UNKNOWN_ERR:
        default: return("JSON: Unknown error");
    }
}

// NOTE: Don't forget to free() ouput c-string JSON
char* content_resp_to_json(content_resp_t *resp)
{
    char *res = NULL;
    cJSON *root = cJSON_CreateObject();

    if (!root) 
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) { errmsg_v(MSG_DEBUG | MSG_TO_FILE, "cJSON_CreateObject() error: %s\n", error_ptr); }
        else { errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unknown cJSON error\n"); }

        EXIT(NULL);
    }

    if (!PUT_JSON_STRING(root, "temperature", resp->temperature)) EXIT(NULL);
    if (!PUT_JSON_STRING(root, "humidity", resp->humidity)) EXIT(NULL);
    if (!PUT_JSON_STRING(root, "water", resp->water)) EXIT(NULL);
    if (!PUT_JSON_STRING(root, "wind_in", resp->wind_in)) EXIT(NULL);
    if (!PUT_JSON_STRING(root, "wind_out", resp->wind_out)) EXIT(NULL);
    if (!PUT_JSON_STRING(root, "light", resp->light)) EXIT(NULL);

    res = cJSON_Print(root);
    if (!res) 
    {
        errmsg_v(MSG_DEBUG, "cJSON error: %s\n", cJSON_GetErrorPtr());
        EXIT(NULL);
    }

    //dbgmsg ("My '%s' JSON:\n%s\n", __FUNCTION__, res);

exit:
    if (root) cJSON_Delete(root);
    return res;
}


// NOTE: Don't forget to free() ouput c-string JSON
char* curr_sett_resp_to_json(curr_sett_t *resp)
{
    char *res = NULL;
    cJSON *root = cJSON_CreateObject();
    cJSON *lan = cJSON_CreateObject();
    cJSON *wifi = cJSON_CreateObject();
    cJSON *rtc = cJSON_CreateObject();
    cJSON *cpu = cJSON_CreateObject();

    if (!root || !lan || !wifi || !rtc || !cpu) 
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) { errmsg_v(MSG_DEBUG | MSG_TO_FILE, "cJSON_CreateObject() error: %s\n", error_ptr); }
        else { errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unknown cJSON error\n"); }
        if(lan) cJSON_Delete(lan);
        if(wifi) cJSON_Delete(wifi);
        if(rtc) cJSON_Delete(rtc);
        if(cpu) cJSON_Delete(cpu);
        EXIT(NULL);
    }

    cJSON_AddItemToObject(root, "LAN", lan);
    if( !cJSON_AddStringToObject(lan, "ip4", resp->lan.ip4) ) EXIT(NULL);
    if( !cJSON_AddStringToObject(lan, "ip3", resp->lan.ip3) ) EXIT(NULL);
    if( !cJSON_AddStringToObject(lan, "ip2", resp->lan.ip2) ) EXIT(NULL);
    if( !cJSON_AddStringToObject(lan, "ip1", resp->lan.ip1) ) EXIT(NULL);
    if( !cJSON_AddStringToObject(lan, "gw4", resp->lan.gw4) ) EXIT(NULL);
    if( !cJSON_AddStringToObject(lan, "gw3", resp->lan.gw3) ) EXIT(NULL);
    if( !cJSON_AddStringToObject(lan, "gw2", resp->lan.gw2) ) EXIT(NULL);
    if( !cJSON_AddStringToObject(lan, "gw1", resp->lan.gw1) ) EXIT(NULL);
    if( !cJSON_AddBoolToObject(lan, "dhcp", resp->lan.dhcp) ) EXIT(NULL);

    cJSON_AddItemToObject(root, "WIFI", wifi);
    if( !cJSON_AddBoolToObject(wifi, "ap_state", resp->wifi.ap_state) ) EXIT(NULL);

    cJSON_AddItemToObject(root, "RTC", rtc);
    if( !cJSON_AddStringToObject(rtc, "hour", resp->rtc.hour) ) EXIT(NULL);
    if( !cJSON_AddStringToObject(rtc, "min", resp->rtc.min) ) EXIT(NULL);
    if( !cJSON_AddStringToObject(rtc, "sec", resp->rtc.sec) ) EXIT(NULL);
    if( !cJSON_AddStringToObject(rtc, "mday", resp->rtc.mday) ) EXIT(NULL);
    if( !cJSON_AddStringToObject(rtc, "month", resp->rtc.month) ) EXIT(NULL);
    if( !cJSON_AddStringToObject(rtc, "year", resp->rtc.year) ) EXIT(NULL);

    cJSON_AddItemToObject(root, "CPU", cpu);
    if( !cJSON_AddStringToObject(cpu, "temperature", resp->cpu.temperature) ) EXIT(NULL);
    if( !cJSON_AddStringToObject(cpu, "app_ver", resp->cpu.app_version) ) EXIT(NULL);

    res = cJSON_Print(root);
    if (!res) {
        errmsg_v(MSG_DEBUG, "cJSON error: %s", cJSON_GetErrorPtr());
        EXIT(NULL);
    }

    dbgmsg_v(MSG_TRACE, "My '%s' JSON:\n%s\n", __FUNCTION__, res);

exit:
    if (root) cJSON_Delete(root);
    return res;
}


char* flc_req_to_json(flc_req_t *req)
{
    char *res = NULL;
    cJSON *root = cJSON_CreateObject();

    if (!root) 
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) { errmsg_v(MSG_DEBUG | MSG_TO_FILE, "cJSON_CreateObject() error: %s\n", error_ptr); }
        else { errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unknown cJSON error"); }

        EXIT(NULL);
    }

    if (strcmp(req->curr_time, "")) {if(!cJSON_AddStringToObject(root, "current_time", req->curr_time)) EXIT(NULL);}
    if (strcmp(req->start_time, "")) {if(!cJSON_AddStringToObject(root, "start_time", req->start_time)) EXIT(NULL);}
    if (strcmp(req->stop_time, "")) {if(!cJSON_AddStringToObject(root, "stop_time", req->stop_time)) EXIT(NULL);}
    if (req->ch_pwr.ch1 >= 0) {if(!cJSON_AddNumberToObject(root, "ch1_power", req->ch_pwr.ch1)) EXIT(NULL);}
    if (req->ch_pwr.ch2 >= 0) {if(!cJSON_AddNumberToObject(root, "ch2_power", req->ch_pwr.ch2)) EXIT(NULL);}
    if (req->ch_pwr.ch3 >= 0) {if(!cJSON_AddNumberToObject(root, "ch3_power", req->ch_pwr.ch3)) EXIT(NULL);}

    //dbgmsg_v(MSG_TRACE, "smooth_control: %d, smooth_value: %d\n", req->smooth, req->smooth_value);

    if (req->smooth > 0) {if(!cJSON_AddBoolToObject(root, "smooth_control", true)) EXIT(NULL);}
    else if (!req->smooth) {if(!cJSON_AddBoolToObject(root, "smooth_control", false)) EXIT(NULL);}
    if (req->smooth_value >= 0) {if(!cJSON_AddNumberToObject(root, "smooth_value", req->smooth_value)) EXIT(NULL);}

    res = cJSON_Print(root);
    if (!res) {
        errmsg_v(MSG_DEBUG, "cJSON error: %s\n", cJSON_GetErrorPtr());
        EXIT(NULL);
    }

    dbgmsg_v(MSG_TRACE, "My '%s' JSON:\n%s\n", __FUNCTION__, res);

exit:
    if (root) cJSON_Delete(root);
    return res;
}

jerr_t json_to_flc_resp(const char *json, flc_resp_t *resp)
{
    jerr_t res = J_UNKNOWN_ERR;
    cJSON *root = cJSON_Parse(json);

    memset(resp, 0, sizeof(flc_resp_t));

    if (!root)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) { errmsg_v(MSG_DEBUG | MSG_TO_FILE, "cJSON parser error: %s (%s)\n", error_ptr, json); }
        else { errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unknown cJSON error\n"); } 

        EXIT(J_PARSER_ERR);
    }

    if (!GET_JSON_OBJ_INT(root, "result_code", resp->res.code)) EXIT(J_FIELD_MISSING);
    if (!GET_JSON_OBJ_STR(root, "result_text", resp->res.text)) EXIT(J_FIELD_MISSING);

    GET_JSON_OBJ_STR(root, "current_dtime", resp->curr_dtime);
    GET_JSON_OBJ_STR(root, "start_time", resp->start_time);
    GET_JSON_OBJ_STR(root, "stop_time", resp->stop_time);
    GET_JSON_OBJ_STR(root, "power_state", resp->power_state);
    GET_JSON_OBJ_INT(root, "ch1_power", resp->ch_pwr.ch1);
    GET_JSON_OBJ_INT(root, "ch2_power", resp->ch_pwr.ch2);
    GET_JSON_OBJ_INT(root, "ch3_power", resp->ch_pwr.ch3); 
    GET_JSON_OBJ_BOOL(root, "smooth_control", resp->smooth);
    GET_JSON_OBJ_INT(root, "smooth_value", resp->smooth_value);
    
    res = J_OK; // Success

exit:
    if(root) cJSON_Delete(root);
    return res;
}


char* res_to_json(res_t *resp)
{
    char *res = NULL;
    cJSON *root = cJSON_CreateObject();

    if (!root) 
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) { errmsg_v(MSG_DEBUG | MSG_TO_FILE, "cJSON_CreateObject() error: %s\n", error_ptr); }
        else { errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unknown cJSON error\n"); }

        EXIT(NULL);
    }

    cJSON_AddStringToObject(root, "result_text", resp->text);
    cJSON_AddNumberToObject(root, "result_code", resp->code);

    res = cJSON_Print(root);
    if (!res) {
        errmsg_v(MSG_DEBUG, "cJSON error: %s\n", cJSON_GetErrorPtr());
        EXIT(NULL);
    }

    //dbgmsg ("My '%s' JSON:\n%s\n", __FUNCTION__, res);

exit:
    if (root) cJSON_Delete(root);
    return res;
}


char* flc_status_to_json(flc_status_t *resp)
{
    char *res = NULL;
    cJSON *root = cJSON_CreateObject();

    if (!root) 
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) { errmsg_v(MSG_DEBUG | MSG_TO_FILE, "cJSON_CreateObject() error: %s\n", error_ptr); }
        else { errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unknown cJSON error\n"); }

        EXIT(NULL);
    }

    cJSON_AddStringToObject(root, "result_text", resp->res.text);
    cJSON_AddNumberToObject(root, "result_code", resp->res.code);

    cJSON_AddBoolToObject(root, "power_state", resp->power_state);
    cJSON_AddStringToObject(root, "curr_hour", resp->curr_time_hour);
    cJSON_AddStringToObject(root, "curr_min", resp->curr_time_min);
    cJSON_AddStringToObject(root, "curr_sec", resp->curr_time_sec);
    cJSON_AddStringToObject(root, "start_hour", resp->start_time_hour);
    cJSON_AddStringToObject(root, "start_min", resp->start_time_min);
    cJSON_AddStringToObject(root, "start_sec", resp->start_time_sec);
    cJSON_AddStringToObject(root, "stop_hour", resp->stop_time_hour);
    cJSON_AddStringToObject(root, "stop_min", resp->stop_time_min);
    cJSON_AddStringToObject(root, "stop_sec", resp->stop_time_sec);
    cJSON_AddNumberToObject(root, "white_power", resp->ch_pwr.ch1);
    cJSON_AddNumberToObject(root, "red_power", resp->ch_pwr.ch2);
    cJSON_AddNumberToObject(root, "blue_power", resp->ch_pwr.ch3);
    cJSON_AddBoolToObject(root, "smooth", resp->smooth);
    cJSON_AddNumberToObject(root, "smooth_value", resp->smooth_value);

    res = cJSON_Print(root);
    if (!res) {
        errmsg_v(MSG_DEBUG, "cJSON error: %s\n", cJSON_GetErrorPtr());
        EXIT(NULL);
    }

    //dbgmsg_v(MSG_TRACE, "My '%s' JSON:\n%s\n", __FUNCTION__, res);

exit:
    if (root) cJSON_Delete(root);
    return res;
}


jerr_t json_to_flc_ch_pwr(const char *json, flc_ch_pwr_t *req)
{
    jerr_t res = J_UNKNOWN_ERR;
    cJSON *root = cJSON_Parse(json);

    memset(req, -1, sizeof(flc_ch_pwr_t));

    if (!root)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) { errmsg_v(MSG_DEBUG | MSG_TO_FILE, "cJSON parser error: %s", error_ptr); }
        else { errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unknown cJSON error\n"); } 

        EXIT(J_PARSER_ERR);
    }

    GET_JSON_OBJ_INT(root, "ch1_power", req->ch1);
    GET_JSON_OBJ_INT(root, "ch2_power", req->ch2);
    GET_JSON_OBJ_INT(root, "ch3_power", req->ch3);
    
    res = J_OK;

exit:
    if(root) cJSON_Delete(root);
    return res;
}


jerr_t json_to_my_time(const char *json, my_time_t *req)
{
    jerr_t res = J_UNKNOWN_ERR;
    cJSON *root = cJSON_Parse(json);

    memset(req, 0, sizeof(my_time_t));

    if (!root)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) { errmsg_v(MSG_DEBUG | MSG_TO_FILE, "cJSON parser error: %s\n", error_ptr); }
        else { errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unknown cJSON error\n"); } 

        EXIT(J_PARSER_ERR);
    }

    if (!GET_JSON_OBJ_INT(root, "hour", req->hour)) EXIT(J_FIELD_MISSING);
    if (!GET_JSON_OBJ_INT(root, "min", req->min)) EXIT(J_FIELD_MISSING);
    if (!GET_JSON_OBJ_INT(root, "sec", req->sec)) EXIT(J_FIELD_MISSING);
    
    res = J_OK;

exit:
    if(root) cJSON_Delete(root);
    return res;
}


jerr_t json_to_flc_smooth(const char *json, bool *req)
{
    jerr_t res = J_UNKNOWN_ERR;
    cJSON *root = cJSON_Parse(json);

    if (!root)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) { errmsg_v(MSG_DEBUG| MSG_TO_FILE, "cJSON parser error: %s\n", error_ptr); }
        else { errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unknown cJSON error\n"); } 

        EXIT(J_PARSER_ERR);
    }

    if (!GET_JSON_OBJ_BOOL(root, "smooth_control", *req)) EXIT(J_FIELD_MISSING);

    res = J_OK;

exit:
    if(root) cJSON_Delete(root);
    return res;
}

jerr_t json_to_flc_smooth_value(const char *json, int *req)
{
    jerr_t res = J_UNKNOWN_ERR;
    cJSON *root = cJSON_Parse(json);

    if (!root)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) { errmsg_v(MSG_DEBUG | MSG_TO_FILE, "cJSON parser error: %s\n", error_ptr); }
        else { errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unknown cJSON error\n"); } 

        EXIT(J_PARSER_ERR);
    }

    if (!GET_JSON_OBJ_INT(root, "smooth_value", *req)) EXIT(J_FIELD_MISSING);
    
    res = J_OK;

exit:
    if(root) cJSON_Delete(root);
    return res;
}

char* coolers_state_to_json(coolers_state_t* resp)
{
    char* res = NULL;
    cJSON* root = cJSON_CreateObject();
    cJSON* result = cJSON_CreateObject();
    cJSON* coolers = cJSON_CreateArray();

    if (!root || !result || !coolers) 
    {
        const char* error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) { errmsg_v(MSG_DEBUG| MSG_TO_FILE, "cJSON_CreateObject() error: %s\n", error_ptr); }
        else { errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unknown cJSON error\n"); }

        if(result) cJSON_Delete(result);
        if(coolers) cJSON_Delete(coolers);
        EXIT(NULL);
    }

    if (!PUT_JSON_INT(result, "code", resp->res.code)) EXIT(NULL);
    if (!PUT_JSON_STRING(result, "text", resp->res.text)) EXIT(NULL);
    cJSON_AddItemToObject(root, "result", result);

    if(resp->arr) {
        for(uint i = 0; i < resp->arr_size; i++)
        {
            if(strcmp(resp->arr[i].type, "")){
                cJSON *elem = cJSON_CreateObject();

                if (!PUT_JSON_STRING(elem, "type", resp->arr[i].type)) EXIT(NULL);
                if (!PUT_JSON_INT(elem, "power", resp->arr[i].power)) EXIT(NULL);
                if (!PUT_JSON_STRING(elem, "start_hour", resp->arr[i].start_hour)) EXIT(NULL);
                if (!PUT_JSON_STRING(elem, "start_min", resp->arr[i].start_min)) EXIT(NULL);
                if (!PUT_JSON_STRING(elem, "start_sec", resp->arr[i].start_sec)) EXIT(NULL);
                if (!PUT_JSON_STRING(elem, "stop_hour", resp->arr[i].stop_hour)) EXIT(NULL);
                if (!PUT_JSON_STRING(elem, "stop_min", resp->arr[i].stop_min)) EXIT(NULL);
                if (!PUT_JSON_STRING(elem, "stop_sec", resp->arr[i].stop_sec)) EXIT(NULL);
                if (!PUT_JSON_STRING(elem, "wp_min", resp->arr[i].wp_min)) EXIT(NULL);
                if (!PUT_JSON_BOOL(elem, "is_on", resp->arr[i].is_on)) EXIT(NULL);

                cJSON_AddItemToArray(coolers, elem);
            }       
        }
        cJSON_AddItemToObject(root, "coolers_state", coolers);
    }
    else {
        if(coolers) cJSON_Delete(coolers);
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "No data array provided\n");
    }

    res = cJSON_Print(root);
    if (!res) {
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "cJSON error: %s\n", cJSON_GetErrorPtr());
        EXIT(NULL);
    }

    //dbgmsg_v(MSG_TRACE, "My '%s' JSON:\n%s\n", __FUNCTION__, res);

exit:
    if (root) cJSON_Delete(root);
    return res;
}

jerr_t json_to_coolers_power(const char *json, coolers_pwr_t* req)
{
    jerr_t res = J_UNKNOWN_ERR;
    cJSON *root = cJSON_Parse(json);

    if (!root)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) { errmsg_v(MSG_DEBUG, "cJSON parser error: %s\n", error_ptr); }
        else { errmsg_v(MSG_DEBUG, "Unknown cJSON error\n"); } 

        EXIT(J_PARSER_ERR);
    }

    if(!GET_JSON_OBJ_INT(root, "wind_in", req->in) && !GET_JSON_OBJ_INT(root, "wind_out", req->out) ) EXIT(J_FIELD_MISSING);
    
    res = J_OK;

exit:
    if(root) cJSON_Delete(root);
    return res;
}

jerr_t json_to_lan_settings(const char *json, lan_sett_t* req)
{
    jerr_t res = J_UNKNOWN_ERR;
    cJSON *root = cJSON_Parse(json);

    if (!root)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) { errmsg_v(MSG_DEBUG, "cJSON parser error: %s\n", error_ptr); }
        else { errmsg_v(MSG_DEBUG, "Unknown cJSON error\n"); } 

        EXIT(J_PARSER_ERR);
    }

    if(!GET_JSON_OBJ_STR(root, "ip4", req->ip4)) EXIT(J_FIELD_MISSING);
    if(!GET_JSON_OBJ_STR(root, "ip3", req->ip3)) EXIT(J_FIELD_MISSING); 
    if(!GET_JSON_OBJ_STR(root, "ip2", req->ip2)) EXIT(J_FIELD_MISSING);
    if(!GET_JSON_OBJ_STR(root, "ip1", req->ip1)) EXIT(J_FIELD_MISSING);
    if(!GET_JSON_OBJ_STR(root, "gw4", req->gw4)) EXIT(J_FIELD_MISSING);
    if(!GET_JSON_OBJ_STR(root, "gw3", req->gw3)) EXIT(J_FIELD_MISSING);
    if(!GET_JSON_OBJ_STR(root, "gw2", req->gw2)) EXIT(J_FIELD_MISSING);
    if(!GET_JSON_OBJ_STR(root, "gw1", req->gw1)) EXIT(J_FIELD_MISSING);
    if(!GET_JSON_OBJ_BOOL(root, "dhcp", req->dhcp)) EXIT(J_FIELD_MISSING);
    
    res = J_OK;

exit:
    if(root) cJSON_Delete(root);
    return res;
}

jerr_t json_to_rtc_settings(const char *json, rtc_sett_t* req)
{
    jerr_t res = J_UNKNOWN_ERR;
    cJSON *root = cJSON_Parse(json);

    if (!root)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) { errmsg_v(MSG_DEBUG, "cJSON parser error: %s\n", error_ptr); }
        else { errmsg_v(MSG_DEBUG, "Unknown cJSON error\n"); } 

        EXIT(J_PARSER_ERR);
    }

    GET_JSON_OBJ_STR(root, "hour", req->hour);
    GET_JSON_OBJ_STR(root, "min", req->min); 
    GET_JSON_OBJ_STR(root, "sec", req->sec);
    GET_JSON_OBJ_STR(root, "mday", req->mday);
    GET_JSON_OBJ_STR(root, "month", req->month);
    GET_JSON_OBJ_STR(root, "year", req->year);
    
    res = J_OK;

exit:
    if(root) cJSON_Delete(root);
    return res;
}

jerr_t json_to_th_period(const char* json, int* period_min)
{
    jerr_t res = J_UNKNOWN_ERR;
    cJSON *root = cJSON_Parse(json);

    if (!root)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) { errmsg_v(MSG_DEBUG, "cJSON parser error: %s\n", error_ptr); }
        else { errmsg_v(MSG_DEBUG, "Unknown cJSON error\n"); } 

        EXIT(J_PARSER_ERR);
    }

    if(!GET_JSON_OBJ_INT(root, "poll_period", *period_min)) EXIT(J_FIELD_MISSING);
    
    res = J_OK;

exit:
    if(root) cJSON_Delete(root);
    return res;
}

char* th_data_to_json(th_data_resp_t* resp)
{
    char *res = NULL;
    cJSON *root = cJSON_CreateObject();
    cJSON *result = cJSON_CreateObject();
    cJSON *th_data = cJSON_CreateArray();

    if (!root || !th_data || !result) 
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) { errmsg_v(MSG_DEBUG | MSG_TO_FILE, "cJSON_CreateObject() error: %s\n", error_ptr); }
        else { errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unknown cJSON error"); }

        if(th_data) cJSON_Delete(th_data);
        if(result) cJSON_Delete(result);

        EXIT(NULL);
    }

    if (!PUT_JSON_INT(result, "code", resp->res.code)) EXIT(NULL);
    if (!PUT_JSON_STRING(result, "text", resp->res.text)) EXIT(NULL);
    cJSON_AddItemToObject(root, "result", result);

    if (!PUT_JSON_INT(root, "poll_period", resp->poll_period)) EXIT(NULL);

    if(resp->arr) {
        for(size_t i = 0; i < resp->arr_size; i++)
        {
            if(strcmp(resp->arr[i].time, "")){
                cJSON *elem = cJSON_CreateObject();

                if (!PUT_JSON_FLOAT(elem, "temp", resp->arr[i].temp)) EXIT(NULL);
                if (!PUT_JSON_FLOAT(elem, "hum", resp->arr[i].hum)) EXIT(NULL);
                if (!PUT_JSON_STRING(elem, "time", resp->arr[i].time)) EXIT(NULL);

                cJSON_AddItemToArray(th_data, elem);
            }       
        }
        cJSON_AddItemToObject(root, "temp_hum_data", th_data);
    }
    else {
        if(th_data) cJSON_Delete(th_data);
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "No data array provided\n");
    }

    res = cJSON_PrintUnformatted(root);
    if (!res) 
    {
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "cJSON error: %s", cJSON_GetErrorPtr());
        EXIT(NULL);
    }

    dbgmsg_v(MSG_TRACE, "My '%s' JSON:\n%s\n", __FUNCTION__, res);

exit:
    if (root) cJSON_Delete(root);
    return res;
}



char* th_avail_to_json(th_avail_resp_t* resp)
{
    char *res = NULL;
    cJSON *root = cJSON_CreateObject();
    cJSON *result = cJSON_CreateObject();
    cJSON *th_avail = cJSON_CreateArray();

    if (!root || !th_avail || !result) 
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) { errmsg_v(MSG_DEBUG | MSG_TO_FILE, "cJSON_CreateObject() error: %s\n", error_ptr); }
        else { errmsg_v(MSG_DEBUG | MSG_TO_FILE, "Unknown cJSON error"); }

        if(th_avail) cJSON_Delete(th_avail);
        if(result) cJSON_Delete(result);

        EXIT(NULL);
    }

    if (!PUT_JSON_INT(result, "code", resp->res.code)) EXIT(NULL);
    if (!PUT_JSON_STRING(result, "text", resp->res.text)) EXIT(NULL);
    cJSON_AddItemToObject(root, "result", result);

    if(resp->arr) {
        // Create available dates Array   
        for(size_t i = 0; i < resp->arr_size; i++)
        {
            if(strcmp(resp->arr[i].date, "")){
                cJSON *elem = cJSON_CreateObject();

                if (!PUT_JSON_STRING(elem, "date", resp->arr[i].date)) EXIT(NULL);

                cJSON_AddItemToArray(th_avail, elem);
            } 
        }
        cJSON_AddItemToObject(root, "avail_dates", th_avail);
    }
    else{
        if(th_avail) cJSON_Delete(th_avail);
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "No avail array provided\n");
    }

    res = cJSON_Print(root);
    if (!res) 
    {
        errmsg_v(MSG_DEBUG | MSG_TO_FILE, "cJSON error: %s", cJSON_GetErrorPtr());
        EXIT(NULL);
    }

    dbgmsg_v(MSG_TRACE, "My '%s' JSON:\n%s\n", __FUNCTION__, res);

exit:
    if (root) cJSON_Delete(root);
    return res;
}

jerr_t json_to_th_date(const char* json, th_date_req_t* req)
{
    jerr_t res = J_UNKNOWN_ERR;
    cJSON *root = cJSON_Parse(json);

    if (!root)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) { errmsg_v(MSG_DEBUG, "cJSON parser error: %s\n", error_ptr); }
        else { errmsg_v(MSG_DEBUG, "Unknown cJSON error\n"); } 

        EXIT(J_PARSER_ERR);
    }

    if(!GET_JSON_OBJ_UINT32(root, "day", req->day)) EXIT(J_FIELD_MISSING);
    if(!GET_JSON_OBJ_UINT32(root, "month", req->month)) EXIT(J_FIELD_MISSING);
    GET_JSON_OBJ_UINT32(root, "year", req->year);

    res = J_OK;

exit:
    if(root) cJSON_Delete(root);
    return res;
}