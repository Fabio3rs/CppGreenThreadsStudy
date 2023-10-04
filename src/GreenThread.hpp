#pragma once

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <optional>
#include <utility>

struct context;
class GreenThreadMgr;

template <class T> struct CoRoutineFuture {
    std::unique_ptr<std::optional<T>> result{};
    GreenThreadMgr *mgr{};

    auto wait();
    auto wait_for(std::chrono::milliseconds dur) -> std::future_status;

    T &get() {
        if (!result->has_value()) {
            throw std::runtime_error("Coroutine not finished");
        }

        return result->value();
    }

    void set_value(T &&val) {
        *result = std::move(val);
        std::cout << "set_value" << std::endl;
    }

    void set_exception([[maybe_unused]] std::exception_ptr ptr) {
        // result = std::move(ptr);
    }
};

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
    static void init_coroutines();
    static void drop_coroutines();

    GreenThread *get_current() const { return current_it.operator->(); }
    size_t new_thread(GreenThread::greenfn_t fun, void *argval = nullptr);

    template <typename rawfn_T, class... Types>
    size_t new_thread_custom(rawfn_T fun, Types &&...args) {
        using args_tuple_t = decltype(std::tuple{std::forward<Types>(args)...});

        typedef struct {
            args_tuple_t argument_tuple;
            rawfn_T thread_start_fn;
        } GreenThreadData;

        auto forward_args_lambda_wrapper = [](GreenThreadMgr &inst,
                                              void *argsdata) {
            GreenThreadData &oldthrdata =
                *reinterpret_cast<GreenThreadData *>(argsdata);
            GreenThreadData fnargs = std::move(oldthrdata);

            std::function thrfn{fnargs.thread_start_fn};

            oldthrdata.~GreenThreadData();

            std::apply(
                [thrfn, &inst](auto &&...argsq) { thrfn(inst, argsq...); },
                std::move(fnargs.argument_tuple));

            return 0;
        };

        threads.push_back({forward_args_lambda_wrapper, this, nullptr});
        auto &greenthrd = threads.back();

        auto argsdata = new (greenthrd.prepare_stack(sizeof(GreenThreadData)))
            GreenThreadData{std::tuple{std::forward<Types>(args)...}, fun};

        greenthrd.set_arg(argsdata);

        return threads.size();
    }

    template <typename rawfn_T, class... Types>
    auto new_thread_noarg(rawfn_T fun, Types &&...args)
        -> CoRoutineFuture<std::invoke_result_t<rawfn_T, Types...>> {
        using args_tuple_t = decltype(std::tuple{std::forward<Types>(args)...});

        typedef struct {
            args_tuple_t argument_tuple;
            rawfn_T thread_start_fn;
            std::optional<std::invoke_result_t<rawfn_T, Types...>> *promise;
        } GreenThreadData;

        auto forward_args_lambda_wrapper =
            []([[maybe_unused]] GreenThreadMgr &inst, void *argsdata) {
                GreenThreadData &oldthrdata =
                    *reinterpret_cast<GreenThreadData *>(argsdata);
                GreenThreadData fnargs = std::move(oldthrdata);

                std::function thrfn{fnargs.thread_start_fn};

                oldthrdata.~GreenThreadData();

                try {
                    std::apply(
                        [thrfn, &fnargs](auto &&...argsq) {
                            (*fnargs.promise) = (thrfn(argsq...));
                        },
                        std::move(fnargs.argument_tuple));
                } catch (...) {
                    // fnargs.promise.set_exception(std::current_exception());
                    return 0;
                }

                return 0;
            };

        threads.push_back({forward_args_lambda_wrapper, this, nullptr});
        auto &greenthrd = threads.back();

        CoRoutineFuture<std::invoke_result_t<rawfn_T, Types...>> fut;

        fut.mgr = this;
        fut.result = std::make_unique<
            std::optional<std::invoke_result_t<rawfn_T, Types...>>>();

        auto argsdata = new (greenthrd.prepare_stack(sizeof(GreenThreadData)))
            GreenThreadData{std::tuple{std::forward<Types>(args)...}, fun,
                            fut.result.get()};

        greenthrd.set_arg(argsdata);

        return fut;
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

template <class T> inline auto CoRoutineFuture<T>::wait() {
    if (!result->has_value()) {
        mgr->yield();
    }

    return result->value();
}

template <class T>
inline auto CoRoutineFuture<T>::wait_for(std::chrono::milliseconds dur)
    -> std::future_status {

    auto start = std::chrono::steady_clock::now();
    auto end = start + dur;
    while (!result->has_value()) {
        mgr->yield();

        if (std::chrono::steady_clock::now() > end) {
            return std::future_status::timeout;
        }
    }

    return std::future_status::ready;
}
