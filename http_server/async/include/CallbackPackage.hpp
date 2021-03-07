#pragma once

template <typename T>
struct CallbackPackage {
    CallbackPackage(std::shared_ptr<T> argument = std::shared_ptr<T>(),
                    event* ev = nullptr) :  // maybe replace event* with shared_ptr?
        argument(argument),
        ev(ev) {}

    std::shared_ptr<T> argument;
    event* ev;
};
