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

#include <string.h>
#include <stdarg.h>

#include "amber.h"

#include <jsprf.h>

static JSClass amber_exception_class;

static JSBool amber_exception_constructor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    return amber_exception_class.construct(cx, obj, argc, argv, rval);
}

void amber_exception_init(JSContext *cx, JSObject *amber) {
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

JSBool amber_exception_throw(JSContext *cx, char *format, ...) {
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

    return JS_FALSE;
}
