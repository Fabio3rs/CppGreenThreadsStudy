#include "GreenThread.hpp"
#include "contextswap.h"
#include <cstdio>
#include <cstdlib>
#include <stack>
#include <stdexcept>

namespace {
struct CoRoutinesStack {
    std::stack<CoRoutinesStack> stack;
};

__thread CoRoutinesStack *thread_mgr{};
} // namespace

void GreenThreadMgr::init_coroutines() {
    if (thread_mgr) {
        return;
    }

    thread_mgr = new CoRoutinesStack;
}

void GreenThreadMgr::drop_coroutines() {
    if (!thread_mgr) {
        return;
    }

    delete thread_mgr;
    thread_mgr = nullptr;
}

union registeru_t {
    uintptr_t reguint;
    void *regptr;

    registeru_t() : regptr(nullptr) {}
};

struct context {
    registeru_t rax, rbx, rcx, rdx, rsi /*,rdi*/, rbp, rsp, r8, r9, r10, r11,
        r12, r13, r14, r15;
};

void GreenThread::call_wrapper [[noreturn]] (GreenThread &gthread) {
    gthread.thrfn(*gthread.get_manager(), gthread.argval);

    gthread.get_manager()->end_current();
    exit(1);
}

static constexpr size_t STACK_SIZE = 1024 * 1024;

void GreenThread::prepare_stack() {
    ctx->rsp.regptr = stack.get() + STACK_SIZE - 1;
    ctx->rbp = ctx->rsp;
    ctx->rcx.regptr = this;
    ctx->rbx.regptr = argval;

    uintptr_t *stck = reinterpret_cast<uintptr_t *>(ctx->rsp.regptr);
    stck--;
    *stck = reinterpret_cast<uintptr_t>(call_wrapper);
    stck--;
    *stck = reinterpret_cast<uintptr_t>(rcx_to_rdi_rbx_to_rsi_mov);

    ctx->rsp.regptr = stck;
}

void GreenThread::set_arg(void *arg) { argval = ctx->rbx.regptr = arg; }

void *GreenThread::prepare_stack(size_t reserve) {
    ctx->rsp.regptr = stack.get() + STACK_SIZE - 1;
    ctx->rbp = ctx->rsp;
    ctx->rcx.regptr = this;
    set_arg(argval);

    uintptr_t *stck = reinterpret_cast<uintptr_t *>(ctx->rsp.regptr);
    stck -= (reserve + sizeof(uintptr_t) - 1) / sizeof(uintptr_t);

    void *result = stck;

    stck--;
    *stck = reinterpret_cast<uintptr_t>(call_wrapper);
    stck--;
    *stck = reinterpret_cast<uintptr_t>(rcx_to_rdi_rbx_to_rsi_mov);

    ctx->rsp.regptr = stck;

    return result;
}

GreenThread::GreenThread(GreenThread::greenfn_t fun, GreenThreadMgr *mgrhandle,
                         void *arg)
    : ctx(std::make_shared<context>()),
      stack(std::make_unique<uintptr_t[]>(STACK_SIZE)), thrmanager(mgrhandle) {
    thrfn = fun;
    to_delete = false;
    argval = arg;
}

GreenThread::GreenThread(GreenThreadMgr *mgrhandle)
    : ctx(std::make_shared<context>()), thrmanager(mgrhandle) {
    to_delete = false;
    argval = nullptr;
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

size_t GreenThreadMgr::new_thread(GreenThread::greenfn_t fun, void *argval) {
    threads.push_back({fun, this, argval});
    threads.back().prepare_stack();
    return threads.size();
}

GreenThreadMgr::GreenThreadMgr() {
    threads.emplace_back(GreenThread{this});
    current_it = threads.begin();
}
