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

#ifndef AMBER_H
#define AMBER_H 1

#define JS_THREADSAFE 1
#include <jsapi.h>

extern int amber_load_script(char *filename, char **script, int *scriptlen);
extern JSBool amber_run_script(JSContext *cx, JSObject *amber, char *filename, jsval *rval);
extern JSBool amber_load_module(JSContext *cx, JSObject *amber, JSObject *search_path, char *thing, jsval *rval);

extern JSBool amber_exception_throw(JSContext *cx, char *format, ...);

#define ASSERT_THROW(expr, ...) \
    if(expr) \
        return amber_exception_throw(cx, __VA_ARGS__)

#define THROW(...) \
    return amber_exception_throw(cx, __VA_ARGS__)

#define AMBER_EXIT_OK       (0)
#define AMBER_EXIT_ARGS     (-128)
#define AMBER_EXIT_SCRIPT   (-129)
#define AMBER_EXIT_INIT     (-130)
#define AMBER_EXIT_RUN      (-131)

#endif
