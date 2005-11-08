#include <stdlib.h>
#include "amber/amber.h"

#include <string.h>

#include <jsapi.h>

static JSBool env_set_property(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
    JSString *idstr, *valstr;
    char *key, *value;
    int rv;

    idstr = JS_ValueToString(cx, id);
    valstr = JS_ValueToString(cx, *vp);
    if(idstr == NULL || valstr == NULL)
        return JS_FALSE;

    key = JS_GetStringBytes(idstr);
    value = JS_GetStringBytes(valstr);

    rv = setenv(key, value, 1);
    ASSERT_THROW(rv < 0, "unable to set environment variable '%s'", key);

    *vp = STRING_TO_JSVAL(valstr);

    return JS_TRUE;
}

static JSClass env_class = {
    "environment", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, env_set_property,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

extern char **environ;

JSBool environment(JSContext *cx, JSObject *amber) {
    JSObject *env;
    char **ev, *key, *value;

    env = JS_DefineObject(cx, amber, "environment", &env_class, NULL, 0);

    for(ev = environ; *ev != NULL; ev++) {
        key = *ev;

        value = strchr(key, '=');
        if(value == NULL)
            continue;
        *value = '\0'; value++;

        JS_DefineProperty(cx, env, key, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, value)), NULL, NULL, JSPROP_ENUMERATE);

        value[-1] = '=';
    }

    return JS_TRUE;
}
