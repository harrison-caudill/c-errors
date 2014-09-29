/*
 * Copyright (c) 2014 Harrison Caudill
 * All Rights Reserved
 * harrison@hypersphere.org
 */
#ifndef _ERROR_H
#define _ERROR_H

#include <stdint.h>
#include <sys/errno.h>

#include "error_codes.h"

/*
 * Error Type (64-bits):
 * 0 1              16       24       32                               64
 * +-+---------------+--------+--------+--------------------------------+
 * |E| RESERVED      | FLAGS  | SUBSYS | ERROR CODE                     |
 * +-+---------------+--------+--------+--------------------------------+
 *
 */

typedef union
{
    uint64_t bits;
    struct
    {
        uint16_t is_error      :1;
        uint16_t reserved      :15;
        union
        {
            uint8_t flags;
            struct
            {
                uint8_t is_transient :1;
                uint8_t is_critical  :1;
                uint8_t spare_2 :1;
                uint8_t spare_3 :1;
                uint8_t spare_4 :1;
                uint8_t spare_5 :1;
                uint8_t spare_6 :1;
                uint8_t spare_7 :1;
            } fields;
        } flags;
        uint8_t subsys;
        uint32_t code;
#ifdef DEBUG
        const char *file;
        const char *function;
        int line;
#endif
    } fields;
} err_t;

#define ERR_FLAG_TRANSIENT (0x1<<7)
#define ERR_FLAG_CRITICAL  (0x1<<6)

#define ERR_IS_TRANSIENT(err) ((err).fields.flags.fields.is_transient)
#define ERR_IS_CRITICAL(err)  ((err).fields.flags.fields.is_critical)

#define ERR_STATIC_MASK 0x8000ffffffffffff
#define ERR_STATIC_INT(err) ((err).bits & ERR_STATIC_MASK)
#define ERR_COMPARE(a, b) (ERR_STATIC_INT(a) == ERR_STATIC_INT(b))

#define ERR_ERRNO() ERR_NEW_ERROR(TRUE, 0, ERR_SUBSYS_ERRNO, errno)

#define ERR_INIT()                              \
    err_t __err_retval = ERR_OK

#define ERR_RETURN() return ERR_RETVAL()

#define ERR_IS_ERROR(err) !!(err).fields.is_error
#define ERR_IS_OK(err) !ERR_IS_ERROR(err)

/* This no-op cast is performed so that any attempt to use the retval
 * as an lvalue will fail.  It also prevents its use as an rvalue for
 * &ERR_RETVAL()
 */
#define ERR_RETVAL() ((err_t)(__err_retval.bits))

#define ERR_DONE()                                              \
    __error_done:                                               \
    do{}while(FALSE) /* To handle the danglging semicolon */

#define ERR_GOTO_DONE() goto __error_done

#define ERR_RETURN() return ERR_RETVAL()

#define ERR_SET(err) __err_retval = (err)

#define ERR_NAME(err) ERROR_NAME_STRINGS[err.fields.subsys][err.fields.code]
#define ERR_DESC(err) ERROR_SHORT_STRINGS[err.fields.subsys][err.fields.code]

#define ERR_OOM_CHECK(ptr)                      \
        ERR_CRIT_PTR((ptr),                     \
                     ERR_ERRNO_ENOMEM,          \
                     "Out of Memory")

#define ERROR_LOG_ERROR(lvl, err, msg, ...)                             \
    do {                                                                \
        LOG(lvl, "+------------------------------------------------+"); \
        LOG(lvl, "  Error: %s", ERR_NAME(err));                         \
        LOG(lvl, "  Description: %s", ERR_DESC(err));                   \
        LOG(lvl, "  File:     %s", __FILE__);                           \
        LOG(lvl, "  Function: %s", __FUNCTION__);                       \
        LOG(lvl, "  Line:     %d", __LINE__);                           \
        LOG(lvl, "  Msg:");                                             \
        LOG(lvl, msg, ## __VA_ARGS__);                                  \
        LOG(lvl, "+------------------------------------------------+"); \
    } while(FALSE)

/* Master macros */
#define __ERR_PTR(_ptr, _err, _lvl, msg, ...)           \
    do {                                                \
        void *__ptr = (_ptr);                           \
        err_t __err = (_err);                           \
        int __lvl = (_lvl);                             \
        if (NULL == __ptr)                              \
        {                                               \
            if (0 <= __lvl)                             \
            {                                           \
                ERROR_LOG_ERROR(__lvl, __err,           \
                                msg, ## __VA_ARGS__);   \
            }                                           \
            ERR_SET(__err);                             \
            ERR_GOTO_DONE();                            \
        }                                               \
    } while(FALSE)

#define __ERR_COND(_val, _err, _lvl, msg, ...)                  \
    do {                                                        \
        int __val = !!(_val);                                   \
        err_t __err = (_err);                                   \
        int __lvl = (_lvl);                                     \
        if (__val)                                              \
        {                                                       \
            if (0 <= __lvl)                                     \
            {                                                   \
                ERROR_LOG_ERROR(__lvl, __err,                   \
                                msg, ## __VA_ARGS__);           \
            }                                                   \
            ERR_SET(__err);                                     \
            ERR_GOTO_DONE();                                    \
        }                                                       \
    } while(FALSE)

#define __ERR_RET(_val, _lvl, msg, ...)                 \
        do {                                            \
            int __val = (_val);                         \
            err_t __err = ERR_ERRNO();                  \
            int __lvl = (_lvl);                         \
            if (0 > __val)                              \
            {                                           \
                if (0 <= __lvl)                         \
                {                                       \
                ERROR_LOG_ERROR(__lvl, __err,           \
                                msg, ## __VA_ARGS__);   \
                }                                       \
                ERR_SET(__err);                         \
                ERR_GOTO_DONE();                        \
            }                                           \
        } while(FALSE)

#define __ERR_ERR(_err, _lvl, msg, ...)                             \
        do {                                                        \
            err_t __err = (_err);                                   \
            int __lvl = (_lvl);                                     \
            if (ERR_IS_ERROR(__err))                                \
            {                                                       \
                if (0 <= __lvl)                                     \
                {                                                   \
                    ERROR_LOG_ERROR(__lvl, __err,                   \
                                    msg, ## __VA_ARGS__);           \
                }                                                   \
                ERR_SET(__err);                                     \
                ERR_GOTO_DONE();                                    \
            }                                                       \
        } while(FALSE)

#define ERR_SILENT_PTR(ptr, err) __ERR_PTR(ptr, err, -1, " ")
#define ERR_SILENT_COND(val, err) __ERR_COND(val, err, -1, " ")
#define ERR_SILENT_RET(val) __ERR_RET(val, -1, " ")
#define ERR_SILENT_ERR(err) __ERR_ERR(err, -1, " ")

#define ERR_DEF_PTR(ptr, err, msg, ...) \
        __ERR_PTR(ptr, err, ERR_LOG_LVL_DEFAULT, msg, ## __VA_ARGS__)
#define ERR_DEF_COND(val, err, msg, ...) \
        __ERR_COND(val, err, ERR_LOG_LVL_DEFAULT, msg, ## __VA_ARGS__)
#define ERR_DEF_RET(val, msg, ...) \
        __ERR_RET(val, ERR_LOG_LVL_DEFAULT, msg, ## __VA_ARGS__)
#define ERR_DEF_ERR(err, msg, ...) \
        __ERR_ERR(err, ERR_LOG_LVL_DEFAULT, msg, ## __VA_ARGS__)

#define ERR_VERB_PTR(ptr, err, msg, ...) \
        __ERR_PTR(ptr, err, ERR_LOG_LVL_VERBOSE, msg, ## __VA_ARGS__)
#define ERR_VERB_COND(val, err, msg, ...) \
        __ERR_COND(val, err, ERR_LOG_LVL_VERBOSE, msg, ## __VA_ARGS__)
#define ERR_VERB_RET(val, msg, ...) \
        __ERR_RET(val, ERR_LOG_LVL_VERBOSE, msg, ## __VA_ARGS__)
#define ERR_VERB_ERR(err, msg, ...) \
        __ERR_ERR(err, ERR_LOG_LVL_VERBOSE, msg, ## __VA_ARGS__)

#define ERR_DEBUG_PTR(ptr, err, msg, ...) \
        __ERR_PTR(ptr, err, ERR_LOG_LVL_DEBUG, msg, ## __VA_ARGS__)
#define ERR_DEBUG_COND(val, err, msg, ...) \
        __ERR_COND(val, err, ERR_LOG_LVL_DEBUG, msg, ## __VA_ARGS__)
#define ERR_DEBUG_RET(val, msg, ...) \
        __ERR_RET(val, ERR_LOG_LVL_DEBUG, msg, ## __VA_ARGS__)
#define ERR_DEBUG_ERR(err, msg, ...) \
        __ERR_ERR(err, ERR_LOG_LVL_DEBUG, msg, ## __VA_ARGS__)

#define ERR_INFO_PTR(ptr, err, msg, ...) \
        __ERR_PTR(ptr, err, ERR_LOG_LVL_INFO, msg, ## __VA_ARGS__)
#define ERR_INFO_COND(val, err, msg, ...) \
        __ERR_COND(val, err, ERR_LOG_LVL_INFO, msg, ## __VA_ARGS__)
#define ERR_INFO_RET(val, msg, ...) \
        __ERR_RET(val, ERR_LOG_LVL_INFO, msg, ## __VA_ARGS__)
#define ERR_INFO_ERR(err, msg, ...) \
        __ERR_ERR(err, ERR_LOG_LVL_INFO, msg, ## __VA_ARGS__)

#define ERR_WARN_PTR(ptr, err, msg, ...) \
        __ERR_PTR(ptr, err, ERR_LOG_LVL_WARN, msg, ## __VA_ARGS__)
#define ERR_WARN_COND(val, err, msg, ...) \
        __ERR_COND(val, err, ERR_LOG_LVL_WARN, msg, ## __VA_ARGS__)
#define ERR_WARN_RET(val, msg, ...) \
        __ERR_RET(val, ERR_LOG_LVL_WARN, msg, ## __VA_ARGS__)
#define ERR_WARN_ERR(err, msg, ...) \
        __ERR_ERR(err, ERR_LOG_LVL_WARN, msg, ## __VA_ARGS__)

#define ERR_ERROR_PTR(ptr, err, msg, ...) \
        __ERR_PTR(ptr, err, ERR_LOG_LVL_ERROR, msg, ## __VA_ARGS__)
#define ERR_ERROR_COND(val, err, msg, ...) \
        __ERR_COND(val, err, ERR_LOG_LVL_ERROR, msg, ## __VA_ARGS__)
#define ERR_ERROR_RET(val, msg, ...) \
        __ERR_RET(val, ERR_LOG_LVL_ERROR, msg, ## __VA_ARGS__)
#define ERR_ERROR_ERR(err, msg, ...) \
        __ERR_ERR(err, ERR_LOG_LVL_ERROR, msg, ## __VA_ARGS__)

#define ERR_CRIT_PTR(ptr, err, msg, ...) \
        __ERR_PTR(ptr, err, ERR_LOG_LVL_CRITICAL, msg, ## __VA_ARGS__)
#define ERR_CRIT_COND(val, err, msg, ...) \
        __ERR_COND(val, err, ERR_LOG_LVL_CRITICAL, msg, ## __VA_ARGS__)
#define ERR_CRIT_RET(val, msg, ...) \
        __ERR_RET(val, ERR_LOG_LVL_CRITICAL, msg, ## __VA_ARGS__)
#define ERR_CRIT_ERR(err, msg, ...) \
        __ERR_ERR(err, ERR_LOG_LVL_CRITICAL, msg, ## __VA_ARGS__)

#endif
