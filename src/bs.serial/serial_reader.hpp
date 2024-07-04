//
// Created by Obi Davis on 03/07/2024.
//

#ifndef SERIAL_READER_HPP
#define SERIAL_READER_HPP

#include <boost/asio/io_context.hpp>
#include <boost/asio/serial_port.hpp>
#include <boost/circular_buffer.hpp>
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
    log_level level;
};

template <typename T>
concept ProcessorCallback = requires(T &&t, std::span<const uint8_t> data) {
    { std::invoke(t, data) } -> std::convertible_to<size_t>;
};

class serial_reader {
public:
    serial_reader(boost::asio::io_context &io_context, const std::string &device)
        : port(io_context, device), device(device), rx_chunk(1024), rx_buffer(1024), message_queue(1024) {
        open_port();
    }

    void try_read(ProcessorCallback auto &&callback) {
        std::lock_guard lock(rx_buffer_mutex);
        if (rx_buffer.empty()) {
            return;
        }

        size_t processed = 0;
        if (rx_buffer.is_linearized()) {
            // Process in one go if the rx_buffer is linearized
            std::span<const uint8_t> data(rx_buffer.linearize(), rx_buffer.size());
            processed = callback(data);
        } else {
            // Process the first part of the rx_buffer
            std::span<const uint8_t> part1(rx_buffer.array_one().first, rx_buffer.array_one().second);
            processed += callback(part1);
            // If more data needs to be processed, process the second part
            if (processed < rx_buffer.size()) {
                std::span<const uint8_t> part2(rx_buffer.array_two().first, rx_buffer.array_two().second);
                processed += callback(part2.subspan(0, rx_buffer.size() - processed));
            }
        }
        // Remove the processed bytes from the rx_buffer
        rx_buffer.erase_begin(processed);
    }

    std::optional<log_message> get_log_message() {
        log_message message;
        if (message_queue.pop(message)) {
            return message;
        }
        return std::nullopt;
    }

private:
    void open_port() {
        try {
            if (port.is_open()) {
                info("Closing port");
                port.close();
            }
            port.open(device);
            port.set_option(boost::asio::serial_port::baud_rate(9600));
            port.set_option(boost::asio::serial_port::character_size(8));
            port.set_option(boost::asio::serial_port::parity(boost::asio::serial_port::parity::none));
            port.set_option(boost::asio::serial_port::stop_bits(boost::asio::serial_port::stop_bits::one));
            port.set_option(boost::asio::serial_port::flow_control(boost::asio::serial_port::flow_control::none));
            info("Port opened successfully");
            start_read();
        } catch (const std::exception &e) {
            warning("Failed to open port, retrying in 5 seconds");
            std::this_thread::sleep_for(std::chrono::seconds(5));
            open_port();
        }
    }
    void start_read() {
        port.async_read_some(boost::asio::buffer(rx_chunk.data(), rx_chunk.size()),
                             [this](const boost::system::error_code &ec, size_t bytes_transferred) {
                                 handle_read(ec, bytes_transferred);
                             });
    }
    void handle_read(const boost::system::error_code &ec, size_t bytes_transferred) {
        if (ec) {
            error(ec.message());
            open_port();
        } else {
            {
                std::unique_lock lock(rx_buffer_mutex);
                rx_buffer.insert(rx_buffer.end(), rx_chunk.begin(), rx_chunk.begin() + bytes_transferred);
                info(std::string("Buffer contains " + std::to_string(rx_buffer.size()) + " bytes"));
            }
            start_read();
        }
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

    boost::asio::serial_port port;
    std::vector<uint8_t> rx_chunk;
    boost::circular_buffer<uint8_t> rx_buffer;
    std::string device;
    std::mutex rx_buffer_mutex;
    std::condition_variable rx_buffer_write_condition;
    boost::lockfree::spsc_queue<log_message> message_queue;
};

#endif //SERIAL_READER_HPP
