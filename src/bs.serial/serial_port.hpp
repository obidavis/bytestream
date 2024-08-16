//
// Created by Obi Davis on 03/07/2024.
//

#ifndef SERIAL_READER_HPP
#define SERIAL_READER_HPP

#include <boost/asio/serial_port.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <condition_variable>
#include <thread>
#include <span>

enum class log_level {
    info,
    warning,
    error
};

struct log_message {
    std::string message;
    log_level level = log_level::info;
};

struct serial_port_options {
    size_t chunk_size = 2 << 10;
    size_t rx_buffer_size = 2 << 14;
    size_t message_queue_size = 256;
    size_t reconnect_interval_ms = 2000;
};

class serial_port : public std::enable_shared_from_this<serial_port> {
public:
    explicit serial_port(boost::asio::io_context &io, const serial_port_options &options = {})
        : io_context(io), port(io),
          message_queue(options.message_queue_size), port_reopen_interval(options.reconnect_interval_ms),
          strand(io), timer(io) {
    }

    void try_consume_from_rx_queue(std::invocable<std::span<const uint8_t>> auto &&callback) {
        std::lock_guard lock(buffer_mutex);
        if (buffer.size() > 0) {
            const auto &buffers = buffer.data();
            for (const auto &b : buffers) {
                callback({buffer_cast<const uint8_t *>(b), buffer_size(b)});
            }
            buffer.consume(buffer.size());
        }
    }

    void try_consume_from_message_queue(std::invocable<log_message> auto &&callback) {
        message_queue.consume_all(callback);
    }

    void open(const std::string &path) {
        auto self = shared_from_this();
        strand.post([self = shared_from_this(), path] {
            self->device_path = path;
            self->open_impl();
        });
    }

    void close() {
        strand.post([self = shared_from_this()] {
            if (self->port.is_open()) {
                self->port.close();
            }
        });
    }

private:
    void open_impl() {
        if (port.is_open()) {
            info("Closing port");
            port.close();
            port.cancel();
        }
        try {
            port.open(device_path);
            // port.set_option(boost::asio::serial_port::baud_rate(9600));
            // port.set_option(boost::asio::serial_port::character_size(8));
            // port.set_option(boost::asio::serial_port::parity(boost::asio::serial_port::parity::none));
            // port.set_option(boost::asio::serial_port::stop_bits(boost::asio::serial_port::stop_bits::one));
            // port.set_option(boost::asio::serial_port::flow_control(boost::asio::serial_port::flow_control::none));
            warning(std::format("Opened {}", device_path));
            start_read();
        } catch (const std::exception &e) {
            warning(std::format("Failed to open {}, retrying in {} seconds", device_path, port_reopen_interval.count() / 1000));
            schedule_reconnect();
        }
    }

    void start_read() {
        port.async_read_some(buffer.prepare(512),
                             [self = shared_from_this()](const boost::system::error_code &ec, size_t bytes_transferred) {
                                 self->handle_read(ec, bytes_transferred);
                             });
    }

    void handle_read(const boost::system::error_code &ec, size_t bytes_transferred) {
        if (ec) {
            if (!port.is_open() && ec == boost::asio::error::operation_aborted) {
                return; // port was closed by us
            } else {
                error(std::format("Error while reading: {}", ec.message()));
                schedule_reconnect();
            }
        } else {
            {
                std::lock_guard lock(buffer_mutex);
                buffer.commit(bytes_transferred);
                info(std::format("Received {} bytes", bytes_transferred));
                info(std::format("Buffer contains {} bytes", buffer.size()));
            }
            start_read();
        }
    }

    void schedule_reconnect() {
        timer.expires_after(port_reopen_interval);
        timer.async_wait([self = shared_from_this()](const boost::system::error_code &ec) {
            if (ec) {
                return;
            }
            self->open_impl();
        });
    }

    void info(const std::string &msg) {
        message_queue.push({msg, log_level::info});
    }

    void warning(const std::string &msg) {
        message_queue.push({msg, log_level::warning});
    }

    void error(const std::string &msg) {
        message_queue.push({msg, log_level::error});
    }


    boost::asio::io_context &io_context;
    boost::asio::io_context::strand strand;
    boost::asio::serial_port port;

    std::string device_path;

    boost::asio::steady_timer timer;
    std::chrono::milliseconds port_reopen_interval;

    std::mutex buffer_mutex;
    boost::asio::streambuf buffer;

    boost::lockfree::spsc_queue<log_message> message_queue;
};

#endif //SERIAL_READER_HPP
