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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#define JS_THREADSAFE 1

#include <jsapi.h>
#include <jsprf.h>
#include <jsstddef.h>
#include <jsinterp.h>

static char *amber_search_path[] = {
    "/usr/local/lib/amber",
    "/usr/local/share/amber",
    NULL
};

#define AMBER_EXIT_OK       (0)
#define AMBER_EXIT_ARGS     (-128)
#define AMBER_EXIT_SCRIPT   (-129)
#define AMBER_EXIT_INIT     (-130)
#define AMBER_EXIT_RUN      (-131)

static int amber_exit_code = AMBER_EXIT_OK;

static JSClass amber_exception_class;

static JSBool amber_exception_constructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    return amber_exception_class.construct(cx, obj, argc, argv, rval);
}

static void amber_exception_init(JSContext *cx, JSObject *amber) {
    JSObject *proto, *class;
    jsval fval, pval;

    JS_GetProperty(cx, amber, "Error", &fval);
    JS_CallFunctionValue(cx, amber, fval, 0, NULL, &pval);
    proto = JSVAL_TO_OBJECT(pval);

    memcpy(&amber_exception_class, JS_GetClass(cx, proto), sizeof(JSClass));

    amber_exception_class.name = "AmberError";

    class = JS_InitClass(cx, amber, proto, &amber_exception_class, amber_exception_constructor, 3, NULL, NULL, NULL, NULL);
    JS_SetPrivate(cx, class, NULL);

    JS_DefineProperty(cx, class, "name", STRING_TO_JSVAL(JS_NewStringCopyZ(cx, "AmberError")), NULL, NULL, JSPROP_ENUMERATE);
}

static void amber_throw_exception(JSContext *cx, char *format, ...) {
    va_list ap;
    JSObject *amber;
    char *text;
    jsval argv[1], fval, eval;

    va_start(ap, format);
    text = JS_vsmprintf(format, ap);
    va_end(ap);

    argv[0] = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, text));

    JS_free(cx, text);

    amber = JS_GetGlobalObject(cx);

    /* !!! need to add filename/linenumber here too */

    JS_GetProperty(cx, amber, "AmberError", &fval);
    JS_CallFunctionValue(cx, amber, fval, 1, argv, &eval);

    JS_SetPendingException(cx, OBJECT_TO_JSVAL(eval));
}

static int amber_load_script(char *filename, char **script, int *scriptlen) {
    FILE *f;
    int len, pos;

    *scriptlen = 0;

    if(filename == NULL)
        f = stdin;
    
    else {
        f = fopen(filename, "r");
        if(f == NULL)
            return -1;
    }

    *script = NULL; len = pos = 0;
    while(!feof(f)) {
        *script = (char *) realloc(*script, sizeof(char) * (len + 1024));
        len += 1024;
        
        pos += fread(&((*script)[pos]), sizeof(char), 1024, f);
        if(ferror(f)) {
            errno = ferror(f);
            free(*script);
            fclose(f);
            return -1;
        }
    }

    if(filename != NULL)
        fclose(f);

    *scriptlen = pos;

    return 0;
}

static JSBool amber_print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
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
            amber_throw_exception(cx, "Couldn't convert JSString to char *");
            return JS_FALSE;
        }

        printf("%s%s", i > 0 ? " " : "", thing);
    }

    fputc('\n', stdout);

    return JS_TRUE;
}

JSBool amber_load_run_script(JSContext *cx, JSObject *amber, char *thing, jsval *rval) {
    char *script;
    int scriptlen;
    uint32 opts;
    JSBool ret;

    if(amber_load_script(thing, &script, &scriptlen) < 0) {
        amber_throw_exception(cx, "Unable to load '%s': %s", thing, strerror(errno));
        return JS_FALSE;
    }
    if(scriptlen == 0) {
        *rval = JS_TRUE;
        return JS_TRUE;
    }

    opts = JS_GetOptions(cx);
    JS_SetOptions(cx, opts | JSOPTION_COMPILE_N_GO);
        
    ret = JS_EvaluateScript(cx, amber, script, scriptlen, thing, 1, rval);

    JS_SetOptions(cx, opts);

    return ret;
}

static JSBool amber_load(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    JSString *str;
    char *thing;
    struct stat st;
    JSObject *amber;
    jsval result;
    JSObject *search_path;
    jsint i;
    jsuint len;
    JSString *full;
#ifdef HAVE_DLFCN_H
    void *dl;
    JSBool (*init)(JSContext *cx, JSObject *amber);
    char *err;
#endif

    if(argc == 0) {
        amber_throw_exception(cx, "No file or module specified");
        return JS_FALSE;
    }

    amber = JS_GetGlobalObject(cx);

    if((str = JS_ValueToString(cx, argv[0])) == NULL ||
       (thing = JS_GetStringBytes(str)) == NULL) {
        amber_throw_exception(cx, "Couldn't convert JSString to char *");
        return JS_FALSE;
    }

    if(stat(thing, &st) == 0)
        return amber_load_run_script(cx, amber, thing, rval);

    JS_GetProperty(cx, JSVAL_TO_OBJECT(argv[-2]), "searchPath", &result);
    if(JSVAL_IS_VOID(result)) {
        amber_throw_exception(cx, "Module search path array not defined");
        return JS_FALSE;
    }
    search_path = JSVAL_TO_OBJECT(result);

    JS_GetArrayLength(cx, search_path, &len);

    for(i = 0; i < len; i++) {
        JS_GetElement(cx, search_path, i, &result);
        full = JS_ConcatStrings(cx, JS_ConcatStrings(cx, JSVAL_TO_STRING(result), JS_NewStringCopyZ(cx, "/")), str);

        thing = JS_GetStringBytes(JS_ConcatStrings(cx, full, JS_NewStringCopyZ(cx, ".js")));
        if(stat(thing, &st) == 0)
            return amber_load_run_script(cx, amber, thing, rval);

#ifdef HAVE_DLFCN_H
        thing = JS_GetStringBytes(JS_ConcatStrings(cx, full, JS_NewStringCopyZ(cx, ".so")));
        if(stat(thing, &st) == 0) {
            dl = dlopen(thing, RTLD_LOCAL | RTLD_NOW);
            if((err = dlerror()) != NULL) {
                amber_throw_exception(cx, "Couldn't open shared object '%s': %s", thing, err);
                return JS_FALSE;
            }

            init = dlsym(dl, JS_GetStringBytes(str));
            if((err = dlerror()) != NULL) {
                amber_throw_exception(cx, "Couldn't get initialiser for shared object '%s': %s", thing, err);
                return JS_FALSE;
            }

            *rval = init(cx, amber);
            return *rval;
        }
#endif
    }

    amber_throw_exception(cx, "Can't find a candidate for module '%s'", JS_GetStringBytes(str));
    return JS_FALSE;
}

static JSBool amber_exit(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
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
    { "print",  amber_print, 0, JSPROP_ENUMERATE },
    { "load",   amber_load,  1, JSPROP_ENUMERATE },
    { "exit",   amber_exit,  0, JSPROP_ENUMERATE },
    { NULL }
};

static JSFunctionSpec amber_core_functions[] = {
    { "print",  amber_print, 0, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT },
    { "load",   amber_load,  1, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT },
    { "exit",   amber_exit,  0, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT },
    { NULL }
};

static JSClass amber_class = {
    "Amber", 0,
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub
};


static void amber_error_reporter(JSContext *cx, const char *message, JSErrorReport *report) {
    fputs(message, stderr);

    if(report != NULL) {
        if(report->filename != NULL)
            fprintf(stderr, " in %s", report->filename);
        if(report->lineno > 0)
            fprintf(stderr, " at line %u", report->lineno);
    }

    fputc('\n', stderr);
}

int main(int argc, char **argv) {
    int optchar;
    char *filename, *pretty;
    char *script;
    int scriptlen, i;
    JSRuntime *rt;
    JSContext *cx;
    JSObject *amber, *core, *obj;
    jsval rval;

    while((optchar = getopt(argc, argv, "+h?")) >= 0) {
        switch(optchar) {
            case 'h': case '?': default:
                fputs(
                    "amber - javascript script host\n"
                    "Usage: amber [options] [scriptfile]\n", stdout);
                return AMBER_EXIT_ARGS;
        }
    }

    if(optind >= argc || strcmp(argv[optind], "-") == 0) {
        pretty = "(stdin)";
        filename = NULL;
    }

    else
        pretty = filename = argv[optind];

    if(amber_load_script(filename, &script, &scriptlen) < 0) {
        fprintf(stderr, "Unable to read '%s': %s\n", pretty, strerror(errno));
        return AMBER_EXIT_SCRIPT;
    }

    if(scriptlen == 0)
        return AMBER_EXIT_OK;

    optind++;

    /* start me up */
    if((rt = JS_NewRuntime(8L * 1024L * 1024L)) == NULL ||
       (cx = JS_NewContext(rt, 8192)) == NULL)
        { amber_exit_code = AMBER_EXIT_INIT; goto cleanup; }

    JS_SetErrorReporter(cx, amber_error_reporter);

    /* the global object */
    if((amber = JS_NewObject(cx, &amber_class, NULL, NULL)) == NULL ||

       /* load all the builtin stuff into it */
       JS_InitStandardClasses(cx, amber) == JS_FALSE ||

       /* get our core functions online */
       JS_DefineFunctions(cx, amber, amber_functions) == JS_FALSE ||

       /* and the loopback so we can access ourselves directly */
       JS_DefineProperty(cx, amber, "amber", OBJECT_TO_JSVAL(amber), NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT) == JS_FALSE)
        { amber_exit_code = AMBER_EXIT_INIT; goto cleanup; }

    /* core is a "mirror" of our builtins, but immutable */
    if((core = JS_NewObject(cx, NULL, NULL, NULL)) == NULL ||

       /* hook it up to the global object */
       JS_DefineProperty(cx, amber, "core", OBJECT_TO_JSVAL(core), NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT) == JS_FALSE ||

       /* and add the functions to it */
       JS_DefineFunctions(cx, core, amber_core_functions) == JS_FALSE)
        { amber_exit_code = AMBER_EXIT_INIT; goto cleanup; }

    /* arguments holds the command line arguments */
    if((obj = JS_NewArrayObject(cx, 0, NULL)) == NULL ||

       /* hook it up to the global objects */
       JS_DefineProperty(cx, amber, "arguments", OBJECT_TO_JSVAL(obj), NULL, NULL, JSPROP_ENUMERATE) == JS_FALSE)
        { amber_exit_code = AMBER_EXIT_INIT; goto cleanup; }

    /* loop over argv and add them to the array */
    for(i = optind; i < argc; i++)
        if(JS_DefineElement(cx, obj, i - optind, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, argv[i])), NULL, NULL, JSPROP_ENUMERATE) == JS_FALSE)
            { amber_exit_code = AMBER_EXIT_INIT; goto cleanup; }
    
    /* the load function has a search path initialised at compile time */
    if(JS_GetProperty(cx, amber, "load", &rval) == JS_FALSE ||

       /* make a new array */
       (obj = JS_NewArrayObject(cx, 0, NULL)) == NULL ||

       /* and attach it to the function */
       JS_DefineProperty(cx, JSVAL_TO_OBJECT(rval), "searchPath", OBJECT_TO_JSVAL(obj), NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT) == JS_FALSE)
        { amber_exit_code = AMBER_EXIT_INIT; goto cleanup; }

    /* loop over the compiled in values and add them in */
    for(i = 0; amber_search_path[i] != NULL; i++)
        if(JS_DefineElement(cx, obj, i, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, amber_search_path[i])), NULL, NULL, JSPROP_ENUMERATE) == JS_FALSE)
            { amber_exit_code = AMBER_EXIT_INIT; goto cleanup; }

    /* we also have to add this to core.load */
    if(JS_GetProperty(cx, core, "load", &rval) == JS_FALSE ||
       JS_DefineProperty(cx, JSVAL_TO_OBJECT(rval), "searchPath", OBJECT_TO_JSVAL(obj), NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT) == JS_FALSE)
        { amber_exit_code = AMBER_EXIT_INIT; goto cleanup; }

    amber_exception_init(cx, amber);

    if(JS_EvaluateScript(cx, amber, script, scriptlen, pretty, 1, &rval) == JS_FALSE)
        amber_exit_code = AMBER_EXIT_RUN;

cleanup:
    switch(amber_exit_code) {
        case AMBER_EXIT_INIT:
            fputs("amber initialisation failed\n", stderr);
            break;
    }

    if(cx != NULL) JS_DestroyContext(cx);
    if(rt != NULL) JS_DestroyRuntime(rt);
    if(script != NULL) free(script);

    return amber_exit_code;
}
