#pragma once

#include <cstdint>
#include <cstdio>
#include <deque>
#include <functional>
#include <memory>

struct context;
class GreenThreadMgr;

class GreenThread {
    friend GreenThreadMgr;
    typedef std::function<int(GreenThreadMgr &, void *)> greenfn_t;

    std::shared_ptr<context> ctx;
    std::shared_ptr<uintptr_t[]> stack;
    greenfn_t thrfn;

    GreenThreadMgr *thrmanager;
    void *argval;
    bool to_delete;

    void set_arg(void *arg);

    void prepare_stack();
    void *prepare_stack(size_t reserve);

    static void call_wrapper [[noreturn]] (GreenThread &gthread);

  public:
    auto get_manager() const { return thrmanager; }

    GreenThread(greenfn_t fun, GreenThreadMgr *mgrhandle, void *arg);

    GreenThread(GreenThreadMgr *mgrhandle);
};

class GreenThreadMgr {
    std::deque<GreenThread> threads;

    std::deque<GreenThread>::iterator current_it;

  public:
    GreenThread *get_current() const { return current_it.operator->(); }
    size_t new_thread(GreenThread::greenfn_t fun, void *argval = nullptr);

    template <typename rawfn_T, class... Types>
    size_t new_thread_custom(rawfn_T fun, Types &&... args) {
        using args_tuple_t = decltype(std::tuple{std::forward<Types>(args)...});
        std::function thrfn{fun};

        threads.push_back({nullptr, this, nullptr});
        auto &greenthrd = threads.back();

        auto argstuple = new (greenthrd.prepare_stack(sizeof(args_tuple_t)))
            std::tuple{std::forward<Types>(args)...};

        auto forward_args_lambda_wrapper = [thrfn](GreenThreadMgr &inst,
                                                   void *argsdata) {
            args_tuple_t &fnargs = *reinterpret_cast<args_tuple_t *>(argsdata);

            std::apply(
                [thrfn, &inst](auto &&... argsq) { thrfn(inst, argsq...); },
                fnargs);

            return 0;
        };

        greenthrd.set_arg(argstuple);
        greenthrd.thrfn = forward_args_lambda_wrapper;

        return threads.size();
    }

    void yield();
    void end_current();

    GreenThreadMgr();

    ~GreenThreadMgr() {
        while (threads.size() > 1) {
            yield();
        }
    }
};
