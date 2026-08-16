/**
 * @file GSP_JSON.c
 * @brief Парсер JSON через GSP_StreamReader.
 */
#include "GSP_JSON.h"
#include "GUI_GSP/Drivers/GSP_Driver.h"
#include <string.h>
#include <stdbool.h>

GSP_JSON_ParseResult GSP_JSON_Result = {0};

static char* json_strdup(const char *s)
{
    if(!s) return NULL;
    GSP_AllocFn alloc = GSP_GetAlloc();
    if(!alloc) return NULL;
    size_t len = strlen(s) + 1;
    char *dup = (char *)alloc(len);
    if(dup)
        strcpy(dup, s);
    return dup;
}

static void save_value_to_result(int *value_pos_ptr)
{
    if(*value_pos_ptr >= GSP_JSON_MAX_LEN_VALUE) *value_pos_ptr = GSP_JSON_MAX_LEN_VALUE - 1;
    GSP_JSON_Result.value[*value_pos_ptr] = '\0';

    if(GSP_JSON_Result.count < GSP_JSON_MAX_VALUES)
    {
        char *dup_str = json_strdup((const char *)GSP_JSON_Result.value);
        if(dup_str)
            GSP_JSON_Result.values[GSP_JSON_Result.count++] = dup_str;
    }
    memset(GSP_JSON_Result.value, 0, sizeof(GSP_JSON_Result.value));
    *value_pos_ptr = 0;
}

int GSP_JSON_Init(void)
{
    memset(GSP_JSON_Result.values, 0, sizeof(GSP_JSON_Result.values));
    GSP_JSON_Result.count = 0;
    return 0;
}

void GSP_JSON_Parse(unsigned int address, unsigned int size, const char *target_key)
{
    const GSP_StreamReader *reader = GSP_GetStreamReader();
    if(!reader || !reader->start || !reader->read_byte || !reader->stop || !target_key)
    {
        GSP_JSON_FreeResult();
        return;
    }

    bool in_string = false;
    bool in_array = false;
    bool found_key = false;
    int key_pos = 0;
    int value_pos = 0;
    bool end_parser = false;

    GSP_JSON_Result.count = 0;
    GSP_JSON_FreeResult();

    reader->start((long)address);

    int len = 0;
    while((++len < (int)size) && !end_parser)
    {
        int ch_val = reader->read_byte();
        if(ch_val < 0) break;
        char ch = (char)ch_val;

        if(ch == '"')
        {
            in_string = !in_string;
            if(!in_string && found_key && !in_array)
            {
                save_value_to_result(&value_pos);
                found_key = false;
            }
            continue;
        }

        if(in_string)
        {
            if(found_key)
            {
                if(ch == '~') ch = '\n';
                if(ch == '|') ch = '\0';
                if(value_pos < GSP_JSON_MAX_LEN_VALUE - 1)
                    GSP_JSON_Result.value[value_pos++] = ch;
            }
            else
            {
                if(key_pos < GSP_JSON_MAX_LEN_KEY - 1)
                    GSP_JSON_Result.key[key_pos++] = ch;
            }
            continue;
        }

        if(ch == ':' && !in_array)
        {
            if(key_pos >= GSP_JSON_MAX_LEN_KEY) key_pos = GSP_JSON_MAX_LEN_KEY - 1;
            GSP_JSON_Result.key[key_pos] = '\0';
            key_pos = 0;
            if(strcmp((const char *)GSP_JSON_Result.key, target_key) == 0)
                found_key = true;
            continue;
        }

        if(ch == '[') { in_array = true; continue; }

        if(ch == ']' && in_array)
        {
            if(found_key)
            {
                save_value_to_result(&value_pos);
                end_parser = true;
            }
            found_key = false;
            in_array = false;
            continue;
        }

        if((ch == ',' || ch == '}') && in_array && found_key)
        {
            save_value_to_result(&value_pos);
            if(ch == '}') { found_key = false; in_array = false; }
            continue;
        }

        if((ch == '{') && in_array) { in_array = false; continue; }

        if(ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t') continue;

        if(ch == '}')
        {
            memset(GSP_JSON_Result.key, 0, sizeof(GSP_JSON_Result.key));
            key_pos = 0;
            continue;
        }
    }

    reader->stop();

    if(GSP_JSON_Result.count == 0)
    {
        GSP_FreeFn free_fn = GSP_GetFree();
        if(free_fn)
        {
            for(int i = 0; i < GSP_JSON_MAX_VALUES; i++)
            {
                if(GSP_JSON_Result.values[i])
                {
                    free_fn(GSP_JSON_Result.values[i]);
                    GSP_JSON_Result.values[i] = NULL;
                }
            }
        }
        memset(GSP_JSON_Result.values, 0, sizeof(GSP_JSON_Result.values));
    }
}

void GSP_JSON_FreeResult(void)
{
    GSP_FreeFn free_fn = GSP_GetFree();
    if(free_fn)
    {
        for(int i = 0; i < GSP_JSON_MAX_VALUES; i++)
        {
            if(GSP_JSON_Result.values[i])
            {
                free_fn(GSP_JSON_Result.values[i]);
                GSP_JSON_Result.values[i] = NULL;
            }
        }
    }
    GSP_JSON_Result.count = 0;
}
