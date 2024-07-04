#include <vector>

#include "ext.h"
#include "ext_obex.h"

struct t_bs_midibitpack {
    t_object ob;
    t_outlet *outlet;
    long chunk_size;
};

extern "C"
{

void *bs_midibitpack_new(t_symbol *s, long argc, t_atom *argv);
void bs_midibitpack_free(t_bs_midibitpack *x);
void bs_midibitpack_assist(t_bs_midibitpack *x, void *b, long m, long a, char *s);
void bs_midibitpack_list(t_bs_midibitpack *x, t_symbol *s, long argc, t_atom *argv);
void bs_midibitpack_int(t_bs_midibitpack *x, long n);

static t_class *s_bs_midibitpack = nullptr;

void ext_main(void *) {
    t_class *c = class_new(
        "bs.midibitpack",
        (method) bs_midibitpack_new,
        (method) bs_midibitpack_free,
        sizeof(t_bs_midibitpack),
        (method) nullptr,
        A_GIMME,
        0);
    class_addmethod(c, (method) bs_midibitpack_list, "list", A_GIMME, 0);
    class_addmethod(c, (method) bs_midibitpack_int, "int", A_LONG, 0);
    class_addmethod(c, (method) bs_midibitpack_assist, "assist", A_CANT, 0);

    class_register(CLASS_BOX, c);
    s_bs_midibitpack = c;
}

} // extern "C"

void *bs_midibitpack_new(t_symbol *s, long argc, t_atom *argv) {
    auto *x = (t_bs_midibitpack *)object_alloc(s_bs_midibitpack);
    if (x) {
        x->outlet = listout(x);
        x->chunk_size = 3;
    }
    return x;
}

void bs_midibitpack_free(t_bs_midibitpack *x) {
    object_free(x->outlet);
}

void bs_midibitpack_assist(t_bs_midibitpack *x, void *b, long io, long index, char *s) {
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

void bs_midibitpack_list(t_bs_midibitpack *x, t_symbol *s, long argc, t_atom *argv) {
    if (argc % x->chunk_size != 0) {
        object_error((t_object *)x, "list length must be a multiple of chunk size");
        return;
    }
    std::vector<t_atom> atoms;
    size_t output_chunk_size = (x->chunk_size * 8 + (7 - 1)) / 7;
    atoms.reserve(output_chunk_size * argc);
    for (int i = 0; i < argc; i += x->chunk_size) {

    }
}

void bs_midibitpack_int(t_bs_midibitpack *x, long n) {
    // int method implementation
}
