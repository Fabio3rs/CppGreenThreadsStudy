#include "GreenThread.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <mutex>
#include <numeric>
#include <thread>

struct gthread_args {
    int num;
};

static std::mutex printmtx;

static void thread_safe_print(const char *__restrict__ format, ...) {
    std::lock_guard<std::mutex> lck(printmtx);
    va_list va;
    va_start(va, format);
    vprintf(format, va);
    va_end(va);
}

static int thread_function(GreenThreadMgr &thrmgr, void *arg) {
    gthread_args *args = reinterpret_cast<gthread_args *>(arg);
    std::array<unsigned int, 4096> arr;

    void *currentptr = thrmgr.get_current();

    thread_safe_print("Thread %p started | arg num %d\n", currentptr,
                      args ? args->num : -1);

    thrmgr.yield();

    for (size_t i = 0; i < arr.size(); i++)
        arr[i] = static_cast<unsigned int>(i);

    thread_safe_print("arr seted %p\n", currentptr);
    thrmgr.yield();

    auto sum = std::accumulate(arr.begin(), arr.end(), 0u);
    thread_safe_print("arr sum %d | %p\n", sum, currentptr);

    return 0;
}

static void other_thread_function(GreenThreadMgr &thrmgr, std::string_view a,
                                  int b, int c, int d) {
    thread_safe_print("thread msg (%p); a (%.*s); b (%d); c (%d); d (%d)\n",
                      reinterpret_cast<void *>(thrmgr.get_current()), a.size(),
                      a.data(), b, c, d);

    thrmgr.yield();
    thread_safe_print("Exiting thread msg (%p);\n",
                      reinterpret_cast<void *>(thrmgr.get_current()));
}

static void real_thread() {
    GreenThreadMgr mgr;

    gthread_args aaa;
    aaa.num = 10;

    mgr.new_thread(thread_function, &aaa);
    mgr.new_thread(thread_function, &aaa);
    mgr.new_thread(thread_function, &aaa);
    mgr.new_thread_custom(other_thread_function, "This is a C string", 0, 1,
                          20);
    mgr.new_thread_custom(other_thread_function, "This is another C string", 2,
                          3, 100);
    mgr.new_thread_custom(other_thread_function, "This is a test", 100, 1000,
                          1000);
    
    std::string str = "AAAAAAAA";

    str += "    \t";
    mgr.yield();
    str += "BBBBBBBBBBBBBBBBBBBBB";

    mgr.yield();
    
    thread_safe_print("%.*s\n", str.size(), str.c_str());
}

int main() {
    size_t thhw = std::thread::hardware_concurrency();

    std::deque<std::thread> thrd;

    for (size_t i = 0; i < thhw; i++) {
        thrd.push_back(std::thread(real_thread));
    }

    for (auto &t : thrd)
        if (t.joinable())
            t.join();

    return 0;
}
