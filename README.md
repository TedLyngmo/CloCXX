# CloCXX
A header only C++ library with `std::chrono` compatible clocks satisfying the _TrivialClock_ requirement 

#### Building the `clocxx.hpp` header
```
make
```

#### About

`CloCXX` comes with a file with recipes named `seen_clockid_t`. This contains the `clockid_t` macro name, a `bool` describing if the clock is steady or not and finally a description of the clock. When building the `clocxx.hpp` header, each recipe will be checked individually and only working clock definitions will be included in the generated header file. In addition to the `typedef`s required to be a _TrivialClock_, each `lyn::chrono` clock comes with two additional constants:

```c++
// The actual clockid_t used:
static constexpr clockid_t clock_id;

// If this clock can use TIMER_ABSTIME in clock_nanosleep:
static constexpr bool can_sleep_until_abstime;
```
While the "steadyness" of each clock is hardcoded in the recipe file, if the clock can sleep until an absolute time is determined while generating the header file. Drop-in replacement functions for `std::this_thread::sleep_for` and `std::this_thread::sleep_until` are available in `lyn::this_thread` that sleeps until an absolute time if the clock supports it. Note that `lyn::this_thread::sleep_for` has a `Clock` template parameter that can be used to specify what clock to use for sleeping.
```c++
template<class Clock = std::chrono::steady_clock, class Rep, class Period>
void sleep_for(const std::chrono::duration<Rep, Period>& dur);

template<class Clock, class Duration>
void sleep_until(const std::chrono::time_point<Clock, Duration>& tp);
```

**Note:** The header file will only function correctly when used on the platform where it was generated. Cross-compilation is not supported. Create the file on each individual platform where you want it.

### Example clocks that may be generated:
```c++
/*
 * monotonic_coarse_clock - A faster but less precise version of
 * CLOCK_MONOTONIC. Use when you need very fast, but not fine-grained
 * timestamps. Requires per-architecture support, and probably also
 * architecture support for this flag in the vdso(7). Linux-specific.
 */
struct monotonic_coarse_clock :
    public detail::clock_base<monotonic_coarse_clock, true, false,
                              CLOCK_MONOTONIC_COARSE> {};

/*
 * monotonic_raw_clock - Similar to CLOCK_MONOTONIC, but provides access to
 * a raw hardware-based time that is not subject to NTP adjustments or the
 * incremental adjustments performed by adjtime(3). This clock does not
 * count time that the system is suspended. Linux-specific.
 */
struct monotonic_raw_clock :
    public detail::clock_base<monotonic_raw_clock, true, false,
                              CLOCK_MONOTONIC_RAW> {};
```
All clocks share the same base implementation:
```c++
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

            return time_point(
                duration(std::chrono::seconds(tp.tv_sec) +
                         std::chrono::nanoseconds(tp.tv_nsec)));
        }
    };
} // namespace detail
```
