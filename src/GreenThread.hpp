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

        typedef struct
        {
            args_tuple_t argument_tuple;
            rawfn_T thread_start_fn;
        } GreenThreadData;

        auto forward_args_lambda_wrapper = [](GreenThreadMgr &inst,
                                                 void *argsdata) {
            GreenThreadData &oldthrdata = *reinterpret_cast<GreenThreadData *>(argsdata);
            GreenThreadData fnargs = std::move(oldthrdata);

            std::function thrfn{fnargs.thread_start_fn};

            oldthrdata.~GreenThreadData();

            std::apply(
                [thrfn, &inst](auto &&... argsq) { thrfn(inst, argsq...); },
                fnargs.argument_tuple);

            return 0;
        };

        threads.push_back({forward_args_lambda_wrapper, this, nullptr});
        auto &greenthrd = threads.back();

        auto argsdata = new (greenthrd.prepare_stack(sizeof(GreenThreadData)))
            GreenThreadData{std::tuple{std::forward<Types>(args)...}, fun};

        greenthrd.set_arg(argsdata);

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
