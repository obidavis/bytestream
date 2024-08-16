#include "c74_max.h"
using namespace c74::max;

#include <ext_globalsymbol.h>
#include <regex>
#include <filesystem>

#include "ext.h"
#include "ext_obex.h"

#include "maxutils/attributes.hpp"

#include "serial_context.hpp"
#include "serial_port.hpp"

#include "sadam.stream.h"

struct device_info {
    std::string name;
    std::string path;
};

struct t_bs_serial {
    t_object ob;
    std::shared_ptr<serial_port> port;
    device_info device;
    t_clock *clock;
    t_outlet *outlet;
    log_level log_level;
    double poll_interval;
    bool match_exact;
    t_object *stream;
};

static std::vector<device_info> get_serial_devices(const std::string &pattern);

static void log(t_bs_serial *x, const log_message &msg);

BEGIN_USING_C_LINKAGE
void *bs_serial_new(t_symbol *s, long argc, t_atom *argv);
void bs_serial_free(t_bs_serial *x);
void bs_serial_notify(t_bs_serial *x, t_symbol *s, t_symbol *msg, void *sender, void *data);
t_max_err bs_serial_open(t_bs_serial *x, t_symbol *s);
void bs_serial_close(t_bs_serial *x);
void bs_serial_poll_rx_task(t_bs_serial *x);

static t_class *s_bs_serial = nullptr;
static boost::asio::io_context *bs_serial_thread_io_context = nullptr;

void ext_main(void *) {
    t_class *c = class_new(
        "bs.serial",
        (method) bs_serial_new,
        (method) bs_serial_free,
        sizeof(t_bs_serial),
        (method) nullptr,
        A_GIMME,
        0);

    class_addmethod(c, (method) bs_serial_open, "open", A_SYM, 0);
    class_addmethod(c, (method) bs_serial_close, "close", 0);
    class_addmethod(c, (method) bs_serial_notify, "notify", A_CANT, 0);

    maxutils::create_attr<&t_bs_serial::log_level>(c);
    maxutils::create_attr(c, "port",
        [](t_bs_serial *x) -> t_symbol * {
            return gensym(x->device.name.c_str());
        },
        [](t_bs_serial *x, t_symbol *s) -> t_max_err  {
            return bs_serial_open(x, s);
        });
    maxutils::create_attr<&t_bs_serial::poll_interval>(c);
    maxutils::create_attr(c, "stream",
        [](t_bs_serial *x) -> t_symbol * {
            t_symbol *name = _sym_none;
            if (x->stream) {
                object_method(x->stream, sadam::stream_getname, &name);
            }
            return name;
        },
        [](t_bs_serial *x, t_symbol *name) -> t_max_err {
            if (x->stream) {
                t_symbol *old_name;
                object_method(x->stream, sadam::stream_getname, &old_name);
                globalsymbol_dereference((t_object *) x, old_name->s_name, sadam::stream_classname->s_name);
            }
            x->stream = (t_object *)globalsymbol_reference((t_object *) x, name->s_name, sadam::stream_classname->s_name);
            return MAX_ERR_NONE;
        });

    class_register(CLASS_BOX, c);
    s_bs_serial = c;

    bs_serial_thread_io_context = serial_context_init();
}

END_USING_C_LINKAGE

void *bs_serial_new(t_symbol *s, long argc, t_atom *argv) {
    auto *x = (t_bs_serial *)object_alloc(s_bs_serial);
    if (x) {
        x->poll_interval = 2.;
        x->port = std::make_shared<serial_port>(*bs_serial_thread_io_context);

        // x->poll_rx_qelem = qelem_new(x, (method) bs_serial_poll_rx_task);
        x->outlet = outlet_new(x, nullptr);
        x->log_level = log_level::info;
        attr_args_process(x, argc, argv);
        x->clock = clock_new(x, (method) bs_serial_poll_rx_task);
        clock_fdelay(x->clock, x->poll_interval);
        if (x->stream != nullptr) {

        }
    }
    return x;
}

void bs_serial_free(t_bs_serial *x) {
    x->port.reset();
    x->port.~shared_ptr();
    object_free(x->outlet);
    clock_free(x->clock);
}


std::vector<device_info> get_serial_devices(const std::string &pattern) {
    std::vector<device_info> devices;
    std::regex re(pattern);
    for (const auto &entry : std::filesystem::directory_iterator("/dev")) {
        if (std::regex_search(entry.path().filename().string(), re)) {
            devices.push_back({
                .name = entry.path().filename().string(),
                .path = entry.path().string()
            });
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

t_max_err bs_serial_open(t_bs_serial *x, t_symbol *s) {
    std::vector<device_info> devices = get_serial_devices(s->s_name);
    if (devices.empty()) {
        object_error((t_object *)x, "No devices found matching pattern %s", s->s_name);
        return MAX_ERR_GENERIC;
    }
    x->device = devices.front();
    x->port->open(x->device.path);
    // object_post((t_object *)x, "Opened %s", x->device.name.c_str());
    return MAX_ERR_NONE;
}

void bs_serial_close(t_bs_serial *x) {
    x->port->close();
}

void bs_serial_poll_rx_task(t_bs_serial *x) {
    x->port->try_consume_from_rx_queue([x](std::span<const uint8_t> data) {

        if (data.empty()) {
            return;
        }

        object_post((t_object *)x, "Task: Received %zu bytes", data.size());

        if (x->stream != nullptr) {
            std::vector copy(data.begin(), data.end());
            object_method(x->stream, sadam::stream_addarray, &copy);
            object_method(x->stream, sadam::stream_clear);
        }
        // object_post((t_object *)x, "Received %zu bytes", data.size());
        // for (auto b : data) {
        //     outlet_int(x->outlet, b);
        // }
    });
    x->port->try_consume_from_message_queue([x](const log_message &msg) {
        log(x, msg);
    });
    clock_fdelay(x->clock, x->poll_interval);
}

void bs_serial_notify(t_bs_serial *x, t_symbol *s, t_symbol *msg, void *sender, void *data) {
    if (msg == sadam::stream_binding) {
        x->stream = (t_object *)data;
    } else if (msg == sadam::stream_unbinding) {
        x->stream = nullptr;
    }
}
