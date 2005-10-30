#include <stdio.h>
#include <stdlib.h>

#include <jsapi.h>
#include <jsprf.h>
#include <jsstddef.h>

static JSBool file_open(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    JSString *str;
    char *name, *mode;
    FILE *f;

    if(argc == 0)
        return JS_TRUE;

    str = JS_ValueToString(cx, argv[0]);
    name = JS_GetStringBytes(str);

    if(argc > 1) {
        str = JS_ValueToString(cx, argv[1]);
        mode = JS_GetStringBytes(str);
    }
    else
        mode = "r";

    if((f = JS_GetPrivate(cx, obj)) != NULL) {
        fclose(f);
        JS_SetPrivate(cx, obj, NULL);
    }
    
    f = fopen(name, mode);
    JS_SetPrivate(cx, obj, f);
    
    return JS_TRUE;
}

static JSBool file_close(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    FILE *f;

    if((f = JS_GetPrivate(cx, obj)) != NULL) {
        fclose(f);
        JS_SetPrivate(cx, obj, NULL);
    }

    return JS_TRUE;
}

static JSBool file_read(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    FILE *f;
    char *buf;
    int want, len, pos;
    JSString *str;

    if((f = JS_GetPrivate(cx, obj)) == NULL)
        return JS_TRUE;

    if(argc == 0)
        want = -1;

    else if(JS_ValueToInt32(cx, argv[0], &want) == JS_FALSE)
        return JS_TRUE;

    if(want == 0)
        return JS_TRUE;

    buf = NULL; len = pos = 0;
    while(!feof(f) && want > 0 && pos < want) {
        if(len < pos + 1024) {
            buf = (char *) JS_realloc(cx, buf, sizeof(char) * (len + 1024));
            len += 1024;
        }

        if(want > 0 && want - pos < 1024)
            pos += fread(&(buf[pos]), sizeof(char), want - pos, f);
        else
            pos += fread(&(buf[pos]), sizeof(char), 1024, f);
    }

    if(pos == 0) {
        if(buf != NULL)
            JS_free(cx, buf);
        return JS_TRUE;
    }

    if(pos == len)
        buf = (char *) JS_realloc(cx, buf, sizeof(char) * (len + 1));

    buf[pos] = '\0';

    str = JS_NewString(cx, buf, pos);
    *rval = STRING_TO_JSVAL(str);

    return JS_TRUE;
}

static JSFunctionSpec file_methods[] = {
    { "open",   file_open,  2, 0, 0 },
    { "close",  file_close, 0, 0, 0 },
    { "read",   file_read,  1, 0, 0 }
};

static JSBool file_constructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    if(argc == 0) {
        JS_SetPrivate(cx, obj, NULL);
        return JS_TRUE;
    }

    file_open(cx, obj, argc, argv, rval);

    return JS_TRUE;
}

static void file_finalize(JSContext *cx, JSObject *obj) {
    FILE *f;

    if((f = JS_GetPrivate(cx, obj)) != NULL) {
        fclose(f);
        JS_SetPrivate(cx, obj, NULL);
    }
}

static JSClass file_class = {
    "File", JSCLASS_HAS_PRIVATE,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, file_finalize
};

JSBool File(JSContext *cx, JSObject *amber) {
    JSObject *file;

    file = JS_InitClass(cx, amber, NULL, &file_class,
                        file_constructor, 2,
                        NULL, file_methods,
                        NULL, NULL);

    return JS_TRUE;
}