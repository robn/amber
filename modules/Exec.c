#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <jsapi.h>
#include <jsprf.h>
#include <jsstddef.h>

static JSBool _exec_fork(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    pid_t pid;

    pid = fork();
    if(pid >= 0) {
        *rval = INT_TO_JSVAL(pid);
        return JS_TRUE;
    }

    JS_SetPendingException(cx, STRING_TO_JSVAL(JS_NewString(cx, JS_smprintf("fork failed: %s", strerror(errno)), 0)));

    return JS_FALSE;
}

static JSBool _exec_waitpid(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
}

static JSBool _exec_exec(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
}

static JSBool _exec_system(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
}

static JSFunctionSpec exec_functions[] = {
    { "fork",       _exec_fork,     0, JSPROP_ENUMERATE },
    { "waitpid",    _exec_waitpid,  2, JSPROP_ENUMERATE },
    { "exec",       _exec_exec,     1, JSPROP_ENUMERATE },
    { "system",     _exec_system,   1, JSPROP_ENUMERATE },
    { NULL }
};

JSBool Exec(JSContext *cx, JSObject *amber) {
    JSObject *exec;

    exec = JS_NewObject(cx, NULL, NULL, NULL);
    JS_DefineProperty(cx, amber, "Exec", OBJECT_TO_JSVAL(exec), NULL, NULL, JSPROP_ENUMERATE);
    JS_DefineFunctions(cx, exec, exec_functions);

    return JS_TRUE;
}
