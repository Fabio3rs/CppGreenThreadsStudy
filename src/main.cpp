#include "GreenThread.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <thread>

static uintptr_t other_number = 1234;
static uintptr_t test_uintptr = 10000;

static int thread_function(GreenThreadMgr &thrmgr) {
    std::array<unsigned int, 4096> arr;

    void *currentptr = thrmgr.get_current();

    for (size_t i = 0; i < arr.size(); i++)
        arr[i] = static_cast<unsigned int>(i);
    test_uintptr += other_number;
    printf("thread_function %zu    threadc %p\n", test_uintptr, currentptr);

    thrmgr.yield();
    test_uintptr *= 2;

    printf("thread_function after yield %zu    threadc %p\n", test_uintptr,
           currentptr);

    thrmgr.yield();

    for (size_t i = 0; i < arr.size(); i++)
        std::cout << reinterpret_cast<uintptr_t>(currentptr) << " " << arr[i]
                  << std::endl;

    printf("thread_function exit %zu    threadc %p\n", test_uintptr,
           currentptr);
    return 0;
}

int main() {
    GreenThreadMgr mgr;
    mgr.new_thread(thread_function);
    mgr.new_thread(thread_function);
    mgr.new_thread(thread_function);

    mgr.yield();

    test_uintptr *= 2;

    printf("main pos yield 1 %zu\n", test_uintptr);

    mgr.yield();
    printf("main pos yield 2 %zu\n", test_uintptr);
    printf("Exiting normally\n");
    return 0;
}
