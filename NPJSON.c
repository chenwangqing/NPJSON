
#include "NPJSON.h"
#include <stdarg.h>
#include <stdlib.h>

#define NEW(TYPE, SIZE)        (TYPE *)NPJSON_Malloc(SIZE * sizeof(TYPE))
#define DELETE(OBJ)            NPJSON_Free(OBJ)
#define REDIM(TYPE, OBJ, SIZE) (TYPE *)NPJSON_Realloc(OBJ, SIZE * sizeof(TYPE))

#define NPJSON_NULL 0

// 是否数值
#define IS_NUMBER(ch) ((ch >= '0' && ch <= '9') || ch == '-' || ch == '+' || ch == '.')
// 是否字符串
#define IS_STRING(ch) ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
// 跳过空白
#define SKIPBLANK                                                                              \
    while (p != strend && (*p == ' ' || *p == '\t' || *p == '\f' || *p == '\r' || *p == '\n')) \
        p++;

static const NPJSON_t NPJSON = {
    {
        NPJSON_Resolve,
        NPJSON_CreateUnSafeObject,
        NPJSON_CreateSafeObject,
        NPJSON_RObjectUnSafeToSafe,
        NPJSON_ResolveToSafeObject,
        NPJSON_DeleteSafeObject,
    },
    {
        NPJSON_CreateSynthesizer,
        NPJSON_SynthesizerClone,
        NPJSON_DeleteSynthesizer,
        NPJSON_CreateSObject,
        NPJSON_DeleteSObject,
    },
    {
        NPJSON_SObjectToSafeRObject,
        NPJSON_SafeRObjectToSObject,
        NPJSON_SafeRObjectClone,
        NPJSON_SObjectClone,
    },
};

const NPJSON_t *NPJSON_Import(void)
{
    return &NPJSON;
}

static size_t CM_vsnprintf(char *buf, size_t maxlen, const char *format, va_list val)
{
    if (maxlen == 0 || buf == NULL)
        return 0;
    int rs = vsnprintf(buf, maxlen, format, val);
    if (rs < 0)
        rs = 0;
    maxlen--;
    if ((size_t)rs > maxlen)
        rs = (int)maxlen;
    return rs;
}

static size_t CM_snprintf(char *buf, size_t maxlen, const char *format, ...)
{
    va_list vArgList;
    va_start(vArgList, format);
    size_t rs = CM_vsnprintf(buf, maxlen, format, vArgList);
    va_end(vArgList);
    return rs;
}

// ---------------------------------------------------------------------------------------------------------------------
//                                             | JSON 解析 |
// ---------------------------------------------------------------------------------------------------------------------

const char *NPJSON_ResolveValue(NPJSON_Result *re, NPJSON_ResolveFunc fun, const char *str, const char *strend, void *obj)
{
    // 获取值
    re->isArray    = 0;
    re->isBinValue = 0;
    re->isInteger  = 0;
    re->isNull     = 0;
    re->isNumber   = 0;
    re->isObject   = 0;
    re->isString   = 0;
    re->err        = str;
    do {
        char ch = *str;
        if (ch == '\"') {
            // 字符串处理
            str++;
            const char *tstr = str;
            while (str != strend &&
                   !(*str == '\"' && *(str - 1) != '\\') &&
                   *str != '\r' &&
                   *str != '\n')
                str++;
            if (str == strend || *str != '\"')
                return NPJSON_NULL;
            re->Val.String.ptr    = tstr;
            re->Val.String.length = str - tstr;
            re->isString          = 1;
            if (!fun(re->name, re, re->ArrIdx, obj))
                return NPJSON_NULL;
            re->Val.String.length = 0;
            re->Val.String.ptr    = NPJSON_NULL;
            str++;
            break;
        } else if (ch == '{') {
            // JSON对象处理
            re->str = str++;
            int lv  = 1;
            while (str != strend) {
                if (*str == '{')
                    lv++;
                if (*str == '}')
                    lv--;
                if (lv < 0)
                    break;
                if (lv == 0)
                    re->Strlength = str - re->str + 1;
                str++;
            }
            if (lv != 0)
                return NPJSON_NULL;
            re->isObject = 1;
            if (!fun(re->name, re, re->ArrIdx, obj))
                return NPJSON_NULL;
            break;
        } else if (ch == '[') {
            // JSON数组处理
            re->str = str++;
            int lv  = 1;
            while (str != strend) {
                if (*str == '[')
                    lv++;
                if (*str == ']')
                    lv--;
                if (lv < 0)
                    break;
                if (lv == 0)
                    re->Strlength = str - re->str + 1;
                str++;
            }
            if (lv != 0)
                return NPJSON_NULL;
            re->isObject = 1;
            re->isArray  = 1;
            if (!fun(re->name, re, re->ArrIdx, obj))
                return NPJSON_NULL;
            break;
        } else if (IS_STRING(ch)) {
            // 二值量处理
            const char STR_TRUE[]  = "true";
            const char STR_FLASE[] = "false";
            const char STR_NULL[]  = "null";
            if (strend - str >= (int)sizeof(STR_TRUE) - 1 && memcmp(str, STR_TRUE, sizeof(STR_TRUE) - 1) == 0) {
                re->isBinValue   = 1;
                re->Val.BinValue = true;
                re->Val.Value    = 1;
                if (!fun(re->name, re, re->ArrIdx, obj))
                    return NPJSON_NULL;
                str += sizeof(STR_TRUE) - 1;
                re->isBinValue = 0;
                break;
            }
            if (strend - str >= (int)sizeof(STR_FLASE) - 1 && memcmp(str, STR_FLASE, sizeof(STR_FLASE) - 1) == 0) {
                re->isBinValue   = 1;
                re->Val.BinValue = false;
                re->Val.Value    = 0;
                if (!fun(re->name, re, re->ArrIdx, obj))
                    return NPJSON_NULL;
                str += sizeof(STR_FLASE) - 1;
                re->isBinValue = 0;
                break;
            }
            if (strend - str >= (int)sizeof(STR_NULL) - 1 && memcmp(str, STR_NULL, sizeof(STR_NULL) - 1) == 0) {
                re->isNull = 1;
                if (!fun(re->name, re, re->ArrIdx, obj))
                    return NPJSON_NULL;
                str += sizeof(STR_NULL) - 1;
                break;
            }
            return NPJSON_NULL;
        } else if (IS_NUMBER(ch)) {
            const char *tstr = str;
            // 安全检查 ， atoll、atof 是不安全的
            bool isXs = false;   // 是否为小数
            bool isE  = false;
            str++;
            while (str != strend) {
                if (*str == 'e' || *str == 'E')
                    isE = true;
                if (*str == '.')
                    isXs = true;
                str++;
            }
            if (isE) {
                // 科学计数
                re->Val.Number = atof(tstr);
                re->Val.Value  = (long long)re->Val.Number;
            } else {
                re->Val.Number = atof(tstr);
                re->Val.Value  = atoll(tstr);
            }
            re->isNumber     = 1;
            re->isInteger    = !isXs;
            re->Val.BinValue = re->Val.Value != 0;
            if (!fun(re->name, re, re->ArrIdx, obj))
                return NPJSON_NULL;
            break;
        } else
            return NPJSON_NULL;
    } while (false);
    // 跳过空白
    const char *p = str;
    SKIPBLANK;
    return p;
}

bool NPJSON_ResolveExev(NPJSON_Result *re, NPJSON_ResolveFunc fun, void *obj, bool getvalue)
{
    if (re == NPJSON_NULL || re->str == NPJSON_NULL || re->Strlength <= 0 || fun == NPJSON_NULL)
        return false;
    const char *p      = re->str;
    const char *strend = re->str + re->Strlength;
    re->str            = NPJSON_NULL;
    re->Strlength      = 0;
    SKIPBLANK;
    *re->name = '\0';
    re->err   = p;
    if (*p == '{') {
        p++;
        while (p != strend) {
            SKIPBLANK;
            if (*p == '}')
                return true;   // 空内容
            // 获取对象名称
            if (*p != '\"')
                return false;
            re->err       = p;
            const char *s = ++p;
            while (p != strend && (p[-1] == '\\' || (*p != '\"' && *p != '\r' && *p != '\n')))
                p++;
            if (p == strend || *p != '\"')
                return false;   // 对象名称获取失败
            int n = p - s;
            if (n > NPJSON_NAME_LEN)
                n = NPJSON_NAME_LEN;
            memcpy(re->name, s, n);
            re->name[n] = '\0';
            // 检查 :
            p++;
            SKIPBLANK;
            if (p == strend || *p != ':')
                return false;   // : 获取失败
            re->err = p;
            p++;
            SKIPBLANK;
            s = p;
            // 获取对象范围
            if (*s == '[' || *s == '{') {
                // 对象
                int lv = 1;
                while (p != strend) {
                    if (lv == 1 && *p == '}')
                        break;
                    if ((*s == '{' && *p == '{') || (*s == '[' && *p == '['))
                        lv++;
                    else if ((*s == '{' && *p == '}') || (*s == '[' && *p == ']'))
                        lv--;
                    if (lv < 0)
                        return false;
                    if (lv == 1 && *p == ',')
                        break;
                    if (lv == 0 && ((*s == '{' && *p == '}') || (*s == '[' && *p == ']')))
                        break;
                    p++;
                }
            } else if (*s == '\"') {
                // 字符串
                p++;
                while (p != strend) {
                    if (p[-1] != '\\' && *p == '\"')
                        break;
                    p++;
                }
                if (*p == '\"')
                    p++;
            } else {
                // 数值
                while (p != strend) {
                    if (*p == '{' || *p == '[' || *p == ']')
                        return false;
                    if (*p == ',' || *p == ']' || *p == '}')
                        break;
                    p++;
                }
            }
            if (p == strend)
                return false;
            re->err = s;
            // 获取对象值
            if (NPJSON_ResolveValue(re, fun, s, p, obj) != p)
                return false;
            p++;
            SKIPBLANK;
        }
    } else if (*p == '[') {
        p++;
        while (p != strend) {
            SKIPBLANK;
            if (*p == ']')
                return true;   // 空内容
            // 获取对象值
            const char *s = p;
            re->err       = s;
            if (*s == '[' || *s == '{') {
                // 对象
                int lv = 1;
                while (p != strend) {
                    if (lv == 1 && *p == ']')
                        break;
                    if ((*s == '{' && *p == '{') || (*s == '[' && *p == '['))
                        lv++;
                    else if ((*s == '{' && *p == '}') || (*s == '[' && *p == ']'))
                        lv--;
                    if (lv < 0)
                        return false;
                    if (lv == 1 && *p == ',')
                        break;
                    if (lv == 0 && ((*s == '{' && *p == '}') || (*s == '[' && *p == ']')))
                        break;
                    p++;
                }
            } else {
                // 数值
                while (p != strend) {
                    if (*p == '{' || *p == '[')
                        return false;
                    if (*p == ',' || *p == ']' || *p == '}')
                        break;
                    p++;
                }
            }
            if (p == strend)
                return false;
            re->err = s;
            // 获取对象值
            *re->name = '\0';
            if (NPJSON_ResolveValue(re, fun, s, p, obj) != p)
                return false;
            p++;
            SKIPBLANK;
        }
    } else
        return false;
    return true;
}

static bool _NPJSON_ResolveExev(NPJSON_Result *re, void *obj, NPJSON_ResolveFunc fun)
{
    if (fun == NPJSON_NULL || re == NPJSON_NULL)
        return 0;
    // 保存状态
    int idx    = re->ArrIdx;
    int lv     = re->level++;
    re->ArrIdx = 0;
    bool res   = NPJSON_ResolveExev(re, fun, obj, !re->isArray);   // 数组内容时要获取对象
    // 恢复状态
    re->ArrIdx = idx;
    re->level  = lv;
    return res;
}

bool NPJSON_Resolve(const char *str, int length, void *obj, NPJSON_ResolveFunc fun, const char **err)
{
    if (str == NPJSON_NULL || length <= 0 || fun == NPJSON_NULL)
        return false;
    if (err != NPJSON_NULL)
        *err = str;
    int         lv     = 1;
    const char *p      = str;
    const char *strend = str + length;
    while (p != strend && *p != '{')
        p++;
    if (p == strend)
        return false;
    const char *s = p;
    p++;
    while (p != strend && lv > 0) {
        if (*p == '{')
            lv++;
        else if (*p == '}')
            lv--;
        p++;
    }
    if (lv != 0)
        return false;
    NPJSON_Result re;
    memset(&re, 0, sizeof(re));
    re.name = NEW(char, NPJSON_NAME_LEN + 1);
    if (re.name == NPJSON_NULL)
        return 0;
    re.err       = s;
    re.str       = s;
    re.Strlength = p - s;
    re.ArrIdx    = 0;
    re.level     = 0;
    re.Resolve   = _NPJSON_ResolveExev;
    bool flag    = NPJSON_ResolveExev(&re, fun, obj, true);
    DELETE(re.name);
    if (err != NPJSON_NULL)
        *err = flag ? NPJSON_NULL : re.err;
    return flag;
}

static bool _NPJSON_ResolveExevObject(NPJSON_RObject *re, void *obj, NPJSON_ResolveFunc fun)
{
    if (re == NPJSON_NULL)
        return false;
    return NPJSON_Resolve(re->str, re->Strlength, obj, fun, NPJSON_NULL);
}

NPJSON_RObject NPJSON_CreateUnSafeObject(const NPJSON_Result *re)
{
    NPJSON_RObject obj;
    obj.Resolve   = _NPJSON_ResolveExevObject;
    obj.str       = NPJSON_NULL;
    obj.Strlength = 0;
    obj.isSafe    = false;
    if (re != NPJSON_NULL) {
        obj.str       = re->str;
        obj.Strlength = re->Strlength;
    }
    return obj;
}

NPJSON_RObject NPJSON_CreateSafeObject(const NPJSON_Result *re)
{
    NPJSON_RObject obj;
    obj.Resolve   = _NPJSON_ResolveExevObject;
    obj.str       = NPJSON_NULL;
    obj.Strlength = 0;
    obj.isSafe    = true;
    if (re != NPJSON_NULL && re->Strlength > 0 && re->str != NPJSON_NULL) {
        char *tmp = NEW(char, re->Strlength);
        if (tmp != NPJSON_NULL) {
            memcpy(tmp, re->str, re->Strlength);
            obj.str       = tmp;
            obj.Strlength = re->Strlength;
        }
    }
    return obj;
}

NPJSON_RObject NPJSON_RObjectUnSafeToSafe(NPJSON_RObject *re)
{
    NPJSON_RObject obj;
    obj.Resolve   = _NPJSON_ResolveExevObject;
    obj.str       = NPJSON_NULL;
    obj.Strlength = 0;
    obj.isSafe    = true;
    if (re != NPJSON_NULL && !re->isSafe && re->str != NPJSON_NULL && re->Strlength > 0) {
        const char *str    = re->str;
        const char *strend = str + re->Strlength;
        while (str != strend && *str != '{')
            str++;
        while (strend != str && *strend != '}')
            strend--;
        int length = strend - str + 1;
        if (length > 0) {
            char *tmp = NEW(char, length);
            if (tmp != NPJSON_NULL) {
                memcpy(tmp, str, length);
                obj.str       = tmp;
                obj.Strlength = re->Strlength;
            }
        }
    }
    return obj;
}

NPJSON_RObject NPJSON_ResolveToSafeObject(const char *str, int length)
{
    NPJSON_RObject obj;
    obj.Resolve   = _NPJSON_ResolveExevObject;
    obj.str       = NPJSON_NULL;
    obj.Strlength = 0;
    obj.isSafe    = true;
    if (str != NPJSON_NULL && length > 0) {
        char *tmp = NEW(char, length + 1);
        if (tmp != NPJSON_NULL) {
            memcpy(tmp, str, length);
            tmp[length]   = '\0';
            obj.str       = tmp;
            obj.Strlength = length;
        }
    }
    return obj;
}

void NPJSON_DeleteSafeObject(NPJSON_RObject *re)
{
    if (re == NPJSON_NULL || !re->isSafe || re->str == NPJSON_NULL)
        return;
    DELETE((void *)re->str);
    re->str       = NPJSON_NULL;
    re->Strlength = 0;
    return;
}

// ---------------------------------------------------------------------------------------------------------------------
//                                             | JSON 生成 |
// ---------------------------------------------------------------------------------------------------------------------

static bool _NPJSON_CheckCacheSize(NPJSON_Synthesizer *syn, int size)
{
#define _ALIGN(n, m) (n <= 0 ? 0 : ((n)-1) / (m) + 1)
    if (syn->endidx - syn->idx > size)
        return true;
    int len = syn->endidx - syn->str + size + 1;
    // 512 对齐
    len       = _ALIGN(len, 512) * 512;
    char *tmp = REDIM(char, syn->str, len);
    if (tmp == NPJSON_NULL)
        return false;
    syn->idx    = tmp + (syn->idx - syn->str);
    syn->str    = tmp;
    syn->endidx = syn->str + len;
    return true;
}

char *_NPJSON_SetName(char *idx, char *endidx, const char *name)
{
    *idx++ = '\"';
    while (idx != endidx && *name != '\0')
        *idx++ = *name++;
    *idx++ = '\"';
    *idx++ = ':';
    return idx;
}

static bool _NPJSON_StartObject(NPJSON_Synthesizer *re, const char *name)
{
    if (re == NPJSON_NULL)
        return false;
    int name_len = name == NPJSON_NULL ? 1 : strlen(name);
    if (!_NPJSON_CheckCacheSize(re, name_len + 5))
        return false;
    if (name != NPJSON_NULL)
        re->idx = _NPJSON_SetName(re->idx, re->endidx, name);
    *re->idx++ = '{';
    return true;
}

static bool _NPJSON_EndObject(NPJSON_Synthesizer *re)
{
    if (re == NPJSON_NULL)
        return false;
    if (!_NPJSON_CheckCacheSize(re, 3))
        return false;
    // 去除对象最后一个 ,
    char *idx = re->idx - 1;
    while (idx != re->str && *idx == ' ')
        idx--;
    if (*idx == ',')
        re->idx = idx;
    *re->idx++ = '}';
    *re->idx++ = ',';
    return true;
}

static bool _NPJSON_StartArray(NPJSON_Synthesizer *re, const char *name)
{
    if (re == NPJSON_NULL)
        return false;
    int name_len = name == NPJSON_NULL ? 1 : strlen(name);
    if (!_NPJSON_CheckCacheSize(re, name_len + 5))
        return false;
    if (name != NPJSON_NULL)
        re->idx = _NPJSON_SetName(re->idx, re->endidx, name);
    *re->idx++ = '[';
    return true;
}

static bool _NPJSON_EndArray(NPJSON_Synthesizer *re)
{
    if (re == NPJSON_NULL)
        return false;
    if (!_NPJSON_CheckCacheSize(re, 3))
        return false;
    // 去除对象最后一个 ,
    char *idx = re->idx - 1;
    while (idx != re->str && *idx == ' ')
        idx--;
    if (*idx == ',')
        re->idx = idx;
    *re->idx++ = ']';
    *re->idx++ = ',';
    return true;
}

static bool _NPJSON_AddInt(NPJSON_Synthesizer *re, const char *name, long long value)
{
    if (re == NPJSON_NULL || name == NPJSON_NULL)
        return false;
    int  name_len = strlen(name);
    char tmp[32];
    int  len = CM_snprintf(tmp, sizeof(tmp), "%lld", value);
    if (!_NPJSON_CheckCacheSize(re, name_len + len + 5))
        return false;
    re->idx += CM_snprintf(re->idx, re->endidx - re->idx, "\"%s\":%s,", name, tmp);
    return true;
}

static bool _NPJSON_AddNumber(NPJSON_Synthesizer *re, const char *name, double value)
{
    if (re == NPJSON_NULL || name == NPJSON_NULL)
        return false;
    int  name_len = strlen(name);
    char tmp[32];
    bool isval = (value * 0) == 0;
    int  len   = isval ? snprintf(tmp, sizeof(tmp), "%lg", value) : 4;
    if (!_NPJSON_CheckCacheSize(re, name_len + len + 5))
        return false;
    if (!isval)   // 检查NaN和Infinity
        re->idx += CM_snprintf(re->idx, re->endidx - re->idx, "\"%s\":null,", name);
    else
        re->idx += CM_snprintf(re->idx, re->endidx - re->idx, "\"%s\":%lg,", name, value);
    return true;
}

static bool _NPJSON_AddString(NPJSON_Synthesizer *re, const char *name, const char *value)
{
    if (re == NPJSON_NULL || name == NPJSON_NULL)
        return false;
    int name_len  = strlen(name);
    int value_len = value == NPJSON_NULL ? 4 : strlen(value);
    if (!_NPJSON_CheckCacheSize(re, name_len + value_len + 6))
        return false;
    if (value == NPJSON_NULL)
        re->idx += CM_snprintf(re->idx, re->endidx - re->idx, "\"%s\":null,", name);
    else
        re->idx += CM_snprintf(re->idx, re->endidx - re->idx, "\"%s\":\"%s\",", name, value);
    return true;
}

static bool _NPJSON_AddBool(NPJSON_Synthesizer *re, const char *name, bool value)
{
    if (re == NPJSON_NULL || name == NPJSON_NULL)
        return false;
    int         name_len   = strlen(name);
    const char *STR_BOOL[] = {"false", "true"};
    if (!_NPJSON_CheckCacheSize(re, name_len + strlen(STR_BOOL[value]) + 4))
        return false;
    re->idx += CM_snprintf(re->idx, re->endidx - re->idx, "\"%s\":%s,", name, STR_BOOL[value]);
    return true;
}

static bool _NPJSON_AddObject(NPJSON_Synthesizer *re, const char *name, const char *str, int Strlength)
{
    if (re == NPJSON_NULL || name == NPJSON_NULL)
        return false;
    if (Strlength <= 0)
        str = NPJSON_NULL;
    int name_len  = strlen(name);
    int value_len = str == NPJSON_NULL ? 4 : Strlength;
    if (!_NPJSON_CheckCacheSize(re, name_len + value_len + 4))   //"":,
        return false;
    if (str == NPJSON_NULL)
        re->idx += CM_snprintf(re->idx, re->endidx - re->idx, "\"%s\":null,", name);
    else {
        re->idx = _NPJSON_SetName(re->idx, re->endidx, name);
        memcpy(re->idx, str, Strlength);
        re->idx += Strlength;
        *re->idx++ = ',';
    }
    return true;
}

static bool _NPJSON_AddSObject(NPJSON_Synthesizer *re, const char *name, const NPJSON_SObject *obj)
{
    return _NPJSON_AddObject(re, name, obj == NPJSON_NULL ? NPJSON_NULL : obj->str, obj == NPJSON_NULL ? 0 : obj->Strlength);
}

static bool _NPJSON_AddRObject(NPJSON_Synthesizer *re, const char *name, const NPJSON_RObject *obj)
{
    return _NPJSON_AddObject(re, name, obj == NPJSON_NULL ? NPJSON_NULL : obj->str, obj == NPJSON_NULL ? 0 : obj->Strlength);
}

const _NPJSON_Synthesizer_AddEvent _AddEvent = {
    _NPJSON_AddInt,
    _NPJSON_AddNumber,
    _NPJSON_AddString,
    _NPJSON_AddBool,
    _NPJSON_AddSObject,
    _NPJSON_AddRObject};

static bool _NPJSON_AddItemInt(NPJSON_Synthesizer *re, long long value)
{
    if (re == NPJSON_NULL)
        return false;
    char tmp[32];
    int  len = CM_snprintf(tmp, sizeof(tmp), "%lld", value);
    if (!_NPJSON_CheckCacheSize(re, len + 1))
        return false;
    re->idx += CM_snprintf(re->idx, re->endidx - re->idx, "%s,", tmp);
    return true;
}

static bool _NPJSON_AddItemNumber(NPJSON_Synthesizer *re, double value)
{
    if (re == NPJSON_NULL)
        return false;
    char tmp[32];
    bool isval = (value * 0) == 0;
    int  len   = isval ? snprintf(tmp, sizeof(tmp), "%lg", value) : 4;
    if (!_NPJSON_CheckCacheSize(re, len + 6))
        return false;
    if (!isval)   // 检查NaN和Infinity
        re->idx += CM_snprintf(re->idx, re->endidx - re->idx, "null,");
    else
        re->idx += CM_snprintf(re->idx, re->endidx - re->idx, "%lg,", value);
    return true;
}

static bool _NPJSON_AddItemString(NPJSON_Synthesizer *re, const char *value)
{
    if (re == NPJSON_NULL)
        return false;
    int value_len = value == NPJSON_NULL ? 5 : strlen(value);
    if (!_NPJSON_CheckCacheSize(re, value_len + 3))
        return false;
    if (value == NPJSON_NULL)
        re->idx += CM_snprintf(re->idx, re->endidx - re->idx, "null,");
    else {
        int len = CM_snprintf(re->idx, re->endidx - re->idx, "\"%s\",", value);
        re->idx += len;
    }
    return true;
}

static bool _NPJSON_AddItemBool(NPJSON_Synthesizer *re, bool value)
{
    if (re == NPJSON_NULL)
        return false;
    const char *STR_BOOL[] = {"false", "true"};
    if (!_NPJSON_CheckCacheSize(re, strlen(STR_BOOL[value]) + 1))
        return false;
    re->idx += CM_snprintf(re->idx, re->endidx - re->idx, "%s,", STR_BOOL[value]);
    return true;
}

static bool _NPJSON_AddItemObject(NPJSON_Synthesizer *re, const char *str, int Strlength)
{
    if (re == NPJSON_NULL)
        return false;
    if (Strlength <= 0)
        str = NPJSON_NULL;
    int value_len = str == NPJSON_NULL ? 4 : Strlength;
    if (!_NPJSON_CheckCacheSize(re, value_len + 2))   //,
        return false;
    if (str == NPJSON_NULL)
        re->idx += CM_snprintf(re->idx, re->endidx - re->idx, "null,");
    else {
        memcpy(re->idx, str, Strlength);
        re->idx += Strlength;
        *re->idx++ = ',';
    }
    return true;
}

static bool _NPJSON_AddItemSObject(NPJSON_Synthesizer *re, const NPJSON_SObject *obj)
{
    return _NPJSON_AddItemObject(re, obj == NPJSON_NULL ? NPJSON_NULL : obj->str, obj == NPJSON_NULL ? 0 : obj->Strlength);
}

static bool _NPJSON_AddItemRObject(NPJSON_Synthesizer *re, const NPJSON_RObject *obj)
{
    return _NPJSON_AddItemObject(re, obj == NPJSON_NULL ? NPJSON_NULL : obj->str, obj == NPJSON_NULL ? 0 : obj->Strlength);
}

const _NPJSON_Synthesizer_AddArrayEvent _AddArrayEvent = {
    _NPJSON_AddItemInt,
    _NPJSON_AddItemNumber,
    _NPJSON_AddItemString,
    _NPJSON_AddItemBool,
    _NPJSON_AddItemSObject,
    _NPJSON_AddItemRObject};

NPJSON_Synthesizer NPJSON_CreateSynthesizer(int size)
{
    if (size < 32)
        size = 32;
    NPJSON_Synthesizer syn;
    syn.idx    = NPJSON_NULL;
    syn.endidx = NPJSON_NULL;
    syn.str    = NEW(char, size + 1);
    if (syn.str != NPJSON_NULL) {
        syn.idx    = syn.str;
        syn.endidx = syn.str + size;
        *syn.idx++ = '{';
    }
    syn.EndObject   = _NPJSON_EndObject;
    syn.StartObject = _NPJSON_StartObject;
    syn.StartArray  = _NPJSON_StartArray;
    syn.EndArray    = _NPJSON_EndArray;

    syn.Add          = &_AddEvent;
    syn.AddArrayItem = &_AddArrayEvent;
    return syn;
}

NPJSON_Synthesizer NPJSON_SynthesizerClone(const NPJSON_Synthesizer *sn)
{
    if (sn == NPJSON_NULL || sn->str == NPJSON_NULL)
        return NPJSON_CreateSynthesizer(0);
    NPJSON_Synthesizer re = *sn;
    re.idx = re.endidx = NPJSON_NULL;
    int len            = sn->endidx - sn->str;
    re.str             = NEW(char, len + 1);
    if (re.str != NPJSON_NULL) {
        memcpy(re.str, sn->str, len);
        re.idx    = re.str + (sn->idx - sn->str);
        re.endidx = re.str + len;
    }
    return re;
}

void NPJSON_DeleteSynthesizer(NPJSON_Synthesizer *re)
{
    if (re == NPJSON_NULL || re->str == NPJSON_NULL)
        return;
    DELETE(re->str);
    re->str = re->idx = re->endidx = NPJSON_NULL;
    return;
}

NPJSON_SObject NPJSON_CreateSObject(NPJSON_Synthesizer *re)
{
    NPJSON_SObject sobj;
    sobj.str       = NPJSON_NULL;
    sobj.Strlength = 0;
    if (re != NPJSON_NULL && re->str != NPJSON_NULL && _NPJSON_CheckCacheSize(re, 2)) {
        // 去除对象最后一个 ,
        char *idx = re->idx - 1;
        while (idx != re->str && *idx == ' ')
            idx--;
        if (*idx == ',')
            re->idx = idx;
        *re->idx++     = '}';
        *re->idx       = '\0';
        sobj.str       = re->str;
        sobj.Strlength = re->idx - re->str;
        re->endidx = re->str = re->idx = NPJSON_NULL;
    } else
        NPJSON_DeleteSynthesizer(re);
    return sobj;
}

void NPJSON_DeleteSObject(NPJSON_SObject *re)
{
    if (re == NPJSON_NULL || re->str == NPJSON_NULL)
        return;
    DELETE(re->str);
    re->Strlength = 0;
    re->str       = NPJSON_NULL;
    return;
}

// ---------------------------------------------------------------------------------------------------------------------
//                                             | 对象转换 |
// ---------------------------------------------------------------------------------------------------------------------

NPJSON_RObject NPJSON_SObjectToSafeRObject(NPJSON_SObject *re)
{
    NPJSON_RObject r;
    r.isSafe  = true;
    r.Resolve = _NPJSON_ResolveExevObject;
    if (re != NPJSON_NULL && re->str != NPJSON_NULL && re->Strlength > 0) {
        r.str         = re->str;
        r.Strlength   = re->Strlength;
        re->str       = NPJSON_NULL;
        re->Strlength = 0;
    }
    return r;
}

NPJSON_SObject NPJSON_SafeRObjectToSObject(NPJSON_RObject *re)
{
    NPJSON_SObject s;
    s.str       = NPJSON_NULL;
    s.Strlength = 0;
    if (re != NPJSON_NULL && re->str != NPJSON_NULL && re->isSafe && re->Strlength > 0) {
        s.str         = (char *)re->str;   // 安全对象，可以强制转换
        s.Strlength   = re->Strlength;
        re->str       = NPJSON_NULL;
        re->Strlength = 0;
    }
    return s;
}

NPJSON_RObject NPJSON_SafeRObjectClone(const NPJSON_RObject *re)
{
    NPJSON_RObject r;
    r.isSafe  = true;
    r.Resolve = _NPJSON_ResolveExevObject;
    if (re != NPJSON_NULL && re->str != NPJSON_NULL && re->Strlength > 0) {
        char *tmp = NEW(char, re->Strlength);
        if (tmp != NPJSON_NULL) {
            memcpy(tmp, re->str, re->Strlength);
            r.str       = tmp;
            r.Strlength = re->Strlength;
        }
    }
    return r;
}

NPJSON_SObject NPJSON_SObjectClone(const NPJSON_SObject *re)
{
    NPJSON_SObject s;
    s.str       = NPJSON_NULL;
    s.Strlength = 0;
    if (re != NPJSON_NULL && re->str != NPJSON_NULL && re->Strlength > 0) {
        char *tmp = NEW(char, re->Strlength);
        if (tmp != NPJSON_NULL) {
            memcpy(tmp, re->str, re->Strlength);
            s.str       = tmp;
            s.Strlength = re->Strlength;
        }
    }
    return s;
}

static void NPJSON_Builder_fix(NPJSONNode *n)
{
    if (n == NPJSON_NULL || n->isObject == 0)
        return;
    // 再次尾插
    NPJSONNode *t = n->Val.Object;
    NPJSONNode *p = NPJSON_NULL;
    while (t != NPJSON_NULL) {
        NPJSONNode *r = t;
        t             = t->next;
        r->next       = p;
        p             = r;
    }
    n->Val.Object = p;
    return;
}

static bool NPJSON_Builder_func(const char *name, NPJSON_Result *re, int index, void *obj)
{
    NPJSONNode **n = (NPJSONNode **)obj;
    if ((*n)->isObject == 0)
        return false;
    NPJSONNode *tmp = NEW(NPJSONNode, 1);
    if (tmp == NPJSON_NULL)
        return false;
    memset(tmp, 0, sizeof(NPJSONNode));
    if (name != NPJSON_NULL && *name != '\0') {
        int name_len = strlen(name);
        tmp->name    = NEW(char, name_len + 1);
        if (tmp->name == NPJSON_NULL) {
            DELETE(tmp);
            return false;
        }
        memcpy(tmp->name, name, name_len + 1);
    }

    // 尾插法
    tmp->next        = (*n)->Val.Object;
    (*n)->Val.Object = tmp;
    (*n)->ChildCount++;

    // 设置值
    tmp->isBinValue   = re->isBinValue;
    tmp->isNull       = re->isNull;
    tmp->isNumber     = re->isNumber;
    tmp->isObject     = re->isObject;
    tmp->isString     = re->isString;
    tmp->isArray      = re->isArray;
    tmp->isInteger    = re->isInteger;
    tmp->Val.BinValue = re->Val.BinValue;
    tmp->Val.Number   = re->Val.Number;
    tmp->Val.Value    = re->Val.Value;

    tmp->level  = re->level;
    tmp->ArrIdx = index;

    if (tmp->isString) {
        char *str = NEW(char, (re->Val.String.length + 1));
        if (str != NPJSON_NULL) {
            memcpy(str, re->Val.String.ptr, re->Val.String.length);
            str[re->Val.String.length] = '\0';
            tmp->Val.String            = str;
        }
    } else if (tmp->isObject) {
        bool rs = re->Resolve(re, &tmp, NPJSON_Builder_func);
        NPJSON_Builder_fix(tmp);   // 顺序修正
        return rs;
    }
    return true;
}

NPJSONNode *NPJSON_Builder(const char *str, size_t len, const char **err)
{
    if (str == NPJSON_NULL || len <= 0)
        return NPJSON_NULL;
    NPJSONNode *n = NEW(NPJSONNode, 1);
    if (n == NPJSON_NULL)
        return NPJSON_NULL;
    memset(n, 0, sizeof(NPJSONNode));
    n->isObject = 1;
    n->level    = 0;
    bool re     = NPJSON.Parsing.Resolve(str, len, &n, NPJSON_Builder_func, err);
    if (re) {
        NPJSON_Builder_fix(n);   // 顺序修正
        return n;
    } else {
        NPJSON_Release(&n);
        return NPJSON_NULL;
    }
}

void NPJSON_Release(NPJSONNode **n)
{
    if (n == NPJSON_NULL || *n == NPJSON_NULL)
        return;
    NPJSONNode *p = (*n);
    while (p != NPJSON_NULL) {
        NPJSONNode *t = p;
        p             = p->next;
        if (t->Val.String != NPJSON_NULL)
            DELETE(t->Val.String);
        if (t->name != NPJSON_NULL)
            DELETE(t->name);
        if (t->isObject && t->Val.Object != NPJSON_NULL)
            NPJSON_Release(&(t->Val.Object));
        DELETE(t);
    }
    *n = NPJSON_NULL;
    return;
}

NPJSONNode *NPJSON_Find(NPJSONNode *n, const char *name)
{
    if (name == NPJSON_NULL || *name == '\0' || n == NPJSON_NULL || n->isObject == 0)
        return NPJSON_NULL;
    n = n->Val.Object;
    while (n != NPJSON_NULL) {
        NPJSONNode *t = n;
        n             = n->next;
        if (strcmp(t->name, name) == 0)
            return t;
    }
    return NPJSON_NULL;
}

size_t NPJSON_GetChildCount(NPJSONNode *n)
{
    if (n == NPJSON_NULL)
        return 0;
    return n->ChildCount;
}

NPJSONNode *NPJSON_GetNext(NPJSONNode *n)
{
    return n == NPJSON_NULL ? NPJSON_NULL : n->next;
}
