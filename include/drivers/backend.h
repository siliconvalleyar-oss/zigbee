#pragma once

#include "hal.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

namespace zigbee_mesh::drivers {

class SPIBackend {
public:
    SPIBackend() = default;
    ~SPIBackend() { close(); }

    bool open(const std::string& device, uint32_t speed = 1000000) {
        fd_ = ::open(device.c_str(), O_RDWR);
        if (fd_ < 0) return false;

        uint8_t mode = SPI_MODE_0;
        uint8_t bits = 8;
        if (ioctl(fd_, SPI_IOC_WR_MODE, &mode) < 0) { close(); return false; }
        if (ioctl(fd_, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) { close(); return false; }
        if (ioctl(fd_, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) { close(); return false; }

        speed_ = speed;
        return true;
    }

    void close() {
        if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
    }

    bool isOpen() const { return fd_ >= 0; }

    bool transfer(uint8_t* tx, uint8_t* rx, size_t len) const {
        struct spi_ioc_transfer tr{};
        tr.tx_buf = reinterpret_cast<unsigned long>(tx);
        tr.rx_buf = reinterpret_cast<unsigned long>(rx);
        tr.len = len;
        tr.speed_hz = speed_;
        tr.bits_per_word = 8;
        return ioctl(fd_, SPI_IOC_MESSAGE(1), &tr) >= 0;
    }

    bool writeRegister(uint16_t reg, uint8_t value) const {
        uint8_t tx[3] = {
            static_cast<uint8_t>((reg >> 8) | 0x80),
            static_cast<uint8_t>(reg & 0xFF),
            value
        };
        uint8_t rx[3] = {};
        return transfer(tx, rx, 3);
    }

    uint8_t readRegister(uint16_t reg) const {
        uint8_t tx[3] = {
            static_cast<uint8_t>((reg >> 8) & 0x7F),
            static_cast<uint8_t>(reg & 0xFF),
            0x00
        };
        uint8_t rx[3] = {};
        transfer(tx, rx, 3);
        return rx[2];
    }

    bool writeBurst(uint16_t reg, const uint8_t* data, size_t len) const {
        std::vector<uint8_t> tx(len + 2);
        tx[0] = static_cast<uint8_t>((reg >> 8) | 0x80);
        tx[1] = static_cast<uint8_t>(reg & 0xFF);
        std::memcpy(tx.data() + 2, data, len);
        std::vector<uint8_t> rx(len + 2, 0);
        return transfer(tx.data(), rx.data(), len + 2);
    }

    bool readBurst(uint16_t reg, uint8_t* data, size_t len) const {
        std::vector<uint8_t> tx(len + 2, 0);
        tx[0] = static_cast<uint8_t>((reg >> 8) & 0x7F);
        tx[1] = static_cast<uint8_t>(reg & 0xFF);
        std::vector<uint8_t> rx(len + 2, 0);
        bool ok = transfer(tx.data(), rx.data(), len + 2);
        if (ok) std::memcpy(data, rx.data() + 2, len);
        return ok;
    }

private:
    mutable int fd_{-1};
    uint32_t speed_{1000000};
};

class UARTBackend {
public:
    UARTBackend() = default;
    ~UARTBackend() { close(); }

    bool open(const std::string& device, uint32_t baud_rate) {
        fd_ = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd_ < 0) return false;

        struct termios tty{};
        tcgetattr(fd_, &tty);

        speed_t speed = B115200;
        switch (baud_rate) {
            case 9600: speed = B9600; break;
            case 19200: speed = B19200; break;
            case 38400: speed = B38400; break;
            case 57600: speed = B57600; break;
            case 115200: speed = B115200; break;
            case 230400: speed = B230400; break;
            case 460800: speed = B460800; break;
            case 921600: speed = B921600; break;
            default: speed = B115200; break;
        }

        cfsetispeed(&tty, speed);
        cfsetospeed(&tty, speed);

        tty.c_cflag &= ~PARENB;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CSIZE;
        tty.c_cflag |= CS8;
        tty.c_cflag |= CLOCAL | CREAD;
        tty.c_cflag &= ~CRTSCTS;

        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
        tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP |
                          INLCR | IGNCR | ICRNL);

        tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
        tty.c_oflag &= ~OPOST;

        tcsetattr(fd_, TCSANOW, &tty);
        tcflush(fd_, TCIOFLUSH);

        return true;
    }

    void close() {
        if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
    }

    bool isOpen() const { return fd_ >= 0; }

    ssize_t write(const uint8_t* data, size_t len) {
        return ::write(fd_, data, len);
    }

    ssize_t read(uint8_t* data, size_t max_len, int timeout_ms = 100) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd_, &fds);

        struct timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;

        int ret = select(fd_ + 1, &fds, nullptr, nullptr, &tv);
        if (ret > 0 && FD_ISSET(fd_, &fds)) {
            return ::read(fd_, data, max_len);
        }
        return 0;
    }

    void flush() { tcflush(fd_, TCIOFLUSH); }

private:
    int fd_{-1};
};

} // namespace zigbee_mesh::drivers
