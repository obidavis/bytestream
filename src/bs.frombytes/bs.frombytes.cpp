#include "c74_max.h"

using namespace c74::max;

#include <ext_globalsymbol.h>

#include "ext.h"
#include "ext_obex.h"
#include "zpp_bits.h"
#include "atom_views.hpp"
#include "type_info.hpp"
#include "storage.hpp"
#include <ranges>
#include <sadam.stream.h>

#include "maxutils/attributes.hpp"

enum class Endianness : long {
    Big, Little, Network, Native
};

struct t_bs_frombytes {
    t_object ob;
    Endianness endianness;
    std::vector<t_outlet *> outlets;
    std::vector<storage> storages;
    t_object *stream;
};

BEGIN_USING_C_LINKAGE
void *bs_frombytes_new(t_symbol *s, long argc, t_atom *argv);
void bs_frombytes_free(t_bs_frombytes *x);
void bs_frombytes_assist(t_bs_frombytes *x, void *b, long io, long index, char *s);
void bs_frombytes_notify(t_bs_frombytes *x, t_symbol *s, t_symbol *msg, void *sender, void *data);
void bs_frombytes_int(t_bs_frombytes *x, long n);
void bs_frombytes_list(t_bs_frombytes *x, t_symbol *s, long argc, t_atom *argv);

static t_class *s_bs_frombytes = nullptr;

void ext_main(void *) {
    common_symbols_init();

    t_class *c = class_new(
        "bs.frombytes",
        (method) bs_frombytes_new,
        (method) bs_frombytes_free,
        sizeof(t_bs_frombytes),
        (method) nullptr,
        A_GIMME,
        0);

    class_addmethod(c, (method) bs_frombytes_assist, "assist", A_CANT, 0);
    class_addmethod(c, (method) bs_frombytes_notify, "notify", A_CANT, 0);
    class_addmethod(c, (method) bs_frombytes_int, "int", A_LONG, 0);
    class_addmethod(c, (method) bs_frombytes_list, "list", A_GIMME, 0);

    maxutils::create_attr<&t_bs_frombytes::endianness>(c);
    maxutils::create_attr(c, "stream",
        [](t_bs_frombytes *x) -> t_symbol * {
            t_symbol *name = _sym_none;
            if (x->stream) {
                object_method(x->stream, sadam::stream_getname, &name);
            }
            return name;
        },
        [](t_bs_frombytes *x, t_symbol *name) -> t_max_err {
            if (x->stream) {
                t_symbol *old_name;
                object_method(x->stream, sadam::stream_getname, &old_name);
                globalsymbol_dereference((t_object *) x, old_name->s_name, sadam::stream_classname->s_name);
            }
            x->stream = (t_object *)globalsymbol_reference((t_object *) x, name->s_name, sadam::stream_classname->s_name);
            return MAX_ERR_NONE;
        });

    class_register(CLASS_BOX, c);
    s_bs_frombytes = c;
}
END_USING_C_LINKAGE


void *bs_frombytes_new(t_symbol *, long argc, t_atom *argv) {
    auto [args, attrs] = get_args_and_attrs(argc, argv);
    if (args.empty()) {
        error("bs.frombytes: Expected at least one argument");
        return nullptr;
    }
    
    auto *x = (t_bs_frombytes *) object_alloc(s_bs_frombytes);
    if (!x) return nullptr;

    try {
        auto storages = args
            | std::views::transform(to_string)
            | std::views::transform([](const std::string &s) { return type_info(s); })
            | std::views::transform([](const type_info &ti) { return storage(ti); });
        x->storages = std::vector(storages.begin(), storages.end());

        for (size_t i = 0; i < x->storages.size(); i++) {
            x->outlets.push_back(outlet_new(x, nullptr));
        }
    } catch (const std::exception &e) {
        object_error((t_object *) x, e.what());
        return nullptr;
    }

    x->endianness = Endianness::Native;
    x->stream = nullptr;
    attr_args_process(x, attrs.size(), attrs.data());

    return x;
}

void bs_frombytes_free(t_bs_frombytes *x) {
    for (auto &outlet: x->outlets) {
        object_free(outlet);
    }
    x->outlets.~vector();
    x->storages.~vector();
}

void bs_frombytes_assist(t_bs_frombytes *x, void *b, long io, long index, char *s) {
    switch (io) {
        case 1:
            strncpy_zero(s, "serialised bytes", 512);
            break;
        case 2: {
            std::string type = x->storages[index].info().to_string();
            strncpy_zero(s, type.c_str(), 512);
            break;
        }
        default:
            break;
    }
}

void bs_frombytes_handle_data(t_bs_frombytes *x, std::span<uint8_t> data) {
    try {
        switch (x->endianness) {
            case Endianness::Big: {
                zpp::bits::in in(data, zpp::bits::endian::big{});
                in(zpp::bits::unsized(x->storages)).or_throw();
                break;
            }
            case Endianness::Little: {
                zpp::bits::in in(data, zpp::bits::endian::little{});
                in(zpp::bits::unsized(x->storages)).or_throw();
                break;
            }
            case Endianness::Network: {
                zpp::bits::in in(data, zpp::bits::endian::network{});
                in(zpp::bits::unsized(x->storages)).or_throw();
                break;
            }
            case Endianness::Native: {
                zpp::bits::in in(data, zpp::bits::endian::native{});
                in(zpp::bits::unsized(x->storages)).or_throw();
                break;
            }
        }
    } catch (const std::exception &e) {
        object_error((t_object *) x, e.what());
    }

    for (size_t i = x->storages.size(); i != 0; --i) {
        const auto &container = x->storages[i - 1];
        const auto &outlet = x->outlets[x->outlets.size() - i];
        std::vector<t_atom> atoms;
        atoms.reserve(container.size());
        container.store_to_atoms(std::back_inserter(atoms));
        outlet_list(outlet, nullptr, atoms.size(), atoms.data());
    }
}

void bs_frombytes_int(t_bs_frombytes *x, long n) {
    uint8_t byte = static_cast<uint8_t>(n);
    bs_frombytes_handle_data(x, std::span(&byte, 1));
}

void bs_frombytes_list(t_bs_frombytes *x, t_symbol *s, long argc, t_atom *argv) {
    try {
        std::span args(argv, argc);
        auto to_bytes = std::ranges::transform_view(args, [](const t_atom &a) -> uint8_t {
            if (a.a_type == A_LONG) {
                return static_cast<uint8_t>(a.a_w.w_long);
            }
            throw std::runtime_error("Expected integer");
        });
        std::vector data(to_bytes.begin(), to_bytes.end());
        bs_frombytes_handle_data(x, data);
    } catch (const std::exception &e) {
        object_error((t_object *) x, e.what());
    }
}

void bs_frombytes_notify(t_bs_frombytes *x, t_symbol *s, t_symbol *msg, void *sender, void *data) {
    if (msg == sadam::stream_binding) {
        x->stream = (t_object *)data;
    } else if (msg == sadam::stream_unbinding) {
        x->stream = nullptr;
    } else if (msg == sadam::stream_before_clear) {
        bs_frombytes_handle_data(x, *static_cast<std::vector<uint8_t> *>(data));
    }
}
