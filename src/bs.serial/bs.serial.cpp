#include <regex>

#include "ext.h"
#include "ext_obex.h"
#include <filesystem>
#include "serial_reader.hpp"


struct t_bs_serial {
    t_object ob;
    boost::asio::io_context io;
    serial_reader *serial_reader;
    t_systhread thread;
    t_qelem *qelem;
    t_outlet *outlet;
    log_level log_level;
};

static std::vector<std::string> get_serial_devices(const std::string &pattern);

BEGIN_USING_C_LINKAGE
void *bs_serial_new(t_symbol *s, long argc, t_atom *argv);
void bs_serial_free(t_bs_serial *x);
void bs_serial_assist(t_bs_serial *x, void *b, long io, long index, char *s);
void bs_serial_qtask(t_bs_serial *x);

static t_class *s_bs_serial = nullptr;

void ext_main(void *) {
    t_class *c = class_new(
        "bs.serial",
        (method) bs_serial_new,
        (method) bs_serial_free,
        sizeof(t_bs_serial),
        (method) nullptr,
        A_GIMME,
        0);
    class_addmethod(c, (method) bs_serial_assist, "assist", A_CANT, 0);

    class_register(CLASS_BOX, c);
    s_bs_serial = c;
}

END_USING_C_LINKAGE

void *bs_serial_new(t_symbol *s, long argc, t_atom *argv) {
    std::string pattern = R"(/dev/tty\.usbmodem.*)";
    auto devices = get_serial_devices(pattern);
    if (devices.empty()) {
        error("No serial devices found matching the pattern %s", pattern.c_str());
        return nullptr;
    }
    auto *x = (t_bs_serial *)object_alloc(s_bs_serial);
    if (x) {
        new (&x->io) boost::asio::io_context();

        x->serial_reader = new serial_reader(x->io, devices[0]);
        x->qelem = qelem_new(x, (method) bs_serial_qtask);
        qelem_set(x->qelem);
        x->outlet = outlet_new(x, nullptr);
        x->log_level = log_level::info;
        systhread_create((method)+[](boost::asio::io_context *io) {
            io->run();
        }, &x->io, 0, 0, 0, &x->thread);
    }
    return x;
}

void bs_serial_free(t_bs_serial *x) {
    x->io.stop();
    unsigned int ret;
    systhread_join(x->thread, &ret);
    delete x->serial_reader;
    x->io.~io_context();
    qelem_free(x->qelem);
}

void bs_serial_assist(t_bs_serial *x, void *b, long io, long index, char *s) {
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

std::vector<std::string> get_serial_devices(const std::string &pattern) {
    std::vector<std::string> devices;
    std::regex re(pattern);
    for (const auto &entry : std::filesystem::directory_iterator("/dev")) {
        if (std::regex_match(entry.path().string(), re)) {
            devices.push_back(entry.path().string());
        }
    }
    return devices;
}

static void log(t_bs_serial *x, const log_message &msg) {
    if (x->log_level > msg.level) {
        return;
    }
    switch (msg.level) {
        case log_level::info:
            object_post((t_object *)x, "%s", msg.message.c_str());
            break;
        case log_level::warning:
            object_warn((t_object *)x, "%s", msg.message.c_str());
            break;
        case log_level::error:
            object_error((t_object *)x, "%s", msg.message.c_str());
            break;
    }
}

void bs_serial_qtask(t_bs_serial *x) {
    x->serial_reader->try_read([x](std::span<const uint8_t> data) {
        for (auto byte : data) {
            outlet_int(x->outlet, byte);
        }
        return data.size();
    });
    if (auto msg = x->serial_reader->get_log_message(); msg.has_value()) {
        log(x, *msg);
    }
    qelem_set(x->qelem);
}
