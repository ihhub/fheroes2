#pragma once

#include <memory>

#include <3ds.h>

template <class T, int MaxThreads = 10>
class ThreadLocal {
public:
    template <class F>
    explicit ThreadLocal(F&& factory)
        : factory_(factory) {}

    T& get() {
        // Note: main thread is created by the console and is nullptr
        Thread self = threadGetCurrent();

        for (auto& s : slots_) {
            if (s.used && s.thread == self) {
                assert(s.value);
                return *s.value;
            }

            if (!s.used) {
                s.used = true;
                s.thread = self;
                s.value.reset(new T(factory_()));
                return *s.value;
            }
        }

        // Unlikely, we use less threads
        svcBreak(USERBREAK_PANIC);
    }

private:
    struct Slot {
        bool used = false;
        Thread thread = nullptr;
        std::unique_ptr<T> value;
    };

    std::function<T()> factory_;
    Slot slots_[MaxThreads];
};
