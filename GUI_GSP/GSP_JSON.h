/**
 * @file GSP_JSON.h
 * @brief Парсер JSON для библиотеки GUI_GSP.
 *        Работает с разными источниками данных через GSP_StreamReader.
 */
#ifndef GSP_JSON_H
#define GSP_JSON_H

#define GSP_JSON_MAX_LEN_KEY   30
#define GSP_JSON_MAX_LEN_VALUE 50
#define GSP_JSON_MAX_VALUES    40

typedef struct
{
    char key[GSP_JSON_MAX_LEN_KEY];
    char value[GSP_JSON_MAX_LEN_VALUE];
    int count;
    char *values[GSP_JSON_MAX_VALUES];
} GSP_JSON_ParseResult;

extern GSP_JSON_ParseResult GSP_JSON_Result;

int GSP_JSON_Init(void);
void GSP_JSON_Parse(unsigned int address, unsigned int size, const char *target_key);
void GSP_JSON_FreeResult(void);

#endif /* GSP_JSON_H */
