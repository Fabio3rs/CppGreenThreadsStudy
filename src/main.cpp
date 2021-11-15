#include "GreenThread.hpp"

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <thread>

uintptr_t other_number = 1234;
uintptr_t test_uintptr = 10000;

int thread_function(GreenThreadMgr &thrmgr) {
    test_uintptr += other_number;
    printf("thread_function %zu    threadc %p\n", test_uintptr, thrmgr.get_current());

    thrmgr.yield();
    test_uintptr *= 2;

    printf("thread_function after yield %zu    threadc %p\n", test_uintptr, thrmgr.get_current());

    thrmgr.yield();

    printf("thread_function exit %zu    threadc %p\n", test_uintptr, thrmgr.get_current());

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
