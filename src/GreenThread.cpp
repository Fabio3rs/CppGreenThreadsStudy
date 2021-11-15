#include "GreenThread.hpp"
#include "contextswap.h"
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

union registeru_t {
    uintptr_t reguint;
    void *regptr;

    registeru_t() : regptr(nullptr) {}
};

struct context {
    registeru_t rax, rbx, rcx, rdx, rsi /*,rdi*/, rbp, rsp, r8, r9, r10, r11,
        r12, r13, r14, r15;
};

void GreenThread::call_wrapper [[noreturn]] (GreenThread &gthread)  {
    gthread.thrfn(*gthread.get_manager());

    gthread.get_manager()->end_current();
    exit(1);
}

static constexpr size_t STACK_SIZE = 1024 * 1024;

void GreenThread::prepare_stack() {
    ctx->rsp.regptr = stack.get() + STACK_SIZE - 1;
    ctx->rbp = ctx->rsp;
    ctx->rcx.regptr = this;

    uintptr_t *stck = reinterpret_cast<uintptr_t *>(ctx->rsp.regptr);
    stck--;
    *stck = reinterpret_cast<uintptr_t>(call_wrapper);
    stck--;
    *stck = reinterpret_cast<uintptr_t>(rcx_to_rdi_mov);

    ctx->rsp.regptr = stck;
}

GreenThread::GreenThread(GreenThread::greenfn_t fun, GreenThreadMgr *mgrhandle)
    : ctx(std::make_shared<context>()),
      stack(std::make_unique<uintptr_t[]>(STACK_SIZE)), thrmanager(mgrhandle) {
    thrfn = fun;
    to_delete = false;
}

GreenThread::GreenThread(GreenThreadMgr *mgrhandle)
    : ctx(std::make_shared<context>()), thrmanager(mgrhandle) {
    to_delete = false;
}

void GreenThreadMgr::yield() {
    auto it = current_it;
    
    do {
        if ((++it) == threads.end()) {
            it = threads.begin();
        }

        for (; it != threads.end();) {
            if (it->to_delete) {
                it = threads.erase(it);
            } else {
                break;
            }
        }

        if (it == threads.end()) {
            it = threads.begin();
        }

        if (threads.size() == 1) {
            return;
        }
    } while (it == current_it);

    if (it == current_it) {
        return;
    }

    GreenThread *cur = current_it.operator->();
    GreenThread *newc = it.operator->();

    if (newc == nullptr) {
        return;
    }

    current_it = it;
    context_swap(cur->ctx.get(), newc->ctx.get());
}

void GreenThreadMgr::end_current() {
    if (!current_it->stack) {
        throw std::runtime_error("Impossible to end the starter thread");
    }

    current_it->to_delete = true;
    yield();
}

size_t GreenThreadMgr::new_thread(GreenThread::greenfn_t fun) {
    threads.push_back({fun, this});
    threads.back().prepare_stack();
    return threads.size();
}

GreenThreadMgr::GreenThreadMgr() {
    threads.emplace_back(GreenThread{this});
    current_it = threads.begin();
}
