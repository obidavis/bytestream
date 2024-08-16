
#include "c74_max.h"
#include "ext.h"
#include "ext_obex.h"
#include "bytestream/COBS.hpp"
#include "bytestream/SLIP.hpp"
#include "maxutils/attributes.hpp"
#include <span>
#include <vector>

using namespace c74::max;

struct t_bs_decodeframe {
    t_object ob;
    enum class Mode { COBS, SLIP } mode;
    t_outlet *out;
    SLIPDecoder slip_decoder;
    COBSDecoder cobs_decoder;
    std::vector<t_atom> buffer;
};

extern "C"
{

void *bs_decodeframe_new(t_symbol *s, long argc, t_atom *argv);
void bs_decodeframe_free(t_bs_decodeframe *x);
void bs_decodeframe_assist(t_bs_decodeframe *x, void *b, long m, long a, char *s);
void bs_decodeframe_int(t_bs_decodeframe *x, long n);
void bs_decodeframe_list(t_bs_decodeframe *x, t_symbol *s, long argc, t_atom *argv);

static t_class *s_bs_decodeframe = nullptr;

void ext_main(void *) {
    t_class *c = class_new(
        "bs.decodeframe",
        (method) bs_decodeframe_new,
        (method) bs_decodeframe_free,
        sizeof(t_bs_decodeframe),
        (method) nullptr,
        A_GIMME,
        0);
    class_addmethod(c, (method) bs_decodeframe_int, "int", A_LONG, 0);
    class_addmethod(c, (method) bs_decodeframe_list, "list", A_GIMME, 0);
    class_addmethod(c, (method) bs_decodeframe_assist, "assist", A_CANT, 0);

    maxutils::create_attr<&t_bs_decodeframe::mode>(c);

    class_register(CLASS_BOX, c);
    s_bs_decodeframe = c;
}

} // extern "C"

void *bs_decodeframe_new(t_symbol *s, long argc, t_atom *argv) {
    auto *x = (t_bs_decodeframe *)object_alloc(s_bs_decodeframe);
    if (x) {
        // Constructor implementation
        x->mode = t_bs_decodeframe::Mode::COBS;
        x->cobs_decoder = {};
        x->slip_decoder = {};
        x->out = listout(x);
    }
    return x;
}

void bs_decodeframe_free(t_bs_decodeframe *x) {
    object_free(x->out);
}

void bs_decodeframe_assist(t_bs_decodeframe *x, void *b, long io, long index, char *s) {
        switch (io) {
        case 1:
            switch (index) {
                case 0:
                    strncpy_zero(s, "This is a description of the leftmost inlet", 512);
                    break;
                case 1:
                    strncpy_zero(s, "This is a description of the rightmost inlet", 512);
                    break;
            }
            break;
        case 2:
            strncpy_zero(s, "This is a description of the outlet", 512);
            break;
        default:
            break;
    }
}

void bs_decodeframe_int(t_bs_decodeframe *x, long n) {
    if (n < 0 || n > 255) {
        object_warn((t_object *) x, "Value %d out of range - clamp to 0-255", n);
    }
    const auto byte = static_cast<uint8_t>(n);

    auto process_byte = [x, byte](auto &decoder) {
        uint8_t decoded_byte;
        if (decoder.process_byte(byte, &decoded_byte)) {
            t_atom atom;
            atom_setlong(&atom, decoded_byte);
            x->buffer.push_back(atom);
        }
        if (decoder.packet_complete()) {
            outlet_list(x->out, nullptr, x->buffer.size(), x->buffer.data());
            x->buffer.clear();
            decoder.reset();
        }
    };

    switch (x->mode) {
        case t_bs_decodeframe::Mode::SLIP:
            process_byte(x->slip_decoder);
            break;
        case t_bs_decodeframe::Mode::COBS:
            process_byte(x->cobs_decoder);
            break;
    }
}

void bs_decodeframe_list(t_bs_decodeframe *x, t_symbol *s, long argc, t_atom *argv) {
    std::span<const t_atom> args(argv, argc);
    for (const t_atom &atom : args) {
        if (atom.a_type != A_LONG) {
            object_error((t_object *) x, "Expected list of integers");
            return;
        }
        bs_decodeframe_int(x, atom_getlong(&atom));
    }
}
