/*
 * amber - a Javascript hosting environment for the command line
 * Copyright (c) 2005 Robert Norris
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA02111-1307USA
 */

#include "config.h"

#include "amber.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

static char *amber_search_path[] = {
    "/usr/local/lib/amber",
    "/usr/local/share/amber",
    NULL
};

static JSBool amber_global_print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    uintN i;
    JSString *str;
    char *thing;

    if(argc == 0) {
        fputs("\n", stdout);
        return JS_TRUE;
    }

    for(i = 0; i < argc; i++) {
        if((str = JS_ValueToString(cx, argv[i])) == NULL ||
           (thing = JS_GetStringBytes(str)) == NULL) {
            THROW("couldn't convert argument to char *");
        }

        printf("%s%s", i > 0 ? " " : "", thing);
    }

    fputc('\n', stdout);

    return JS_TRUE;
}

static JSBool amber_global_load(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    JSString *str;
    char *thing;
    jsval result;
    struct stat st;
    JSObject *amber;
    JSObject *search_path;

    ASSERT_THROW(argc == 0, "no file or module specified");

    amber = JS_GetGlobalObject(cx);

    if((str = JS_ValueToString(cx, argv[0])) == NULL ||
       (thing = JS_GetStringBytes(str)) == NULL) {
        THROW("couldn't convert argument to char *");
    }

    if(stat(thing, &st) == 0)
        return amber_run_script(cx, amber, thing, rval);

    JS_GetProperty(cx, JSVAL_TO_OBJECT(argv[-2]), "searchPath", &result);
    ASSERT_THROW(JSVAL_IS_VOID(result), "module search path array not defined");
    search_path = JSVAL_TO_OBJECT(result);

    return amber_load_module(cx, amber, search_path, thing, rval);
}

static JSBool amber_global_exit(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    int code = AMBER_EXIT_OK;
    JSRuntime *rt;

    if(argc > 0)
        code = JSVAL_TO_INT(argv[0]);

    rt = JS_GetRuntime(cx);

    JS_DestroyContext(cx);
    JS_DestroyRuntime(rt);

    exit(code);

    return JS_FALSE;
}

static JSFunctionSpec amber_functions[] = {
    { "print",  amber_global_print, 0, 0 },
    { "load",   amber_global_load,  1, 0 },
    { "exit",   amber_global_exit,  0, 0 },
    { NULL }
};

static JSFunctionSpec amber_core_functions[] = {
    { "print",  amber_global_print, 0, JSPROP_READONLY | JSPROP_PERMANENT },
    { "load",   amber_global_load,  1, JSPROP_READONLY | JSPROP_PERMANENT },
    { "exit",   amber_global_exit,  0, JSPROP_READONLY | JSPROP_PERMANENT },
    { NULL }
};

static JSClass amber_class = {
    "Amber", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};

JSObject *amber_global_init(JSContext *cx) {
    JSObject *amber, *core, *obj;
    jsval rval;
    int i;

    /* the global object */
    if((amber = JS_NewObject(cx, &amber_class, NULL, NULL)) == NULL ||

       /* load all the builtin stuff into it */
       JS_InitStandardClasses(cx, amber) == JS_FALSE ||

       /* get our core functions online */
       JS_DefineFunctions(cx, amber, amber_functions) == JS_FALSE ||

       /* and the loopback so we can access ourselves directly */
       JS_DefineProperty(cx, amber, "amber", OBJECT_TO_JSVAL(amber), NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT) == JS_FALSE)
        return NULL;


    /* core is a "mirror" of our builtins, but immutable */
    if((core = JS_NewObject(cx, NULL, NULL, NULL)) == NULL ||

       /* hook it up to the global object */
       JS_DefineProperty(cx, amber, "core", OBJECT_TO_JSVAL(core), NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT) == JS_FALSE ||

       /* and add the functions to it */
       JS_DefineFunctions(cx, core, amber_core_functions) == JS_FALSE)
        return NULL;


    /* the load function has a search path initialised at compile time */
    if(JS_GetProperty(cx, amber, "load", &rval) == JS_FALSE ||

       /* make a new array */
       (obj = JS_NewArrayObject(cx, 0, NULL)) == NULL ||

       /* and attach it to the function */
       JS_DefineProperty(cx, JSVAL_TO_OBJECT(rval), "searchPath", OBJECT_TO_JSVAL(obj), NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT) == JS_FALSE)
        return NULL;

    /* loop over the compiled in values and add them in */
    for(i = 0; amber_search_path[i] != NULL; i++)
        if(JS_DefineElement(cx, obj, i, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, amber_search_path[i])), NULL, NULL, JSPROP_ENUMERATE) == JS_FALSE)
            return NULL;

    /* we also have to add this to core.load */
    if(JS_GetProperty(cx, core, "load", &rval) == JS_FALSE ||
       JS_DefineProperty(cx, JSVAL_TO_OBJECT(rval), "searchPath", OBJECT_TO_JSVAL(obj), NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT) == JS_FALSE)
        return NULL;

    return amber;
}
