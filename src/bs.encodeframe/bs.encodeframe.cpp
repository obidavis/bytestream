#include <numeric>

#include "ext.h"
#include "ext_obex.h"
#include "bytestream/COBS.hpp"
#include "bytestream/SLIP.hpp"
#include "maxutils/attributes.hpp"
#include <span>
#include <vector>
#include <ranges>

struct t_bs_encodeframe {
    t_object ob;
    enum Mode { COBS, SLIP } mode;
    t_outlet *out;
    t_symbol *stream_name;
};

static void bs_encodeframe_

extern "C" {
// Basic Methods
void *bs_encodeframe_new(t_symbol *s, long argc, t_atom *argv);

void bs_encodeframe_free(t_bs_encodeframe *x);

void bs_encodeframe_assist(t_bs_encodeframe *x, void *b, long m, char *s);

// Messages
void bs_encodeframe_int(t_bs_encodeframe *x, long n);
void bs_encodeframe_list(t_bs_encodeframe *x, t_symbol *s, long argc, t_atom *argv);

// Global class instance
static t_class *s_bs_encodeframe;

void ext_main(void *r) {
    common_symbols_init();

    t_class *c = class_new(
        "bs.encodeframe",
        (method) bs_encodeframe_new,
        (method) bs_encodeframe_free,
        (short) sizeof(t_bs_encodeframe),
        nullptr,
        A_GIMME,
        0);

    class_addmethod(c, (method) bs_encodeframe_assist, "assist", A_CANT, 0);
    class_addmethod(c, (method) bs_encodeframe_int, "int", A_LONG, 0);
    class_addmethod(c, (method) bs_encodeframe_list, "list", A_GIMME, 0);

    maxutils::create_attr<&t_bs_encodeframe::mode>(c);

    class_register(CLASS_BOX, c);
    s_bs_encodeframe = c;
}

} // extern "C"

void *bs_encodeframe_new(t_symbol *s, long argc, t_atom *argv) {
    auto *x = (t_bs_encodeframe *) object_alloc(s_bs_encodeframe);
    if (x) {
        x->mode = t_bs_encodeframe::Mode::COBS;
        x->out = listout(x);
        attr_args_process(x, argc, argv);
    }
    return x;
}

void bs_encodeframe_free(t_bs_encodeframe *x) {
    object_free(x->out);
}

void bs_encodeframe_assist(t_bs_encodeframe *x, void *b, long m, char *s) {
    /* assist implementation */
}

void bs_encodeframe_bang(t_bs_encodeframe *x) {
    /* bang message implementation */
}

void bs_encodeframe_list(t_bs_encodeframe *x, t_symbol *, long argc, t_atom *argv) {
    if (!argc) return;

    std::span<const t_atom> args(argv, argc);
    if (!std::ranges::all_of(args, [](const t_atom &atom) { return atom_gettype(&atom) == A_LONG; })) {
        object_error((t_object *) x, "Expected list of integers");
        return;
    }

    auto as_bytes = std::ranges::views::transform(args, [x](const t_atom atom) {
        const long value = atom_getlong(&atom);
        if (value < 0 || value > 255) {
            object_warn((t_object *) x, "Value %d out of range for byte, clamping to 0-255", value);
        }
        return static_cast<uint8_t>(value);
    });
    std::vector bytes(as_bytes.begin(), as_bytes.end());

    std::vector<uint8_t> encoded;

    switch (x->mode) {
        case t_bs_encodeframe::Mode::SLIP: {
            encoded.resize(slip_encoded_max_length(bytes.size()));
            auto encoded_end = slip_encode_frame(bytes.begin(), bytes.end(), encoded.begin());
            encoded.resize(std::distance(encoded.begin(), encoded_end));
            break;
        }
        case t_bs_encodeframe::Mode::COBS: {
            encoded.resize(cobs_encoded_max_length(bytes.size()));
            auto encoded_end = cobs_encode_frame(bytes.begin(), bytes.end(), encoded.begin());
            encoded.resize(std::distance(encoded.begin(), encoded_end));
            break;
        }
    }

    if (encoded.empty()) {
        object_error((t_object *) x, "Failed to encode frame");
        return;
    }

    auto as_atoms = std::ranges::views::transform(encoded, [](const uint8_t byte) -> t_atom {
        return { .a_type = A_LONG, .a_w.w_long = byte };
    });
    std::vector atoms_out(as_atoms.begin(), as_atoms.end());
    outlet_list(x->out, nullptr, atoms_out.size(), atoms_out.data());
}

void bs_encodeframe_int(t_bs_encodeframe *x, long n) {
    t_atom atom;
    atom_setlong(&atom, n);
    bs_encodeframe_list(x, _sym_list, 1, &atom);
}
