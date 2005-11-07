#include "amber/amber.h"

#include <pthread.h>
#include <errno.h>

#define JS_THREADSAFE 1
#include <jsapi.h>

static JSBool mutex_lock(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    pthread_mutex_t *m;

    if((m = JS_GetPrivate(cx, obj)) == NULL)
        return JS_TRUE;

    pthread_mutex_lock(m);

    return JS_TRUE;
}

static JSBool mutex_trylock(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    pthread_mutex_t *m;

    if((m = JS_GetPrivate(cx, obj)) == NULL)
        return JS_TRUE;

    if(pthread_mutex_trylock(m) == EBUSY)
        *rval = BOOLEAN_TO_JSVAL(JS_FALSE);
    else
        *rval = BOOLEAN_TO_JSVAL(JS_TRUE);

    return JS_TRUE;
}

static JSBool mutex_unlock(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    pthread_mutex_t *m;

    if((m = JS_GetPrivate(cx, obj)) == NULL)
        return JS_TRUE;

    pthread_mutex_unlock(m);

    return JS_TRUE;
}

static JSFunctionSpec mutex_methods[] = {
    { "lock",       mutex_lock,     0, 0 },
    { "trylock",    mutex_trylock,  0, 0 },
    { "unlock",     mutex_unlock,   0, 0 },
    { NULL }
};

static JSBool mutex_constructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    pthread_mutex_t *m;

    m = JS_malloc(cx, sizeof(pthread_mutex_t));
    pthread_mutex_init(m, NULL);

    JS_SetPrivate(cx, obj, m);

    return JS_TRUE;
}

static void mutex_finalize(JSContext *cx, JSObject *obj) {
    pthread_mutex_t *m;

    if((m = JS_GetPrivate(cx, obj)) != NULL)
        pthread_mutex_destroy(m);
}

static JSClass mutex_class = {
    "Mutex", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, mutex_finalize
};

JSBool Mutex(JSContext *cx, JSObject *amber) {
    JSObject *mutex;

    mutex = JS_InitClass(cx, amber, NULL, &mutex_class,
                         mutex_constructor, 0,
                         NULL, mutex_methods,
                         NULL, NULL);

    return JS_TRUE;
}
