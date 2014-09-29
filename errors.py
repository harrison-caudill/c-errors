#!/usr/bin/python

import os
import sys
import re
import pprint

import error_codes

def c_escape(s):
    return s.replace('"', '\\"')

def errno_codes():

    # Read in the errno documentation to snarf the codes
    fd = open('error_decls/errno.dat', 'r')
    errno_raw = fd.read()
    fd.close()

    err_vals = errno_vals()

    err_rxp = '^ {7}E[A-Z0-9]+ '
    err_rcm = re.compile(err_rxp)
    errno_codes = []
    err = None
    short_desc = None
    long_desc = None
    for p in errno_raw.split('\n\n'):

        if (err_rcm.match(p)):
            # save old one
            # FIXME: Will have to manually set flags, etc
            if err:
                err_val = err_vals[err]
                assert (err_val is not None)
                errno_codes.append((err,
                                    True,
                                    0,
                                    err,
                                    short_desc,
                                    long_desc,
                                    err_val))

            # new error code
            err = p.split()[0]
            short_desc = c_escape(' '.join(p.split()[1:]))
            long_desc = ''
        elif len(p):
            long_desc = c_escape(' '.join(p.split()))
    return errno_codes

def errno_vals():
    vals = {}
    nrxp = 'E[A-Z0-9]+'
    nrcm = re.compile(nrxp)
    vrxp = '[1-9][0-9]*'
    vrcm = re.compile(vrxp)
    deferred = []
    def __vals(f, vals):
        fd = open(f, 'r')
        raw = fd.read()
        fd.close()
        for line in raw.split('\n'):
            if not len(line): continue
            parts = line.split()
            if not 3 == len(parts): continue
            if (parts[0] == '#define'
                and nrcm.match(parts[1])
                and vrcm.match(parts[2])):
                vals[parts[1]] = int(parts[2])
            if (parts[0] == '#define'
                and nrcm.match(parts[1])
                and nrcm.match(parts[2])):
                deferred.append((parts[1], parts[2],))
    __vals('error_decls/errno.h', vals)
    __vals('error_decls/errno-base.h', vals)
    for n, a, in deferred: vals[n] = vals[a]
    return vals

def get_definition_strings(subsystems, codes):
    i = 0
    code_strings = []
    for s in subsystems:
        sys_codes = codes[s]
        for c in sys_codes:
            (ename, is_err, flags, code_str, short_str, long_str, val,) = c
            sys_str = 'ERR_SUBSYS_%s'%s
            if code_str is None: code_str = '%#8.8x'%i
            i += 1
            is_err = {True: 'TRUE', False: 'FALSE'}[is_err]
            if ('UTIL' == s):
                cstring = '#define ERR_%s ERR_NEW_ERROR(%s, 0, %s, %s)'
                code_strings.append(cstring%(ename, is_err, sys_str, code_str))
            else:
                cstring = '#define ERR_%s_%s ERR_NEW_ERROR(%s, 0, %s, %s)'
                code_strings.append(cstring%(s, ename, is_err, sys_str, code_str))
    return code_strings

def get_code_strings(subsystems, codes):
    i = 0
    code_strings = []
    for s in subsystems:
        sys_codes = codes[s]
        for c in sys_codes:
            (ename, is_err, flags, code_str, short_str, long_str, val,) = c
            sys_str = 'ERR_SUBSYS_%s'%s
            if code_str is None: code_str = '%#8.8x'%i
            i += 1
            is_err = {True: 'TRUE', False: 'FALSE'}[is_err]
            subsys_part = '_%s'%s
            if 'UTIL' == s: subsys_part = ''
            name = 'ERR_CODE%s_%s'%(subsys_part, ename)
            cstring = '#define %s ((ERR_SUBSYS_%s<<32)|%s)'%(name, s, code_str)
            code_strings.append(cstring)
    return code_strings

def get_short_strings(subsystems, codes):
    short_strings = ["char **ERROR_SHORT_STRINGS[%d] = {"%(len(subsystems)+2),
                     "    {},"]
    for s in subsystems:
        short_strings.append("    {")
        sys_codes = codes[s]
        for c in sys_codes:
            (ename, is_err, flags, code_str, short_str, long_str, val) = c
            s = '        "%s",'%short_str
            short_strings.append(s)
        short_strings.append("    },")
    short_strings.append("    {},")
    short_strings.append("};")
    return short_strings

err_code_transient = (1<<7)
err_code_critical = (1<<6)

def util_codes():
    codes = error_codes.CODES

    retval = []
    n = 0
    for a, b, c, d, e, f, in codes:
        retval.append((a, b, c, '%#8.8x'%n, e, f, n))
        n += 1
    return retval

def write_header_file(subsystems, codes):
    fmt = """/*
 * Copyright (c) 2014 Harrison Caudill
 * All Rights Reserved
 * harrison@hypersphere.org
 */

#ifndef _ERROR_CODES_H
#define _ERROR_CODES_H


/*****************************************************************************/
/* Error Subsystem Definitions                                               */
/*****************************************************************************/

enum
{
  ERR_SUBSYS_INVALID_LOW=-1,
%(subsystem_names)s
  ERR_SUBSYS_INVALID_HIGH
};


/*****************************************************************************/
/* External Static Strings                                                   */
/*****************************************************************************/

extern char **ERROR_SHORT_STRINGS[ERR_SUBSYS_INVALID_HIGH+1];
extern char **ERROR_LONG_STRINGS[ERR_SUBSYS_INVALID_HIGH+1];
extern char **ERROR_NAME_STRINGS[ERR_SUBSYS_INVALID_HIGH+1];


/*****************************************************************************/
/* Static Codes (subsys/code) for Switch/Case Statements                     */
/*****************************************************************************/

%(error_codes)s


/*****************************************************************************/
/* Error Object Definitions (err_t)                                          */
/*****************************************************************************/

#define ERR_NEW_ERROR(is_error, flags, subsys, code)    \
    ((err_t){.fields={is_error, 0, flags, subsys, code}})

%(definitions)s


#endif
"""

    subsystem_names = "\n".join(['  ERR_SUBSYS_%s,'%s for s in subsystems])
    definition_strings = get_definition_strings(subsystems, codes)
    code_strings = get_code_strings(subsystems, codes)
    short_strings = get_short_strings(subsystems, codes)

    fd = open('error_codes.h', 'w')
    fd.write(fmt%{
        'subsystem_names':subsystem_names,
        'definitions':'\n'.join(definition_strings),
        'error_codes':'\n'.join(code_strings)
        })

def get_registry_declarations(subsystems):
    n = len(subsystems) + 1
    short_string_registry = [
        "char **ERROR_SHORT_STRINGS[ERR_SUBSYS_INVALID_HIGH+1] = "
        "{",
        ]
    long_string_registry = [
        "char **ERROR_LONG_STRINGS[ERR_SUBSYS_INVALID_HIGH+1] = "
        "{",
        ]
    name_string_registry = [
        "char **ERROR_NAME_STRINGS[ERR_SUBSYS_INVALID_HIGH+1] = "
        "{",
        ]
    for s in subsystems:
        fmt = "    [ERR_SUBSYS_%s] = __ERROR_%s_STRINGS_%s,"
        short_string_registry.append(fmt%(s, 'SHORT', s))
        long_string_registry.append(fmt%(s, 'LONG', s))
        name_string_registry.append(fmt%(s, 'NAME', s))
    short_string_registry += [
        "    [ERR_SUBSYS_INVALID_HIGH] = NULL",
        "};"
    ]
    long_string_registry += [
        "    [ERR_SUBSYS_INVALID_HIGH] = NULL",
        "};"
    ]
    name_string_registry += [
        "    [ERR_SUBSYS_INVALID_HIGH] = NULL",
        "};"
    ]
    return (short_string_registry,
            long_string_registry,
            name_string_registry,)

def get_strings(subsystems, codes):
    short_strings = []
    long_strings = []
    name_strings = []
    for s in subsystems:
        max_code = None
        max_code = max([x[6] for x in codes[s]])
        fmt = "static char *__ERROR_%s_STRINGS_%s[%d] = "
        short_strings += [fmt%("SHORT", s, max_code+2), "{"]
        long_strings += [fmt%("LONG", s, max_code+2), "{"]
        name_strings += [fmt%("NAME", s, max_code+2), "{"]

        code_lst = codes[s]
        def __cmp(a, b):
            if a[-1] > b[-1]: return 1
            if a[-1] == b[-1]: return 0
            return -1
        code_lst.sort(cmp=__cmp)
        for c in code_lst:
            (ename, is_err, flags, code_str, short_str, long_str, v,) = c
            if 'synonym' in short_str.lower(): continue
            fmt = '    [%s] = "%s",'
            short_strings.append(fmt%(code_str, short_str))
            long_strings.append(fmt%(code_str, long_str))
            if ('UTIL') == s:
                name_strings.append(fmt%(code_str, "ERR_%s"%(ename)))
            else:
                name_strings.append(fmt%(code_str, "ERR_%s_%s"%(s, ename)))
        short_strings += ["    [%#8.8x] = NULL"%(max_code+1), "};", ""]
        long_strings += ["    [%#8.8x] = NULL"%(max_code+1), "};", ""]
        name_strings += ["    [%#8.8x] = NULL"%(max_code+1), "};", ""]
    return (short_strings, long_strings, name_strings,)

def write_source_file(subsystems, codes):
    fmt = """/*
 * Copyright (c) 2014 Harrison Caudill
 * All Rights Reserved
 * harrison@hypersphere.org
 */

#include <stdlib.h>
#include <errno.h>
#include "error_codes.h"

%(short_strings)s
%(short_string_registry)s

%(long_strings)s
%(long_string_registry)s

%(name_strings)s
%(name_string_registry)s

"""
    (short_string_registry,
     long_string_registry,
     name_string_registry,) = get_registry_declarations(subsystems)

    (short_strings,
     long_strings,
     name_strings,) = get_strings(subsystems, codes)
    fd = open('error_codes.c', 'w')
    fd.write(fmt%{
        'short_strings':'\n'.join(short_strings),
        'short_string_registry':'\n'.join(short_string_registry),
        'long_strings':'\n'.join(long_strings),
        'long_string_registry':'\n'.join(long_string_registry),
        'name_strings':'\n'.join(name_strings),
        'name_string_registry':'\n'.join(name_string_registry),
        })
    fd.close()


def main():
    subsystems = ['UTIL', 'ERRNO']
    codes = {
        'UTIL': util_codes(),
        'ERRNO': errno_codes()
        }
    write_header_file(subsystems, codes)
    write_source_file(subsystems, codes)

if __name__ == '__main__': main()
