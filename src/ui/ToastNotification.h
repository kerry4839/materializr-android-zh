#pragma once
#include <string>
#include <vector>

namespace materializr {

class ToastNotification {
public:
    ToastNotification();

    void show(const std::string& message, float durationSec = 2.0f);
    void render();

private:
    struct Toast {
        std::string message;
        float remainingTime;
    };
    std::vector<Toast> m_toasts;
};

} // namespace materializr
