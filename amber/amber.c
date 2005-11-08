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
#include "internal.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

static int amber_exit_code = AMBER_EXIT_OK;

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

    while((optchar = getopt(argc, argv, "+vh?")) >= 0) {
        switch(optchar) {
            case 'v':
                printf(" amber version: " VERSION "\n"
                       "engine version: %s\n", JS_GetImplementationVersion());
                return AMBER_EXIT_ARGS;

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

    if((rt = JS_NewRuntime(8L * 1024L * 1024L)) == NULL ||
       (cx = JS_NewContext(rt, 8192)) == NULL)
        { amber_exit_code = AMBER_EXIT_INIT; goto cleanup; }

    JS_SetErrorReporter(cx, amber_error_reporter);

    amber = amber_global_init(cx);
    if(amber == NULL) {
        amber_exit_code = AMBER_EXIT_INIT;
        goto cleanup;
    }

    /* arguments holds the command line arguments */
    if((obj = JS_NewArrayObject(cx, 0, NULL)) == NULL ||

       /* hook it up to the global objects */
       JS_DefineProperty(cx, amber, "arguments", OBJECT_TO_JSVAL(obj), NULL, NULL, JSPROP_ENUMERATE) == JS_FALSE)
        { amber_exit_code = AMBER_EXIT_INIT; goto cleanup; }

    /* loop over argv and add them to the array */
    for(i = optind; i < argc; i++)
        if(JS_DefineElement(cx, obj, i - optind, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, argv[i])), NULL, NULL, JSPROP_ENUMERATE) == JS_FALSE)
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
