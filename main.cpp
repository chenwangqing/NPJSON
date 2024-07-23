
#include <stdio.h>
#include "NPJSON.h"
#include <malloc.h>
#include <stdlib.h>

void *NPJSON_Malloc(size_t size)
{
    return malloc(size);
}

void NPJSON_Free(void *ptr)
{
    free(ptr);
}

void *NPJSON_Realloc(void *ptr, size_t size)
{
    return realloc(ptr, size);
}

int main()
{
    char str[2048];
    // ------------------------------- 生成 -------------------------------
    NPJSON_Serialization_Begin(2048);
    NPJSON_Object_SetInt(ver, 123);
    NPJSON_Object_SetBool(is_type, false);
    NPJSON_Object_SetNumber(num, 1.34);
    NPJSON_Object_Start(info);
    NPJSON_Object_SetString(RSSI, "123344");
    NPJSON_Object_SetString(IMEI, "122345667755");
    NPJSON_Object_End();
    NPJSON_Array_Start(list);
    for (int i = 0; i < 10; i++)
        NPJSON_Array_AddInt(i);
    NPJSON_Array_End();
    NPJSON_Serialization_Complete();
    int s = NPJSON_Serialization_GetJsonStringLength();
    if (s > sizeof(str) - 1)
        s = sizeof(str) - 1;
    memcpy(str, NPJSON_Serialization_GetJsonString(), s);
    str[s] = '\0';
    NPJSON_Serialization_End();
    printf("%s\n", str);
    // ------------------------------- 解析 -------------------------------
    const char *msg = NULL;
    NPJSON_Builder_Begin(str, strlen(str), msg);
    int ver = 0;
    NPJSON_Object_GetInt(ver, ver);
    bool is_type = true;
    NPJSON_Object_GetBool(is_type, is_type);
    char tmp[64];
    NPJSON_Object_Enter(info);
    NPJSON_Object_GetString(IMEI, tmp, sizeof(tmp));
    NPJSON_Object_Exit();
    NPJSON_Array_Enter(list);
    int n   = NPJSON_Array_GetCount();
    int val = 0;
    for (int i = 0; i < n; i++) {
        NPJSON_Array_GetInt(val);
        printf("[%d] = %d\n", i, val);
    }
    NPJSON_Array_Exit();
    NPJSON_Builder_End();
    return 0;
}
