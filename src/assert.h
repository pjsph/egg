#ifndef ASSERT_H
#define ASSERT_H

#define dbg_break() __builtin_trap()

#define EASSERT_MSG(expr, msg, ...)                                                             \
    {                                                                                           \
        if((expr)) {                                                                            \
        } else {                                                                                \
            elog(LOG_LEVEL_FATAL, __FILE__, __LINE__, "Assertion failed. " msg, ##__VA_ARGS__); \
            dbg_break();                                                                        \
        }                                                                                       \
    }

#define EASSERT(expr) EASSERT_MSG(expr, "")

#ifdef EDEBUG_MODE
    #define EASSERT_DBG(expr) EASSERT(expr)
#else
    #define EASSERT_DBG(expr)
#endif

#endif // ASSERT_H
