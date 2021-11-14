#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <memory>

struct context;
class GreenThreadMgr;

class GreenThread {
    friend GreenThreadMgr;
    typedef std::function<int(GreenThreadMgr &)> greenfn_t;

    std::shared_ptr<context> ctx;
    std::shared_ptr<uintptr_t[]> stack;
    greenfn_t thrfn;

    GreenThreadMgr *thrmanager;
    bool to_delete;

    void prepare_stack();

    static void call_wrapper(GreenThread &gthread);

  public:
    auto get_manager() const { return thrmanager; }

    GreenThread(greenfn_t fun, GreenThreadMgr *mgrhandle);

    GreenThread(GreenThreadMgr *mgrhandle);
};

class GreenThreadMgr {
    std::deque<GreenThread> threads;

    std::deque<GreenThread>::iterator current_it;

  public:
    GreenThread *get_current() const { return current_it.operator->(); }
    size_t new_thread(GreenThread::greenfn_t fun);

    void yield();
    void end_current();

    GreenThreadMgr();
};
