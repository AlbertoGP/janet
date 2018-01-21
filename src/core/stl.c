/*
* Copyright (c) 2017 Calvin Rose
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to
* deal in the Software without restriction, including without limitation the
* rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
* sell copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
* IN THE SOFTWARE.
*/

#include <dst/dst.h>
#include <dst/dststl.h>
#include <dst/dstasm.h>
#include <dst/dstopcodes.h>

int dst_stl_exit(DstArgs args) {
    int32_t exitcode = 0;
    if (args.n > 0) {
        exitcode = dst_hash(args.v[0]);
    }
    exit(exitcode);
    return 0;
}

int dst_stl_print(DstArgs args) {
    int32_t i;
    for (i = 0; i < args.n; ++i) {
        int32_t j, len;
        const uint8_t *vstr = dst_to_string(args.v[i]);
        len = dst_string_length(vstr);
        for (j = 0; j < len; ++j) {
            putc(vstr[j], stdout);
        }
    }
    putc('\n', stdout);
    return 0;
}

int dst_stl_describe(DstArgs args) {
    int32_t i;
    for (i = 0; i < args.n; ++i) {
        int32_t j, len;
        const uint8_t *vstr = dst_description(args.v[i]);
        len = dst_string_length(vstr);
        for (j = 0; j < len; ++j) {
            putc(vstr[j], stdout);
        }
    }
    putc('\n', stdout);
    return 0;
}

int dst_stl_string(DstArgs args) {
    int32_t i;
    DstBuffer b;
    dst_buffer_init(&b, 0);
    for (i = 0; i < args.n; ++i) {
        int32_t len;
        const uint8_t *str = dst_to_string(args.v[i]);
        len = dst_string_length(str);
        dst_buffer_push_bytes(&b, str, len);
    }
    *args.ret = dst_stringv(b.data, b.count);
    dst_buffer_deinit(&b);
    return 0;
}

int dst_stl_buffer_to_string(DstArgs args) {
    DstBuffer *b;
    if (args.n != 1) return dst_throw(args, "expected 1 argument");
    if (!dst_checktype(args.v[0], DST_BUFFER)) return dst_throw(args, "expected buffer");
    b = dst_unwrap_buffer(args.v[0]);
    return dst_return(args, dst_wrap_string(dst_string(b->data, b->count)));
}

int dst_cfun_tuple(DstArgs args) {
    return dst_return(args, dst_wrap_tuple(dst_tuple_n(args.v, args.n)));
}

int dst_cfun_array(DstArgs args) {
    DstArray *array = dst_array(args.n);
    array->count = args.n;
    memcpy(array->data, args.v, args.n * sizeof(Dst));
    return dst_return(args, dst_wrap_array(array));
}

int dst_cfun_table(DstArgs args) {
    int32_t i;
    DstTable *table = dst_table(args.n >> 1);
    if (args.n & 1) return dst_throw(args, "expected even number of arguments");
    for (i = 0; i < args.n; i += 2) {
        dst_table_put(table, args.v[i], args.v[i + 1]);
    }
    return dst_return(args, dst_wrap_table(table));
}

int dst_cfun_struct(DstArgs args) {
    int32_t i;
    DstKV *st = dst_struct_begin(args.n >> 1);
    if (args.n & 1) return dst_throw(args, "expected even number of arguments");
    for (i = 0; i < args.n; i += 2) {
        dst_struct_put(st, args.v[i], args.v[i + 1]);
    }
    return dst_return(args, dst_wrap_struct(dst_struct_end(st)));
}

int dst_stl_fiber(DstArgs args) {
    DstFiber *fiber;
    if (args.n < 1) return dst_throw(args, "expected at least one argument");
    if (!dst_checktype(args.v[0], DST_FUNCTION)) return dst_throw(args, "expected a function");
    fiber = dst_fiber(64);
    dst_fiber_funcframe(fiber, dst_unwrap_function(args.v[0]));
    fiber->parent = dst_vm_fiber;
    return dst_return(args, dst_wrap_fiber(fiber));
}

int dst_stl_buffer(DstArgs args) {
    DstBuffer *buffer = dst_buffer(10);
    int32_t i;
    for (i = 0; i < args.n; ++i) {
        const uint8_t *bytes = dst_to_string(args.v[i]);
        int32_t len = dst_string_length(bytes);
        dst_buffer_push_bytes(buffer, bytes, len);
    }
    return dst_return(args, dst_wrap_buffer(buffer));
}

int dst_stl_gensym(DstArgs args) {
    if (args.n > 1) return dst_throw(args, "expected one argument");
    if (args.n == 0) {
        return dst_return(args, dst_wrap_symbol(dst_symbol_gen(NULL, 0)));
    } else {
        const uint8_t *s = dst_to_string(args.v[0]);
        return dst_return(args, dst_wrap_symbol(dst_symbol_gen(s, dst_string_length(s))));
    }
}

int dst_stl_length(DstArgs args) {
    if (args.n != 1) return dst_throw(args, "expected at least 1 argument");
    return dst_return(args, dst_wrap_integer(dst_length(args.v[0])));
}

int dst_stl_get(DstArgs args) {
    int32_t i;
    Dst ds;
    if (args.n < 1) return dst_throw(args, "expected at least 1 argument");
    ds = args.v[0];
    for (i = 1; i < args.n; i++) {
        ds = dst_get(ds, args.v[i]);
        if (dst_checktype(ds, DST_NIL))
            break;
    }
    return dst_return(args, ds);
}

int dst_stl_status(DstArgs args) {
    const char *status;
    if (args.n != 1) return dst_throw(args, "expected 1 argument");
    if (!dst_checktype(args.v[0], DST_FIBER)) return dst_throw(args, "expected fiber");
    switch(dst_unwrap_fiber(args.v[0])->status) {
        case DST_FIBER_PENDING:
            status = "pending";
            break;
        case DST_FIBER_NEW:
            status = "new";
            break;
        case DST_FIBER_ALIVE:
            status = "alive";
            break;
        case DST_FIBER_DEAD:
            status = "dead";
            break;
        case DST_FIBER_ERROR:
            status = "error";
            break;
    }
    return dst_return(args, dst_cstringv(status));
}

int dst_stl_put(DstArgs args) {
    Dst ds, key, value;
    DstArgs subargs = args;
    if (args.n < 3) return dst_throw(args, "expected at least 3 arguments");
    subargs.n -= 2;
    if (dst_stl_get(subargs)) return 1;
    ds = *args.ret;
    key = args.v[args.n - 2];
    value = args.v[args.n - 1];
    dst_put(ds, key, value);
    return 0;
}

int dst_stl_gccollect(DstArgs args) {
    (void) args;
    dst_collect();
    return 0;
}

int dst_stl_type(DstArgs args) {
    if (args.n != 1) return dst_throw(args, "expected 1 argument");
    if (dst_checktype(args.v[0], DST_ABSTRACT)) {
        return dst_return(args, dst_cstringv(dst_abstract_type(dst_unwrap_abstract(args.v[0]))->name));
    } else {
        return dst_return(args, dst_cstringv(dst_type_names[dst_type(args.v[0])]));
    }
}

int dst_stl_next(DstArgs args) {
    Dst ds;
    const DstKV *kv;
    if (args.n != 2) return dst_throw(args, "expected 2 arguments");
    ds = args.v[0];
    if (dst_checktype(ds, DST_TABLE)) {
        DstTable *t = dst_unwrap_table(ds);
        kv = dst_checktype(args.v[1], DST_NIL)
            ? t->data
            : dst_table_find(t, args.v[1]);
    } else if (dst_checktype(ds, DST_STRUCT)) {
        const DstKV *st = dst_unwrap_struct(ds);
        kv = dst_checktype(args.v[1], DST_NIL)
            ? st
            : dst_struct_find(st, args.v[1]);
    } else {
        return dst_throw(args, "expected table/struct");
    }
    kv = dst_next(ds, kv);
    if (kv) {
        return dst_return(args, kv->key);
    }
    return dst_return(args, dst_wrap_nil());
}

int dst_stl_hash(DstArgs args) {
    if (args.n != 1) return dst_throw(args, "expected 1 argument");
    return dst_return(args, dst_wrap_integer(dst_hash(args.v[0])));
}

static const DstReg cfuns[] = {
    {"native", dst_load_native},
    {"print", dst_stl_print},
    {"describe", dst_stl_describe},
    {"string", dst_stl_string},
    {"buffer-string", dst_stl_buffer_to_string},
    {"table", dst_cfun_table},
    {"array", dst_cfun_array},
    {"tuple", dst_cfun_tuple},
    {"struct", dst_cfun_struct},
    {"fiber", dst_stl_fiber},
    {"status", dst_stl_status},
    {"buffer", dst_stl_buffer},
    {"gensym", dst_stl_gensym},
    {"get", dst_stl_get},
    {"put", dst_stl_put},
    {"length", dst_stl_length},
    {"gccollect", dst_stl_gccollect},
    {"type", dst_stl_type},
    {"next", dst_stl_next},
    {"hash", dst_stl_hash},
    {"exit", dst_stl_exit},
    {"asm", dst_asm_cfun},
    {"disasm", dst_disasm_cfun},
    {NULL, NULL}
};

DstTable *dst_stl_env() {
    static uint32_t error_asm[] = {
        DOP_ERROR
    };

    static uint32_t apply_asm[] = {
       DOP_PUSH_ARRAY | (1 << 8),
       DOP_TAILCALL
    };

    DstTable *env = dst_table(0);
    Dst ret = dst_wrap_table(env);

    /* Load main functions */
    dst_env_cfuns(env, cfuns);

    dst_env_def(env, "error", dst_wrap_function(dst_quick_asm(1, 0, 1, error_asm, sizeof(uint32_t))));
    dst_env_def(env, "apply", dst_wrap_function(dst_quick_asm(2, 0, 2, apply_asm, 2 * sizeof(uint32_t))));

    /* Allow references to the environment */
    dst_env_def(env, "_env", ret);

    /* Load auxiliary envs */
    {
        DstArgs args;
        args.n = 1;
        args.v = &ret;
        args.ret = &ret;
        dst_lib_io(args);
        dst_lib_math(args);
        dst_lib_array(args);
        dst_lib_buffer(args);
    }

    return env;
}
