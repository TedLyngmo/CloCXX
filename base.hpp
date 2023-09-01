/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <https://unlicense.org>
*/

// Original: https://github.com/TedLyngmo/CloCXX

// NOLINTNEXTLINE(llvm-header-guard)
#ifndef CLOCXX_HPP_EE7431F0_4801_11EE_B0CD_90B11C0C0FF8
#define CLOCXX_HPP_EE7431F0_4801_11EE_B0CD_90B11C0C0FF8

#include <cerrno>
#include <chrono>
#include <ctime>
#include <type_traits>

namespace lyn {
namespace chrono {
    template<class Clock>
    struct can_sleep_until_abstime {
        static std::false_type test(...);

        template<class C>
        static auto test(C) -> std::bool_constant<C::can_sleep_until_abstime>;

        static constexpr bool value = decltype(test(std::declval<Clock>()))::value;
    };
    template<class Clock>
    static constexpr bool can_sleep_until_abstime_v = can_sleep_until_abstime<Clock>::value;
}

namespace this_thread {
    template<class Clock, class Duration>
    void sleep_until(const std::chrono::time_point<Clock, Duration>& tp);

    template<class Clock = std::chrono::steady_clock, class Rep, class Period>
    void sleep_for(const std::chrono::duration<Rep, Period>& dur) {
        if(dur <= dur.zero()) return;
        if constexpr(chrono::can_sleep_until_abstime_v<Clock>) {
            sleep_until(Clock::now() + dur);
        } else {
            auto s = std::chrono::duration_cast<std::chrono::seconds>(dur);
            auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(dur - s);
            ::timespec req{static_cast<std::time_t>(s.count()), static_cast<decltype(::timespec::tv_nsec)>(ns.count())};
            while(::nanosleep(&req, &req) == -1 && errno == EINTR) {
            }
        }
    }

    template<class Clock, class Duration>
    void sleep_until(const std::chrono::time_point<Clock, Duration>& tp) {
        if constexpr(chrono::can_sleep_until_abstime_v<Clock>) {
            auto dur = tp.time_since_epoch();
            auto s = std::chrono::duration_cast<std::chrono::seconds>(dur);
            auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(dur - s);
            ::timespec req{static_cast<std::time_t>(s.count()), static_cast<decltype(::timespec::tv_nsec)>(ns.count())};

            while(::clock_nanosleep(Clock::clock_id, TIMER_ABSTIME, &req, nullptr) == EINTR) {
            }
        } else {
            sleep_for<Clock>(tp - Clock::now());
        }
    }
} // namespace this_thread

namespace chrono {
    namespace detail {
        template<class Clock, bool IsSteady, bool CanClockUseAbsTime, clockid_t CLOCK_ID>
        struct clock_base {
            using duration = std::chrono::nanoseconds;
            using rep = duration::rep;
            using period = duration::period;
            using time_point = std::chrono::time_point<Clock, duration>;
            static constexpr bool is_steady = IsSteady;

            static constexpr clockid_t clock_id = CLOCK_ID;
            static constexpr bool can_sleep_until_abstime = CanClockUseAbsTime;

            static time_point now() noexcept {
                using namespace std;
                timespec tp; // not in std:: until gcc 9
                static_cast<void>(::clock_gettime(CLOCK_ID, &tp));

                return time_point(duration(std::chrono::seconds(tp.tv_sec) + std::chrono::nanoseconds(tp.tv_nsec)));
            }
        };
    } // namespace detail

    // Generated clocks - Note: These will only work as promised on the platform
    // on which this file was generated!

