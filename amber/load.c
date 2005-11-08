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
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

int amber_load_script(char *filename, char **script, int *scriptlen) {
    FILE *f;
    int first = 1, len, pos;
    int c;

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
        if(len < pos + 1024) {
            *script = (char *) realloc(*script, sizeof(char) * (len + 1024));
            len += 1024;
        }
        
        pos += fread(&((*script)[pos]), sizeof(char), 1024, f);
        if(ferror(f)) {
            errno = ferror(f);
            free(*script);
            fclose(f);
            return -1;
        }

        if(first && pos >= 2) {
            if((*script)[0] == '#' && (*script)[1] == '!') {
                for(c = 2; c < pos && (*script)[c] != '\n' && (*script)[c] != '\r'; c++);
                if(c == pos)
                    continue;

                pos = pos - c;
                memmove((*script), &((*script)[c]), pos);
                (*script)[pos] = '\0';

                first = 0;
            }
        }
    }

    if(filename != NULL)
        fclose(f);

    *scriptlen = pos;

    return 0;
}

JSBool amber_run_script(JSContext *cx, JSObject *amber, char *filename, jsval *rval) {
    char *script;
    int scriptlen;
    uint32 opts;
    JSBool ret;

    if(amber_load_script(filename, &script, &scriptlen) < 0) {
        THROW("unable to load '%s': %s", filename, strerror(errno));
        return JS_FALSE;
    }
    if(scriptlen == 0) {
        *rval = JS_TRUE;
        return JS_TRUE;
    }

    opts = JS_GetOptions(cx);
    JS_SetOptions(cx, opts | JSOPTION_COMPILE_N_GO);
        
    ret = JS_EvaluateScript(cx, amber, script, scriptlen, filename, 1, rval);

    JS_SetOptions(cx, opts);

    return ret;
}

JSBool amber_load_module(JSContext *cx, JSObject *amber, JSObject *search_path, char *thing, jsval *rval) {
    jsuint i, len;
    jsval result;
    struct stat st;
    JSString *str, *full;
#ifdef HAVE_DLFCN_H
    void *dl;
    JSBool (*init)(JSContext *cx, JSObject *amber);
    char *err;
#endif
    
    str = JS_NewStringCopyZ(cx, thing);
    JS_GetArrayLength(cx, search_path, &len);

    for(i = 0; i < len; i++) {
        JS_GetElement(cx, search_path, i, &result);
        full = JS_ConcatStrings(cx, JS_ConcatStrings(cx, JSVAL_TO_STRING(result), JS_NewStringCopyZ(cx, "/")), str);

        thing = JS_GetStringBytes(JS_ConcatStrings(cx, full, JS_NewStringCopyZ(cx, ".js")));
        if(stat(thing, &st) == 0)
            return amber_run_script(cx, amber, thing, rval);

#ifdef HAVE_DLFCN_H
        thing = JS_GetStringBytes(JS_ConcatStrings(cx, full, JS_NewStringCopyZ(cx, ".so")));
        if(stat(thing, &st) == 0) {
            dl = dlopen(thing, RTLD_NOW | RTLD_LOCAL);
            ASSERT_THROW((err = dlerror()) != NULL, "couldn't open shared object '%s': %s", thing, err);

            init = dlsym(dl, JS_GetStringBytes(str));
            ASSERT_THROW((err = dlerror()) != NULL, "couldn't get initialiser for shared object '%s': %s", thing, err);

            *rval = init(cx, amber);
            return *rval;
        }
#endif
    }

    THROW("can't find a candidate for module '%s'", JS_GetStringBytes(str));
}
