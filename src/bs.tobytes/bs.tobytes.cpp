#include "c74_max.h"
using namespace c74::max;
#include "ext.h"
#include "ext_obex.h"
#include "ext_globalsymbol.h"
#include "jit.common.h"
#include "zpp_bits.h"
#include "type_info.hpp"
#include "storage.hpp"
#include "atom_views.hpp"
#include "sadam.stream.h"
#include <ranges>

#include "maxutils/attributes.hpp"


struct t_bs_tobytes {
    t_object ob;

    long proxy_id;

    static constexpr size_t max_args = 256;
    long triggers[max_args];
    size_t num_args;

    enum class Endianness { Big, Little, Network, Native } endianness;

    t_outlet *outlet;
    std::vector<void *> proxies;
    std::vector<storage> storages;

    t_object *stream;
};

static void bs_tobytes_handle_data(t_bs_tobytes *x, long index, auto data);

BEGIN_USING_C_LINKAGE
void *bs_tobytes_new(t_symbol *s, long argc, t_atom *argv);
void bs_tobytes_free(t_bs_tobytes *x);
void bs_tobytes_assist(t_bs_tobytes *x, void *b, long io, long index, char *s);
void bs_tobytes_inletinfo(t_bs_tobytes *x, void *b, long index, char *t);
void bs_tobytes_notify(t_bs_tobytes *x, t_symbol *s, t_symbol *msg, void *sender, void *data);
void bs_tobytes_bang(t_bs_tobytes *x);
void bs_tobytes_int(t_bs_tobytes *x, long n);
void bs_tobytes_float(t_bs_tobytes *x, double f);
void bs_tobytes_list(t_bs_tobytes *x, t_symbol *s, long argc, t_atom *argv);
void bs_tobytes_jit_matrix(t_bs_tobytes *x, t_symbol *s, long argc, t_atom *argv);

static t_class *s_bs_tobytes = nullptr;

void ext_main(void *) {
    t_class *c = class_new(
        "bs.tobytes",
        (method) bs_tobytes_new,
        (method) bs_tobytes_free,
        sizeof(t_bs_tobytes),
        (method) nullptr,
        A_GIMME,
        0);

    class_addmethod(c, (method) bs_tobytes_assist, "assist", A_CANT, 0);
    class_addmethod(c, (method) bs_tobytes_inletinfo, "inletinfo", A_CANT, 0);
    class_addmethod(c, (method) bs_tobytes_bang, "bang", 0);
    class_addmethod(c, (method) bs_tobytes_int, "int", A_LONG, 0);
    class_addmethod(c, (method) bs_tobytes_float, "float", A_FLOAT, 0);
    class_addmethod(c, (method) bs_tobytes_list, "list", A_GIMME, 0);
    class_addmethod(c, (method) bs_tobytes_jit_matrix, "jit_matrix", A_GIMME, 0);
    class_addmethod(c, (method) bs_tobytes_notify, "notify", A_CANT, 0);

    CLASS_ATTR_LONG_VARSIZE(c, "triggers", 0, t_bs_tobytes, triggers, num_args, t_bs_tobytes::max_args);

    maxutils::create_attr<&t_bs_tobytes::endianness>(c);
    maxutils::create_attr(c, "stream",
        [](t_bs_tobytes *x) -> t_symbol * {
            t_symbol *name = nullptr;
            if (x->stream) {
                object_method(x->stream, sadam::stream_getname, &name);
            }
            return name;
        },
        [](t_bs_tobytes *x, t_symbol *name) -> t_max_err {
            if (x->stream) {
                t_symbol *old_name;
                object_method(x->stream, sadam::stream_getname, &old_name);
                globalsymbol_dereference((t_object *)x, old_name->s_name, sadam::stream_classname->s_name);
            }
            x->stream = (t_object *)globalsymbol_reference((t_object *)x, name->s_name, sadam::stream_classname->s_name);
            return MAX_ERR_NONE;
        });

    class_register(CLASS_BOX, c);
    s_bs_tobytes = c;
}
END_USING_C_LINKAGE

void bs_tobytes_inletinfo(t_bs_tobytes *x, void *b, long index, char *t) {
    std::span active_triggers(x->triggers, x->num_args);
    bool cold = std::ranges::find(active_triggers, index) == active_triggers.end()
                && std::ranges::find(active_triggers, -1) == active_triggers.end();
    if (cold) *t = 1;
}

void *bs_tobytes_new(t_symbol *, long argc, t_atom *argv) {
    auto [args, attrs] = get_args_and_attrs(argc, argv);
    if (args.empty()) {
        error("bs.tobytes: expected at least one argument");
        return nullptr;
    }

    auto *x = (t_bs_tobytes *) object_alloc(s_bs_tobytes);
    if (!x) {
        return nullptr;
    }

    try {
        std::ranges::transform(args, std::back_inserter(x->storages), [](const t_atom &a) {
            const type_info ti(to_string(a));
            return storage(ti);
        });

        for (size_t i = x->storages.size() - 1; i > 0; --i) {
            void *proxy = proxy_new(x, (long)i, &x->proxy_id);
            x->proxies.push_back(proxy);
        }

    } catch (const std::exception &e) {
        object_error((t_object *) x, e.what());
        object_free(x);
        return nullptr;
    }

    x->triggers[0] = 0;
    x->num_args = 1;
    x->endianness = t_bs_tobytes::Endianness::Native;

    x->outlet = listout(x);
    x->stream = nullptr;
    attr_args_process(x, (short)attrs.size(), attrs.data());
    return x;
}

void bs_tobytes_free(t_bs_tobytes *x) {
    object_free(x->outlet);
    for (auto &p: x->proxies) {
        proxy_delete(p);
    }
    x->proxies.~vector();
    x->storages.~vector();
    if (x->stream) {
        t_symbol *name;
        object_method(x->stream, sadam::stream_getname, &name);
        globalsymbol_dereference((t_object *)x, name->s_name, sadam::stream_classname->s_name);
    }
}

void bs_tobytes_assist(t_bs_tobytes *x, void *b, long io, long index, char *s) {
    switch (io) {
        case 1: {
            std::string type = x->storages[index].info().to_string();
            strncpy_zero(s, type.c_str(), 512);
            break;
        }
        case 2:
            strncpy_zero(s, "serialised bytes", 512);
            break;
        default:
            break;
    }
}

void bs_tobytes_handle_data(t_bs_tobytes *x, long index, auto data) {
    x->storages[index].load(data);

    char cold = 0;
    bs_tobytes_inletinfo(x, nullptr, index, &cold);
    if (!cold) {
        bs_tobytes_bang(x);
    }
}

void bs_tobytes_bang(t_bs_tobytes *x) {
    std::vector<uint8_t> out_bytes;
    switch (x->endianness) {
        case t_bs_tobytes::Endianness::Big: {
            zpp::bits::out{out_bytes, zpp::bits::endian::big{}}(zpp::bits::unsized(x->storages)).or_throw();
            break;
        }
        case t_bs_tobytes::Endianness::Little: {
            zpp::bits::out{out_bytes, zpp::bits::endian::little{}}(zpp::bits::unsized(x->storages)).or_throw();
            break;
        }
        case t_bs_tobytes::Endianness::Network: {
            zpp::bits::out{out_bytes, zpp::bits::endian::network{}}(zpp::bits::unsized(x->storages)).or_throw();
            break;
        }
        case t_bs_tobytes::Endianness::Native: {
            zpp::bits::out{out_bytes}(zpp::bits::unsized(x->storages)).or_throw();
            break;
        }
    }

    if (x->stream) {
        object_method(x->stream, sadam::stream_addarray, &out_bytes);
        object_method(x->stream, sadam::stream_clear);
    } else {
        std::vector<t_atom> out_atoms;
        out_atoms.reserve(out_bytes.size());
        std::ranges::transform(out_bytes, std::back_inserter(out_atoms), [](const uint8_t b) -> t_atom {
            return { .a_type = A_LONG, .a_w.w_long = b };
        });
        if (out_atoms.size() > std::numeric_limits<short>::max()) {
            object_warn((t_object *) x, "Output list too long");
        }
        outlet_list(x->outlet, nullptr, static_cast<short>(out_atoms.size()), out_atoms.data());
    }
}

void bs_tobytes_int(t_bs_tobytes *x, long n) {
    try {
        bs_tobytes_handle_data(x, proxy_getinlet((t_object *) x), n);
    } catch (const std::exception &e) {
        object_error((t_object *) x, e.what());
    }
}

void bs_tobytes_float(t_bs_tobytes *x, double f) {
    try {
        bs_tobytes_handle_data(x, proxy_getinlet((t_object *) x), f);
    } catch (const std::exception &e) {
        object_error((t_object *) x, e.what());
    }
}

void bs_tobytes_list(t_bs_tobytes *x, t_symbol *s, long argc, t_atom *argv) {
    try {
        bs_tobytes_handle_data(x, proxy_getinlet((t_object *) x), std::span(argv, argc));
    } catch (const std::exception &e) {
        object_error((t_object *) x, e.what());
    }
}

void bs_tobytes_notify(t_bs_tobytes *x, t_symbol *s, t_symbol *msg, void *sender, void *data) {
    if (msg == sadam::stream_binding) {
        x->stream = (t_object *)data;
    } else if (msg == sadam::stream_unbinding) {
        x->stream = nullptr;
    }
}

void bs_tobytes_jit_matrix(t_bs_tobytes *x, t_symbol *s, long argc, t_atom *argv) {
    t_symbol *name = atom_getsym(argv);
    t_jit_object *matrix = (t_jit_object *)jit_object_findregistered(name);
    if (matrix) {
        try {
            bs_tobytes_handle_data(x, proxy_getinlet((t_object *)x ), matrix);
        } catch (const std::exception &e) {
            object_error((t_object *)x, e.what());
        }
    }
    else {
        object_error((t_object *)x, "Couldn't find matrix %s", name->s_name);
    }
}


