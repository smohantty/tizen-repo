#pragma once

#include <memory>

namespace systemd {

class SystemdNotifier {
public:
    SystemdNotifier();
    ~SystemdNotifier();

    void start();

    void stop();

private:
    class Impl;
    std::unique_ptr<Impl> mImpl;
};

} // namespace systemd