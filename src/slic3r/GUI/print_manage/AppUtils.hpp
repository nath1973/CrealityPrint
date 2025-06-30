#ifndef RemotePrint_DmUtils_hpp_
#define RemotePrint_DmUtils_hpp_
#include "nlohmann/json.hpp"
#include <string>

class wxWebView;
namespace DM{

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
// ==================== 线程控制核心类 ====================
class ThreadController {
public:
    ThreadController() : stop_requested(false) {}

    // 请求中断
    void requestStop() {
        std::lock_guard<std::mutex> lock(mtx);
        stop_requested = true;
        cv.notify_all();
    }
    void reset() {
        std::lock_guard<std::mutex> lock(mtx);
        stop_requested = false; // 清除中断标志
    }
    // 检查中断状态
    bool isStopRequested() const {
        return stop_requested.load(std::memory_order_relaxed);
    }

    // 带超时的等待中断检查
    template<typename Rep, typename Period>
    bool waitForStop(const std::chrono::duration<Rep, Period>& timeout) {
        std::unique_lock<std::mutex> lock(mtx);
        return cv.wait_for(lock, timeout, [this] {
            return stop_requested.load();
            });
    }

private:
    std::atomic<bool> stop_requested;
    mutable std::mutex mtx;
    std::condition_variable cv;
};

class AppUtils{
public:
    static void PostMsg(wxWebView*browse, const std::string&data);
    static void PostMsg(wxWebView*browse, nlohmann::json&data);
    static std::string MD5(const std::string&file);

    static std::string extractDomain(const std::string& url);
};
class LANConnectCheck {
public: 
    static bool pingHostWithRetry(const std::string& ip, ThreadController& ctrl,  // 注入控制器,
        int retries = 1, int timeout_ms = 1000, int delay_ms = 200);// 判断设备是否在同一网段（ping）
    static bool isPortOpen(const std::string& ip, int port, ThreadController& ctrl);    //// 检查80端口，9999端口是否连接正常

    static int checkLan(const std::string& ip, ThreadController& ctrl);
};


}
#endif