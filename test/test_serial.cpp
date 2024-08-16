//
// Created by Obi Davis on 03/07/2024.
//
#include "boost/asio/io_context.hpp"
#include "boost/asio/serial_port.hpp"
#include <iostream>
#include <regex>
#include <span>
#include <thread>
#include <filesystem>

static std::vector<std::string> get_serial_devices(const std::string &pattern);

class serial_port {
public:
    serial_port(boost::asio::io_context &io_context, const std::string &device)
        : port(io_context, device), device(device) {
        open_port();
    }

private:
    void open_port() {
        try {
            if (port.is_open()) {
                port.close();
            }
            port.open(device);
            port.set_option(boost::asio::serial_port::baud_rate(9600));
            port.set_option(boost::asio::serial_port::character_size(8));
            port.set_option(boost::asio::serial_port::parity(boost::asio::serial_port::parity::none));
            port.set_option(boost::asio::serial_port::stop_bits(boost::asio::serial_port::stop_bits::one));
            port.set_option(boost::asio::serial_port::flow_control(boost::asio::serial_port::flow_control::none));
            start_read();
        } catch (const std::exception &e) {
            std::cerr << "Failed to open port: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));
            open_port();
        }
    }
    void start_read() {
        port.async_read_some(boost::asio::buffer(read_buffer),
                             [this](const boost::system::error_code &ec, size_t bytes_transferred) {
                                 handle_read(ec, bytes_transferred);
                             });
    }
    void handle_read(const boost::system::error_code &ec, size_t bytes_transferred) {
        if (ec) {
            std::cerr << "Error: " << ec.message() << std::endl;
            open_port();
        } else {
            std::cout.write(read_buffer.data(), bytes_transferred);
            std::cout.flush();
            start_read();
        }
    }
    boost::asio::serial_port port;
    std::array<char, 1024> read_buffer;
    std::string device;

};

void io_context_run(boost::asio::io_context &io_context) {
    io_context.run();
}

int main() {
    boost::asio::io_context io_context;

    std::string pattern = R"(/dev/tty\.usbmodem.*)";
    auto devices = get_serial_devices(pattern);
    for (const auto &device : devices) {
        std::cout << device << std::endl;
    }

    if (devices.empty()) {
        std::cerr << "No devices found" << std::endl;
        return 1;
    }

    serial_port serial(io_context, devices[0]);
    std::thread t(io_context_run, std::ref(io_context));

    while (true) {
        // break on ctl + c
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "listening" << std::endl;

    }

    t.join();

    return 0;
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