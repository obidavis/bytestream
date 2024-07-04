#include <string>

#include "ext.h"
#include "ext_obex.h"
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/buffer.h>
#include <openssl/bio.h>
#include <span>
#include <sstream>
#include <iomanip>
#include <string>

struct t_bs_hash {
    t_object ob;
    enum class HashAlgorithm : long {
        SHA1, SHA256, SHA512
    } hash_algorithm;
    enum class OutputMode : long {
        HexString, Base64String, RawBytes
    } output_mode;
    long output_length;

    t_outlet *outlet;
};

extern "C"
{

void *bs_hash_new(t_symbol *s, long argc, t_atom *argv);
void bs_hash_free(t_bs_hash *x);
void bs_hash_assist(t_bs_hash *x, void *b, long m, long a, char *s);
void bs_hash_anything(t_bs_hash *x, t_symbol *s, long argc, t_atom *argv);
void bs_hash_list(t_bs_hash *x, t_symbol *s, long argc, t_atom *argv);
void bs_hash_int(t_bs_hash *x, long n);

static t_class *s_bs_hash = nullptr;

void ext_main(void *) {
    t_class *c = class_new(
        "bs.hash",
        (method) bs_hash_new,
        (method) bs_hash_free,
        sizeof(t_bs_hash),
        (method) nullptr,
        A_GIMME,
        0);
    class_addmethod(c, (method) bs_hash_anything, "anything", A_GIMME, 0);
    class_addmethod(c, (method) bs_hash_list, "list", A_GIMME, 0);
    class_addmethod(c, (method) bs_hash_int, "int", A_LONG, 0);
    class_addmethod(c, (method) bs_hash_assist, "assist", A_CANT, 0);

    CLASS_ATTR_LONG(c, "algo", 0, t_bs_hash, hash_algorithm);
    CLASS_ATTR_ENUMINDEX(c, "algo", 0, "SHA1 SHA256 SHA512");

    CLASS_ATTR_LONG(c, "output_mode", 0, t_bs_hash, output_mode);
    CLASS_ATTR_ENUMINDEX(c, "output_mode", 0, "HexString Base64String RawBytes");

    CLASS_ATTR_LONG(c, "output_length", 0, t_bs_hash, output_length);

    class_register(CLASS_BOX, c);
    s_bs_hash = c;
}

} // extern "C"

void *bs_hash_new(t_symbol *s, long argc, t_atom *argv) {
    auto *x = (t_bs_hash *)object_alloc(s_bs_hash);
    if (x) {
        x->outlet = outlet_new(x, nullptr);
        x->hash_algorithm = t_bs_hash::HashAlgorithm::SHA256;
        x->output_mode = t_bs_hash::OutputMode::HexString;
        x->output_length = -1;
        attr_args_process(x, argc, argv);
    }
    return x;
}

void bs_hash_free(t_bs_hash *x) {
    object_free(x->outlet);
}

void bs_hash_assist(t_bs_hash *x, void *b, long io, long index, char *s) {
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

void bs_hash_anything(t_bs_hash *x, t_symbol *s, long argc, t_atom *argv) {
    if (argc) {
        object_warn((t_object *)x, "ignoring extra arguments: only \"%s\" will be hashed", s->s_name);
    }

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    const EVP_MD *md = [algorithm = x->hash_algorithm] {
        switch (algorithm) {
            case t_bs_hash::HashAlgorithm::SHA1:
                return EVP_sha1();
            case t_bs_hash::HashAlgorithm::SHA256:
                return EVP_sha256();
            case t_bs_hash::HashAlgorithm::SHA512:
                return EVP_sha512();
            default:
                __builtin_unreachable();
        }
    }();

    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len;

    EVP_DigestInit_ex(mdctx, md, nullptr);
    EVP_DigestUpdate(mdctx, s->s_name, strlen(s->s_name));
    EVP_DigestFinal_ex(mdctx, md_value, &md_len);
    EVP_MD_CTX_free(mdctx);

    size_t len = x->output_length == -1 ? md_len : std::min<size_t>(x->output_length, md_len);
    std::span output(md_value, len);

    switch (x->output_mode) {
        case t_bs_hash::OutputMode::HexString: {
            std::ostringstream oss;
            for (auto byte: output) {
                oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
            }
            std::string hex_string = oss.str();
            outlet_anything(x->outlet, gensym(hex_string.c_str()), 0, nullptr);
            break;
        }
        case t_bs_hash::OutputMode::Base64String: {
            BIO *bio = BIO_new(BIO_s_mem());
            BIO *b64 = BIO_new(BIO_f_base64());
            BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
            bio = BIO_push(b64, bio);
            BIO_write(bio, output.data(), output.size());
            BIO_flush(bio);
            BUF_MEM *bptr;
            BIO_get_mem_ptr(bio, &bptr);
            std::string base64_string(bptr->data, bptr->length);
            BIO_free_all(bio);
            outlet_anything(x->outlet, gensym(base64_string.c_str()), 0, nullptr);
            break;
        }
        case t_bs_hash::OutputMode::RawBytes: {
            std::vector<t_atom> atoms;
            atoms.reserve(output.size());
            for (auto byte: output) {
                atoms.push_back(t_atom{A_LONG, static_cast<long>(byte)});
            }
            outlet_list(x->outlet, nullptr, atoms.size(), atoms.data());
            break;
        }
        default:
            __builtin_unreachable();
    }

}

void bs_hash_list(t_bs_hash *x, t_symbol *s, long argc, t_atom *argv) {
    // list method implementation
}

void bs_hash_int(t_bs_hash *x, long n) {
    // int method implementation
}
