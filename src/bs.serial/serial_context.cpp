//
// Created by Obi Davis on 04/07/2024.
//

#include "serial_context.hpp"
#include "boost/asio/io_context.hpp"
#include "ext_systhread.h"
#include "ext_proto.h"

class serial_context {
public:
    serial_context() : work(io_context) {
        method m = +[](void *io) -> void * {
            static_cast<boost::asio::io_context *>(io)->run();
            return nullptr;
        };
        systhread_create(m, &io_context, 0, 0, 0, &thread);
        quittask_install((method)+[](serial_context *context) {
            delete context;
        }, this);
    }
    ~serial_context() {
        io_context.stop();
        systhread_detach(thread);
    }

    boost::asio::io_context *get_io_context() {
        return &io_context;
    }
private:
    boost::asio::io_context io_context;
    boost::asio::io_context::work work;
    t_systhread thread;
};


boost::asio::io_context *serial_context_init() {
    static auto *context = new serial_context();
    return context->get_io_context();
}
