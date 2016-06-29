#ifndef __TAF_HOOK_H__
#define __TAF_HOOK_H__

#define TAF_API extern

#ifdef __cplusplus
extern "C" {
#endif


#define TAF_HOOK_DONE       0
#define TAF_HOOK_CONTINUE   1

enum TafObjectType
{
    TAF_STRING_OBJ = 0,
    TAF_LIST_OBJ   = 1,
    TAF_SET_OBJ    = 2,
    TAF_HASH_OBJ   = 3,
    TAF_ZSET_OBJ   = 4
} ;


typedef struct RotDataRecord
{
    const char *sKey;
    const void *oVal; /* robj *val */
} RotDataRecord;


/*
 *
 */
typedef int (*logSinkerHook)(int, const char *);

typedef int (*toDoFunctorHook)(const RotDataRecord &);

struct RotTodoFunctor
{
    toDoFunctorHook evicted_fn;
    toDoFunctorHook expired_fn;
    /* toDoFunctorHook del_fn; */ //too many places
};


#ifdef __cplusplus
}
#endif

#endif
