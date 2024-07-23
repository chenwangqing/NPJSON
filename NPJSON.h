/**
 * @file     NPJSON.h
 * @brief    JSON解析、生成
 * @author   CXS (chenxiangshu@outlook.com)
 * @version  1.12
 * @date     2022-07-18
 *
 * @copyright Copyright (c) 2024  chenxiangshu@outlook.com
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * @par 修改日志:
 * <table>
 * <tr><th>日期       <th>版本    <th>作者    <th>说明
 * <tr><td>2020-05-30 <td>1.0     <td>CXS    <td>添加结构体对象JSON_t;修正在科学计数法在整型值获取不正确的错误；
 * <tr><td>2020-11-06 <td>1.1     <td>CXS    <td>修正在C语言环境中的一下错误
 * <tr><td>2021-08-05 <td>1.2     <td>CXS    <td>添加嵌套数组生成
 * <tr><td>2021-09-04 <td>1.3     <td>CXS    <td>添加NPJSON_Builder
 * <tr><td>2021-11-23 <td>1.4     <td>CXS    <td>修正_NPJSON_CheckCacheSize分配计算错误
 * <tr><td>2021-12-22 <td>1.5     <td>CXS    <td>修正NPJSON_Builder顺序错误;区分整数和小数
 * <tr><td>2022-02-11 <td>1.6     <td>CXS    <td>修正NPJSON_ResolveValue类型判断初始化不完善错误
 * <tr><td>2022-03-01 <td>1.7     <td>CXS    <td>修正对象数组解析不正确
 * <tr><td>2022-03-25 <td>1.8     <td>CXS    <td>加载异常处理，提供异常信息返回
 * <tr><td>2022-06-09 <td>1.9     <td>CXS    <td>修正解析字符串值遇到括号就不能解析的错误
 * <tr><td>2023-03-14 <td>1.10    <td>CXS    <td>修正解析遇到结束}就返回错误
 * <tr><td>2023-03-24 <td>1.11    <td>CXS    <td>添加接口NPJSON_SetInter
 * <tr><td>2024-07-23 <td>1.12    <td>CXS    <td>完善宏处理
 * </table>

功能说明：
1.支持JSON对象、数组解析
2.不支持注释
3.不支持错误收集
4.支持JSON对象、数组生成
5.内存使用极低 需要的内存 = NPJSON_NAME_LEN + JSON对象深度(涉及递归)
6.不支持'\uXXXX'扩展字符

注意：
1.NPJSON_Synthesizer 转换成 NPJSON_SObject 后会删除 NPJSON_Synthesizer
内容。如果想重复使用需要 NPJSON_CreateSynthesizer 或者
提前NPJSON_SynthesizerClone

*/

#if !defined(__CXSMETHOD_NPJSON_H__)
#define __CXSMETHOD_NPJSON_H__
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CFG_NPJSON_NAME_LEN
#define NPJSON_NAME_LEN 64   // 名字长度
#else
#define NPJSON_NAME_LEN CFG_NPJSON_NAME_LEN
#endif

// 外部接口
extern void *NPJSON_Malloc(size_t size);
extern void  NPJSON_Free(void *ptr);
extern void *NPJSON_Realloc(void *ptr, size_t size);

// JSON 解析结果
typedef struct _NPJSON_Result NPJSON_Result;

// JSON 合成器
typedef struct _NPJSON_Synthesizer NPJSON_Synthesizer;

// JSON 解析对象
typedef struct _NPJSON_RObject NPJSON_RObject;

// JSON 合成对象
typedef struct _NPJSON_SObject NPJSON_SObject;

// PARS：name 对象名称
// PARS：re 解析结果
// PARS：index 数组索引
// PARS：obj 存储对象
// RETV：false 停止解析
typedef bool (*NPJSON_ResolveFunc)(const char *name, NPJSON_Result *re, int index, void *obj);

// JSON 解析对象
struct _NPJSON_RObject
{
    bool        isSafe;      // 是否为安全对象
    const char *str;         // 要解析的字符串
    int         Strlength;   // 要解析的字符串长度

    // 对象深度解析
    bool (*Resolve)(NPJSON_RObject *re, void *obj, NPJSON_ResolveFunc fun);
};

// JSON 解析结果
struct _NPJSON_Result
{
    const char *str;         // 要解析的字符串
    int         Strlength;   // 要解析的字符串长度
    const char *err;         // 发生错误字符串

    int ArrIdx;   // 数组索引
    int level;    // 层级

    // 解析值
    struct
    {
        bool BinValue;   // 二值量
        struct
        {
            const char *ptr;
            int         length;
        } String;           // 字符串
        long long Value;    // 整型值
        double    Number;   // 数值量
    } Val;

    char *name;   // 对象名称，数组为null

    uint8_t isObject : 1;     // 是否为对象（包含数组）
    uint8_t isArray : 1;      // 是否为数组
    uint8_t isBinValue : 1;   // 是否为二值量
    uint8_t isNumber : 1;     // 是否为数值量包含整数
    uint8_t isInteger : 1;    // 整型
    uint8_t isString : 1;     // 是否为字符串
    uint8_t isNull : 1;       // 是否空值

    // 对象深度解析
    bool (*Resolve)(NPJSON_Result *re, void *obj, NPJSON_ResolveFunc fun);
};

typedef struct
{
    bool (*Int)(NPJSON_Synthesizer *re, const char *name, long long value);
    bool (*Number)(NPJSON_Synthesizer *re, const char *name, double value);
    bool (*String)(NPJSON_Synthesizer *re, const char *name, const char *value);
    bool (*Bool)(NPJSON_Synthesizer *re, const char *name, bool value);
    bool (*SObject)(NPJSON_Synthesizer *re, const char *name, const NPJSON_SObject *value);
    bool (*RObject)(NPJSON_Synthesizer *re, const char *name, const NPJSON_RObject *value);
} _NPJSON_Synthesizer_AddEvent;

typedef struct
{
    bool (*Int)(NPJSON_Synthesizer *re, long long value);
    bool (*Number)(NPJSON_Synthesizer *re, double value);
    bool (*String)(NPJSON_Synthesizer *re, const char *value);
    bool (*Bool)(NPJSON_Synthesizer *re, bool value);
    bool (*SObject)(NPJSON_Synthesizer *re, const NPJSON_SObject *value);
    bool (*RObject)(NPJSON_Synthesizer *re, const NPJSON_RObject *value);
} _NPJSON_Synthesizer_AddArrayEvent;

struct _NPJSON_Synthesizer
{
    char *str;
    char *idx;
    char *endidx;

    // FUNC：StartObject
    // PARS：name 对象名称 null：嵌套对象用于数组中
    // NOTE：创建对象
    // DATE：2020年5月23日
    // RETV：false 创建失败
    bool (*StartObject)(NPJSON_Synthesizer *re, const char *name);
    // FUNC：StartObject
    // PARS：name 对象名称
    // NOTE：结束对象
    // DATE：2020年5月23日
    // RETV：false 结束失败
    bool (*EndObject)(NPJSON_Synthesizer *re);
    // FUNC：StartObject
    // PARS：name 对象名称 null：嵌套数组
    // NOTE：创建数组
    // DATE：2020年5月23日
    // RETV：false 创建失败
    bool (*StartArray)(NPJSON_Synthesizer *re, const char *name);
    // FUNC：StartObject
    // PARS：name 对象名称
    // NOTE：结束数组
    // DATE：2020年5月23日
    // RETV：false 结束失败
    bool (*EndArray)(NPJSON_Synthesizer *re);

    const _NPJSON_Synthesizer_AddEvent *     Add;            // 添加元素
    const _NPJSON_Synthesizer_AddArrayEvent *AddArrayItem;   // 添加数组元素
};

struct _NPJSON_SObject
{
    char *str;
    int   Strlength;
};

// ---------------------------------------------------------------------------------------------------------------------
//                                             | JSON 解析 |
// ---------------------------------------------------------------------------------------------------------------------

// FUNC：NPJSON_Resolve
// PARS：str 解析的字符串
// PARS：length 解析的字符串 长度
// PARS：fun 解析回调
// PARS：obj 存储对象
// PARS：err 发生错误的字符串（指向str内容的指针）
// NOTE：JSON解析
// DATE：2020年5月21日
// RETV：true 解析成功 false 解析失败
extern bool NPJSON_Resolve(const char *str, int length, void *obj, NPJSON_ResolveFunc fun, const char **err);

// FUNC：NPJSON_CreateObject
// PARS：re JSON解析结果
// NOTE：创建非安全解析对象 在整个解析过程中确保 NPJSON_Resolve
// 传入的字符串不会改变下使用 DATE：2020年5月21日
extern NPJSON_RObject NPJSON_CreateUnSafeObject(const NPJSON_Result *re);

// FUNC：NPJSON_CreateObject
// PARS：re JSON解析结果
// NOTE：创建安全解析对象 使用后必须调用 NPJSON_DeleteSafeObject 否则内存泄漏
// DATE：2020年5月21日
extern NPJSON_RObject NPJSON_CreateSafeObject(const NPJSON_Result *re);

// FUNC：NPJSON_RObjectUnSafeToSafe
// PARS：re 非安全解析对象
// NOTE：将非安全解析对象 转换成安全的 使用后必须调用 NPJSON_DeleteSafeObject
// 否则内存泄漏 DATE：2020年5月21日
extern NPJSON_RObject NPJSON_RObjectUnSafeToSafe(NPJSON_RObject *re);

// FUNC：NPJSON_ResolveToSafeObject
// PARS：str 解析的字符串
// PARS：length 解析的字符串 长度
// NOTE：解析字符串转解析对象（安全） 使用后必须调用 NPJSON_DeleteSafeObject
// 否则内存泄漏 DATE：2020年5月22日
extern NPJSON_RObject NPJSON_ResolveToSafeObject(const char *str, int length);

// FUNC：NPJSON_DeleteSafeObject
// PARS：re 安全解析对象
// NOTE：删除安全解析对象
// DATE：2020年5月21日
extern void NPJSON_DeleteSafeObject(NPJSON_RObject *re);

#define NAME_IS(NAME) strcmp(name, #NAME) == 0

// ---------------------------------------------------------------------------------------------------------------------
//                                             | JSON 生成 |
// ---------------------------------------------------------------------------------------------------------------------

// FUNC：NPJSON_CreateSynthesizer
// PARS：size 设置初始缓存大小
// NOTE：创建JSON合成器
// DATE：2020年5月23日
extern NPJSON_Synthesizer NPJSON_CreateSynthesizer(int size);

// FUNC：NPJSON_SynthesizerClone
// PARS：sn JSON合成器
// NOTE：JSON合成器克隆
// DATE：2020年5月23日
extern NPJSON_Synthesizer NPJSON_SynthesizerClone(const NPJSON_Synthesizer *sn);

// FUNC：NPJSON_DeleteSynthesizer
// PARS：sn JSON合成器
// NOTE：删除JSON合成器
// DATE：2020年5月23日
extern void NPJSON_DeleteSynthesizer(NPJSON_Synthesizer *re);

// FUNC：NPJSON_CreateSObject
// PARS：re JSON合成器
// NOTE：创建JSON合成对象，并删除JSON合成器 使用后必须 NPJSON_DeleteSObject
// 否则内存泄漏 DATE：2020年5月23日
extern NPJSON_SObject NPJSON_CreateSObject(NPJSON_Synthesizer *re);

// FUNC：NPJSON_DeleteSObject
// PARS：re JSON合成对象
// NOTE：删除JSON合成对象
// DATE：2020年5月23日
extern void NPJSON_DeleteSObject(NPJSON_SObject *re);

// FUNC：NPJSON_SObject_String
// PARS：obj JSON合成对象
// NOTE：获取JSON生成的字符串
// DATE：2020年5月23日
#define NPJSON_SObject_GetString(obj) (obj).str

// ---------------------------------------------------------------------------------------------------------------------
//                                             | 对象转换 |
// ---------------------------------------------------------------------------------------------------------------------

// FUNC：NPJSON_SObjectToSafeRObject
// PARS：re JSON合成对象
// NOTE：安全解析对象转换JSON合成对象 并删除安全解析对象 使用后必须
// NPJSON_DeleteSObject 否则内存泄漏 DATE：2020年5月23日
extern NPJSON_RObject NPJSON_SObjectToSafeRObject(NPJSON_SObject *re);

// FUNC：NPJSON_SafeRObjectToSObject
// PARS：re 安全解析对象
// NOTE：JSON合成对象转安全解析对象 并删除JSON合成对 使用后必须调用
// NPJSON_DeleteSafeObject 否则内存泄漏 DATE：2020年5月23日
extern NPJSON_SObject NPJSON_SafeRObjectToSObject(NPJSON_RObject *re);

// FUNC：NPJSON_SafeRObjectClone
// PARS：re 安全解析对象
// NOTE：对象克隆
// DATE：2020年5月23日
extern NPJSON_RObject NPJSON_SafeRObjectClone(const NPJSON_RObject *re);

// FUNC：NPJSON_SObjectClone
// PARS：re JSON合成对象
// NOTE：对象克隆
// DATE：2020年5月23日
extern NPJSON_SObject NPJSON_SObjectClone(const NPJSON_SObject *re);

typedef struct
{
    // 解析
    struct
    {
        // FUNC：NPJSON_Resolve
        // PARS：str 解析的字符串
        // PARS：length 解析的字符串 长度
        // PARS：fun 解析回调
        // PARS：obj 存储对象
        // NOTE：JSON解析
        // DATE：2020年5月21日
        // RETV：true 解析成功 false 解析失败
        bool (*Resolve)(const char *str, int length, void *obj, NPJSON_ResolveFunc fun, const char **err);

        // FUNC：NPJSON_CreateObject
        // PARS：re JSON解析结果
        // NOTE：创建非安全解析对象 在整个解析过程中确保 NPJSON_Resolve
        // 传入的字符串不会改变下使用 DATE：2020年5月21日
        NPJSON_RObject (*CreateUnSafeObject)(const NPJSON_Result *re);

        // FUNC：NPJSON_CreateObject
        // PARS：re JSON解析结果
        // NOTE：创建安全解析对象 使用后必须调用 NPJSON_DeleteSafeObject
        // 否则内存泄漏 DATE：2020年5月21日
        NPJSON_RObject (*CreateSafeObject)(const NPJSON_Result *re);

        // FUNC：NPJSON_RObjectUnSafeToSafe
        // PARS：re 非安全解析对象
        // NOTE：将非安全解析对象 转换成安全的 使用后必须调用
        // NPJSON_DeleteSafeObject 否则内存泄漏 DATE：2020年5月21日
        NPJSON_RObject (*RObjectUnSafeToSafe)(NPJSON_RObject *re);

        // FUNC：NPJSON_ResolveToSafeObject
        // PARS：str 解析的字符串
        // PARS：length 解析的字符串 长度
        // NOTE：解析字符串转解析对象（安全） 使用后必须调用 NPJSON_DeleteSafeObject
        // 否则内存泄漏 DATE：2020年5月22日
        NPJSON_RObject (*ResolveToSafeObject)(const char *str, int length);

        // FUNC：NPJSON_DeleteSafeObject
        // PARS：re 安全解析对象
        // NOTE：删除安全解析对象
        // DATE：2020年5月21日
        void (*DeleteSafeObject)(NPJSON_RObject *re);
    } Parsing;
    // 序列化
    struct
    {
        // FUNC：NPJSON_CreateSynthesizer
        // PARS：size 设置初始缓存大小
        // NOTE：创建JSON合成器
        // DATE：2020年5月23日
        NPJSON_Synthesizer (*CreateSynthesizer)(int size);

        // FUNC：NPJSON_SynthesizerClone
        // PARS：sn JSON合成器
        // NOTE：JSON合成器克隆
        // DATE：2020年5月23日
        NPJSON_Synthesizer (*SynthesizerClone)(const NPJSON_Synthesizer *sn);

        // FUNC：NPJSON_DeleteSynthesizer
        // PARS：sn JSON合成器
        // NOTE：删除JSON合成器
        // DATE：2020年5月23日
        void (*DeleteSynthesizer)(NPJSON_Synthesizer *re);

        // FUNC：NPJSON_CreateSObject
        // PARS：re JSON合成器
        // NOTE：创建JSON合成对象，并删除JSON合成器 使用后必须 NPJSON_DeleteSObject
        // 否则内存泄漏 DATE：2020年5月23日
        NPJSON_SObject (*CreateSObject)(NPJSON_Synthesizer *re);

        // FUNC：NPJSON_DeleteSObject
        // PARS：re JSON合成对象
        // NOTE：删除JSON合成对象
        // DATE：2020年5月23日
        void (*DeleteSObject)(NPJSON_SObject *re);
    } Serialization;
    // 对象转换
    struct
    {
        // FUNC：NPJSON_SObjectToSafeRObject
        // PARS：re JSON合成对象
        // NOTE：安全解析对象转换JSON合成对象 并删除安全解析对象 使用后必须
        // NPJSON_DeleteSObject 否则内存泄漏 DATE：2020年5月23日
        NPJSON_RObject (*SObjectToSafeRObject)(NPJSON_SObject *re);

        // FUNC：NPJSON_SafeRObjectToSObject
        // PARS：re 安全解析对象
        // NOTE：JSON合成对象转安全解析对象 并删除JSON合成对 使用后必须调用
        // NPJSON_DeleteSafeObject 否则内存泄漏 DATE：2020年5月23日
        NPJSON_SObject (*SafeRObjectToSObject)(NPJSON_RObject *re);

        // FUNC：NPJSON_SafeRObjectClone
        // PARS：re 安全解析对象
        // NOTE：对象克隆
        // DATE：2020年5月23日
        NPJSON_RObject (*SafeRObjectClone)(const NPJSON_RObject *re);

        // FUNC：NPJSON_SObjectClone
        // PARS：re JSON合成对象
        // NOTE：对象克隆
        // DATE：2020年5月23日
        NPJSON_SObject (*SObjectClone)(const NPJSON_SObject *re);
    } Object;
} NPJSON_t;

/**
 * @brief    导入NPJSON实现
 * @return   * const NPJSON_t*
 * @author   CXS (chenxiangshu@outlook.com)
 * @date     2024-07-23
 */
extern const NPJSON_t *NPJSON_Import(void);

typedef struct _NPJSONNode
{
    uint8_t isObject : 1;     // 是否为对象（包含数组）
    uint8_t isArray : 1;      // 是否为数组
    uint8_t isBinValue : 1;   // 是否为二值量
    uint8_t isNumber : 1;     // 是否为数值量包含整数
    uint8_t isInteger : 1;    // 整型
    uint8_t isString : 1;     // 是否为字符串
    uint8_t isNull : 1;       // 是否空值

    union
    {
        bool  BinValue;   // 二值量
        char *String;     // 字符串
        struct
        {
            long long Value;    // 整型值
            double    Number;   // 数值量
        };
        struct
        {
            struct _NPJSONNode *Object;       // 对象值
            size_t              ChildCount;   // 子对象的数量
        };
    } Val;

    struct _NPJSONNode *next;     // 后驱指针
    char *              name;     // null 表示数组内容
    int                 ArrIdx;   // 数组索引
    int                 level;    // 层级
} NPJSONNode;

// FUNC：NPJSON_Builder
// PARS：str JSON 字符串
// PARS：len JSON 字符串长度
// PARS：err 发生错误的字符串
// NOTE：生成
// DATE：2021年9月23日
extern NPJSONNode *NPJSON_Builder(const char *str, size_t len, const char **err);

// FUNC：NPJSON_Release
// PARS：n JSON 生成对象
// NOTE：释放
// DATE：2021年9月23日
extern void NPJSON_Release(NPJSONNode **n);

// FUNC：NPJSON_Find
// PARS：n JSON 生成对象
// PARS：name 名称
// NOTE：查找
// DATE：2021年9月23日
extern NPJSONNode *NPJSON_Find(NPJSONNode *n, const char *name);

// FUNC：NPJSON_GetChildCount
// PARS：n JSON 生成对象
// NOTE：获取子节点数量
// DATE：2021年9月23日
extern size_t NPJSON_GetChildCount(NPJSONNode *n);

// FUNC：NPJSON_GetNext
// PARS：n JSON 生成对象
// NOTE：获取下一个对象
// DATE：2021年9月23日
extern NPJSONNode *NPJSON_GetNext(NPJSONNode *n);

// ----------------------------------------------------------------------------------------------------
//                                          | 序列化宏  |
// ----------------------------------------------------------------------------------------------------

/**
 * @brief    序列化开始
 * @param    space   初始空间大小
 * @author   CXS (chenxiangshu@outlook.com)
 * @date     2023-09-19
 */
#define NPJSON_Serialization_Begin(space)                            \
    do {                                                             \
        NPJSON_Synthesizer __sn =                                    \
            NPJSON_Import()->Serialization.CreateSynthesizer(space); \
        NPJSON_SObject __s_obj = {0, 0};

/**
 * @brief    序列化结束
 * @author   CXS (chenxiangshu@outlook.com)
 * @date     2023-09-19
 */
#define NPJSON_Serialization_End()                           \
    NPJSON_Import()->Serialization.DeleteSObject(&__s_obj);  \
    NPJSON_Import()->Serialization.DeleteSynthesizer(&__sn); \
    }                                                        \
    while (false)

#define NPJSON_Object_SetInt(key, value)    __sn.Add->Int(&__sn, #key, value)
#define NPJSON_Object_SetNumber(key, value) __sn.Add->Number(&__sn, #key, value)
#define NPJSON_Object_SetString(key, value) __sn.Add->String(&__sn, #key, value)
#define NPJSON_Object_SetBool(key, value)   __sn.Add->Bool(&__sn, #key, value)
#define NPJSON_Object_SetNull(key, value)   __sn.Add->String(&__sn, #key, NULL)
#define NPJSON_Object_Start(key)            __sn.StartObject(&__sn, #key)
#define NPJSON_Object_End()                 __sn.EndObject(&__sn)
#define NPJSON_Array_Start(key)             __sn.StartArray(&__sn, #key)
#define NPJSON_Array_End()                  __sn.EndArray(&__sn)
#define NPJSON_Array_AddInt(value)          __sn.AddArrayItem->Int(&__sn, value)
#define NPJSON_Array_AddNumber(value)       __sn.AddArrayItem->Number(&__sn, value)
#define NPJSON_Array_AddString(value)       __sn.AddArrayItem->String(&__sn, value)
#define NPJSON_Array_AddBool(value)         __sn.AddArrayItem->Bool(&__sn, value)

/**
 * @brief    序列化完成
 * @author   CXS (chenxiangshu@outlook.com)
 * @date     2023-09-19
 */
#define NPJSON_Serialization_Complete() \
    __s_obj = NPJSON_Import()->Serialization.CreateSObject(&__sn);

/**
 * @brief    获取JSON字符串
 * @author   CXS (chenxiangshu@outlook.com)
 * @date     2023-09-19
 */
#define NPJSON_Serialization_GetJsonString() (__s_obj.str)

/**
 * @brief    获取JSON字符串长度
 * @author   CXS (chenxiangshu@outlook.com)
 * @date     2023-09-19
 */
#define NPJSON_Serialization_GetJsonStringLength() \
    (__s_obj.str ? __s_obj.Strlength : __sn.idx - __sn.str)

/*
    NPJSON_Serialization_Begin(512);
    NPJSON_Object_SetInt(a, 123);
    NPJSON_Object_SetString(str, "hello");
    NPJSON_Object_Start(info);
    NPJSON_Object_SetInt(a, 123);
    NPJSON_Object_SetString(str, "hello");
    NPJSON_Object_End();
    NPJSON_Array_Start(list);
    NPJSON_Array_AddInt(1);
    NPJSON_Array_AddInt(2);
    NPJSON_Array_AddInt(3);
    NPJSON_Array_AddInt(4);
    NPJSON_Array_End();
    NPJSON_Array_Start(objs);
    NPJSON_Object_Start(NULL);
    NPJSON_Object_SetInt(a, 123);
    NPJSON_Object_SetString(str, "hello");
    NPJSON_Object_End();
    NPJSON_Object_Start(NULL);
    NPJSON_Object_SetInt(a, 123);
    NPJSON_Object_SetString(str, "hello");
    NPJSON_Object_End();
    NPJSON_Array_End();
    NPJSON_Serialization_Complete();
    LOG_DEBUG("%s", NPJSON_Serialization_GetJsonString());
    NPJSON_Serialization_End();
*/

// ----------------------------------------------------------------------------------------------------
//                                          | 反序列化宏  |
// ----------------------------------------------------------------------------------------------------

#define __npjson_find(name)             \
    if (__is_err)                       \
        break;                          \
    __ptr = NPJSON_Find(__obj, #name);  \
    if (__ptr == NULL) {                \
        *__err   = "not found: " #name; \
        __is_err = true;                \
        break;                          \
    }

#define __npjson_check_object_type(name, check, type)         \
    if (__is_err)                                             \
        break;                                                \
    if (!(check)) {                                           \
        if (__err != NULL)                                    \
            *__err = "Type error: name=" #name " Type=" type; \
        __is_err = true;                                      \
        break;                                                \
    }

#define __npjson_check_array_type(check, type) \
    if (__is_err)                              \
        break;                                 \
    if (!(check)) {                            \
        if (__err != NULL)                     \
            *__err = "Type error: " type;      \
        __is_err = true;                       \
        break;                                 \
    }

#define __npjson_object_get(name, check, type, field, ret) \
    __npjson_find(name);                                   \
    __npjson_check_object_type(name, check, type);         \
    (ret) = __ptr->Val.field

#define __npjson_array_get(check, type, field, ret) \
    if (__ptr == NULL || __is_err)                  \
        break;                                      \
    __npjson_check_array_type(check, type);         \
    (ret) = __ptr->Val.field;                       \
    __ptr = __ptr->next

/**
 * @brief    开始解析JSON
 * @author   CXS (chenxiangshu@outlook.com)
 * @date     2024-07-23
 */
#define NPJSON_Builder_Begin(str, len, msg)                   \
    do {                                                      \
        msg                 = NULL;                           \
        const char **__err  = &msg;                           \
        NPJSONNode * __root = NPJSON_Builder(str, len, &msg); \
        if (__root == NULL || msg != NULL)                    \
            break;                                            \
        NPJSONNode *__obj    = __root;                        \
        NPJSONNode *__ptr    = __obj->Val.Object;             \
        NPJSONNode *__tmp    = NULL;                          \
        bool        __is_err = false;

/**
 * @brief    结束解析
 * @author   CXS (chenxiangshu@outlook.com)
 * @date     2024-07-23
 */
#define NPJSON_Builder_End() \
    NPJSON_Release(&__root); \
    }                        \
    while (false)

#define NPJSON_Object_Enter(name)                                      \
    if (strcmp(#name, "NULL") != 0) {                                  \
        __npjson_find(name);                                           \
        __npjson_check_object_type(name, (__ptr->isObject), "Object"); \
    } else {                                                           \
        __npjson_check_array_type((__ptr->isObject), "Object");        \
    }                                                                  \
    __tmp = __ptr;                                                     \
    __ptr = __ptr->next;                                               \
    {                                                                  \
        NPJSONNode *__obj = __tmp;                                     \
        NPJSONNode *__ptr = __obj->Val.Object;

#define NPJSON_Object_Exit() }

#define NPJSON_Array_Enter(name)                                     \
    if (strcmp(#name, "NULL") != 0) {                                \
        __npjson_find(name);                                         \
        __npjson_check_object_type(name, (__ptr->isArray), "Array"); \
    } else {                                                         \
        __npjson_check_array_type((__ptr->isArray), "Array");        \
    }                                                                \
    __tmp = __ptr;                                                   \
    __ptr = __ptr->next;                                             \
    {                                                                \
        NPJSONNode *__obj = __tmp;                                   \
        NPJSONNode *__ptr = __obj->Val.Object;

#define NPJSON_Array_Exit() }

#define NPJSON_Object_GetInt(name, ret) \
    __npjson_object_get(                \
        name, (__ptr->isInteger || __ptr->isNumber), "Int", Value, ret)

#define NPJSON_Object_GetNumber(name, ret) \
    __npjson_object_get(name, (__ptr->isNumber || __ptr->isInteger), "Number", Number, ret)

#define NPJSON_Object_GetBool(name, ret) \
    __npjson_object_get(name, (__ptr->isBinValue), "Bool", BinValue, ret)

#define NPJSON_Object_GetString(name, ret, capacity)                                            \
    {                                                                                           \
        const char *__str = NULL;                                                               \
        __npjson_object_get(name, (__ptr->isString || __ptr->isNull), "String", String, __str); \
        if (__str == NULL || (capacity) == 0)                                                   \
            ;                                                                                   \
        else {                                                                                  \
            size_t __s = strlen(__str);                                                         \
            if (__s >= (capacity)-1)                                                            \
                __s = (capacity)-1;                                                             \
            memcpy(ret, __str, __s);                                                            \
            (ret)[__s] = '\0';                                                                  \
        }                                                                                       \
    }

#define NPJSON_Array_GetInt(ret) \
    __npjson_array_get(          \
        (__ptr->isInteger || __ptr->isNumber), "Int", Value, ret)

#define NPJSON_Array_GetNumber(ret) \
    __npjson_array_get((__ptr->isNumber || __ptr->isInteger), "Number", Number, ret)

#define NPJSON_Array_GetBool(name, ret) \
    __npjson_array_get((__ptr->isBinValue), "Bool", BinValue, ret)

#define NPJSON_Array_GetString(ret, capacity)                                            \
    {                                                                                    \
        const char *__str = NULL;                                                        \
        __npjson_array_get((__ptr->isString || __ptr->isNull), "String", String, __str); \
        if (__str == NULL || (capacity) <= 0)                                            \
            ;                                                                            \
        else {                                                                           \
            size_t __s = strlen(__str);                                                  \
            if (__s >= (capacity)-1)                                                     \
                __s = (capacity)-1;                                                      \
            memcpy(ret, __str, __s);                                                     \
            (ret)[__s] = '\0';                                                           \
        }                                                                                \
    }

/**
 * @brief    当前字段是否为空
 * @author   CXS (chenxiangshu@outlook.com)
 * @date     2024-07-23
 */
#define NPJSON_Object_IsNull(name, ret) \
    __npjson_find(name);                \
    (ret) = __ptr ? __ptr->isNull : true

/**
 * @brief    当前元素是否位空
 * @author   CXS (chenxiangshu@outlook.com)
 * @date     2024-07-23
 */
#define NPJSON_Array_IsNull(ret) (ret) = __ptr ? __ptr->isNull : true

/**
 * @brief    获取数组数量
 * @author   CXS (chenxiangshu@outlook.com)
 * @date     2024-07-23
 */
#define NPJSON_Array_GetCount() (__is_err ? 0 : NPJSON_GetChildCount(__obj));
/*
    char cmdId[64];
    const char* msg = NULL;
    NPJSON_Builder_Begin(str, sizeof(str), msg);
    int Version = 0;
    NPJSON_Object_GetInt(Version, Version);
    char deviceId[5];
    NPJSON_Object_GetString(deviceId, deviceId, sizeof(deviceId));
    NPJSON_Array_Enter(data);
    int n = NPJSON_Array_GetCount();
    for (int i = 0; i < n; i++)
    {
        NPJSON_Object_Enter(NULL);
        char name[16];
        char value[16];
        NPJSON_Object_GetString(name, name, sizeof(name));
        NPJSON_Object_GetString(value, value, sizeof(value));
        printf("%s = %s\n", name, value);
        NPJSON_Object_Exit();
    }
    NPJSON_Array_Exit();
    NPJSON_Array_Enter(list);
    int n = NPJSON_Array_GetCount();
    int val = 0;
    for (int i = 0; i < n; i++)
    {
        NPJSON_Array_GetInt(val);
        printf("[%d] = %d\n", i, val);
    }
    NPJSON_Array_Exit();
    NPJSON_Builder_End();
*/
#ifdef __cplusplus
}
#endif
#endif   // __CM_NPJSON_H__
