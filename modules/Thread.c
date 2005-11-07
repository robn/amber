#include "amber/amber.h"

#include <pthread.h>

#define JS_THREADSAFE 1
#include <jsapi.h>

typedef enum thread_state {
    THREAD_INIT,
    THREAD_RUN,
    THREAD_DONE
} thread_state;

typedef struct thread_stuff {
    pthread_mutex_t     mutex;
    thread_state        state;
    pthread_t           t;
    JSRuntime           *rt;
    JSObject            *amber;
    JSFunction          *fun;
    jsval               arg;
} *thread_stuff;

static JSBool thread_join(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    thread_stuff ts;
    void *ret;

    if((ts = JS_GetPrivate(cx, obj)) == NULL)
        return JS_TRUE;

    pthread_join(ts->t, &ret);

    return JS_TRUE;
}

static JSBool thread_detach(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    thread_stuff ts;

    if((ts = JS_GetPrivate(cx, obj)) == NULL)
        return JS_TRUE;

    pthread_detach(ts->t);

    return JS_TRUE;
}

static JSBool thread_exit(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    thread_stuff ts;

    if((ts = JS_GetContextPrivate(cx)) == NULL)
        return JS_TRUE;

    return JS_FALSE;
}

static JSFunctionSpec thread_methods[] = {
    { "join",   thread_join,    0, 0 },
    { "detach", thread_detach,  0, 0 },
    { NULL }
};

static JSFunctionSpec thread_static_methods[] = {
    { "exit",   thread_exit,    0, 0 },
    { NULL }
};

enum thread_tinyid {
    THREAD_STATE
};

static JSPropertySpec thread_properties[] = {
    { "state",  THREAD_STATE,   JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT },
    { NULL }
};

static void *thread_start(void *arg) {
    thread_stuff ts = (thread_stuff) arg;
    JSContext *cx;
    jsval argv[1], rval;
    uintN argc;

    cx = JS_NewContext(ts->rt, 8192);
    JS_SetContextPrivate(cx, ts);

    if(ts->arg != JSVAL_VOID) {
        argv[0] = ts->arg;
        argc = 1;
    } else
        argc = 0;

    ts->state = THREAD_RUN;
    JS_CallFunction(cx, ts->amber, ts->fun, argc, argv, &rval);
    ts->state = THREAD_DONE;

    JS_DestroyContext(cx);

    return NULL;
}

static JSBool thread_constructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    JSFunction *fun;
    jsval arg;
    thread_stuff ts;

    if(argc == 0) {
        *rval = JSVAL_VOID;
        return JS_TRUE;
    }

    fun = JS_ValueToFunction(cx, argv[0]);
    ASSERT_THROW(fun == NULL, "argument is not a function");
    
    if(argc > 1)
        arg = argv[1];
    else
        arg = JSVAL_VOID;

    ts = JS_malloc(cx, sizeof(struct thread_stuff));

    pthread_mutex_init(&ts->mutex, NULL);

    ts->state = THREAD_INIT;

    ts->rt = JS_GetRuntime(cx);
    ts->amber = JS_GetGlobalObject(cx);

    ts->fun = fun;
    ts->arg = arg;

    JS_SetPrivate(cx, obj, ts);

    pthread_create(&ts->t, NULL, thread_start, ts);

    return JS_TRUE;
}

static JSBool thread_get_property(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    thread_stuff ts;
    JSString *str;

    if((ts = JS_GetPrivate(cx, obj)) == NULL)
        return JS_TRUE;

    switch(JSVAL_TO_INT(id)) {
        case THREAD_STATE:
            switch(ts->state) {
                case THREAD_INIT:
                    str = JS_InternString(cx, "initialising");
                    break;

                case THREAD_RUN:
                    str = JS_InternString(cx, "running");
                    break;

                case THREAD_DONE:
                    str = JS_InternString(cx, "done");
                    break;
            }

            *vp = STRING_TO_JSVAL(str);
            break;
    }

    return JS_TRUE;
}

static JSClass thread_class = {
    "Thread", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, thread_get_property, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

JSBool Thread(JSContext *cx, JSObject *amber) {
    JSObject *thread;

    thread = JS_InitClass(cx, amber, NULL, &thread_class,
                          thread_constructor, 1,
                          thread_properties, thread_methods,
                          NULL, thread_static_methods);

    return JS_TRUE;
}
