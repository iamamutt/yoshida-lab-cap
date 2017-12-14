/**
    project: cogdevcam
    source file: tools
    description:

    @author Joseph M. Burling
    @version 0.9.1 12/14/2017
*/

#ifndef __COGDEVCAM_TOOLS_H
#define __COGDEVCAM_TOOLS_H

#include <algorithm>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <chrono>
#include <future>
#include <iostream>
#include <map>
#include <ratio>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
//#include <opencv2/core/mat.hpp>
/*

namespace pkgs {
template<typename Val = double>
using KeyVal = std::pair<std::string, Val>;

template<typename Val       = int,
         typename Compare   = std::less<std::string>,
         typename Allocator = std::allocator<KeyVal<Val>>>
class Dict : public std::map<std::string, Val, Compare, Allocator>
{
  public:
    Dict() = default;

    Dict(const std::initializer_list<KeyVal<Val>> __l,
         const Compare &                          __comp = Compare(),
         const Allocator &                        __a    = Allocator())
      : std::map<std::string, Val, Compare, Allocator>(__l, __comp, __a){};

    const std::vector<std::string>
    keys()
    {
        std::vector<std::string> k;
        for (auto iter = this->begin(); iter != this->end(); ++iter)
        {
            k.emplace_back(iter->first);
        }
        return k;
    };

    std::vector<Val>
    values()
    {
        std::vector<Val> v;
        for (auto iter = this->begin(); iter != this->end(); iter++)
        {
            v.emplace_back(iter->second);
        }
        return v;
    };
};
};  // namespace pkgs
*/

namespace err {
class Runtime;

struct Force : std::exception
{
    Force() = default;

    explicit Force(std::string _msg) : msg(std::move(_msg)){};
    std::string msg = "!";

    const char *
    what() const noexcept override
    {
        return msg.c_str();
    }
};

/*
 * try {
 *  err::Force("An error!");
 * } catch (std::exception &e) {
 *  throw err::Runtime(e);
 * }
 */
class Runtime : public std::runtime_error
{
  public:
    Runtime() : Runtime("Unknown error"){};

    explicit Runtime(const std::string &msg) : Runtime("", Force(msg)){};

    explicit Runtime(const std::exception &err) : Runtime("", err){};

    Runtime(const std::string &msg, const std::exception &err)
      : std::runtime_error(appendExcept(msg, err))
    {
        static_error += 1;
    };

    std::string
    appendExcept(const std::string &err_insert, const std::exception &err)
    {
        std::stringstream err_msg;
        std::string       sep = err_insert.empty() ? "" : ": ";
        err_msg << "\n  - (" << getStatic_error() << ") " << err_insert << sep
                << err.what();
        return err_msg.str();
    };

    static void
    resetCounter(int init_val = 0)
    {
        static_error = init_val;
    };

    static int
    getStatic_error()
    {
        return static_error;
    }

  private:
    static int static_error;
};

void
resetRuntimeErrorCount(int init_val = 0)
{
    Runtime::resetCounter(init_val);
};

int Runtime::static_error = 0;
};  // namespace err

namespace misc {

std::string
strAppend(const std::initializer_list<std::string> &strings,
          const std::string &                       sep = std::string(" "))
{
    std::stringstream stream;
    auto              n = strings.size();
    for (auto iter = strings.begin(); iter != strings.end(); ++iter, --n)
    {
        if (n > 1)
        {
            stream << *iter << sep;
        } else
        {
            stream << *iter;
        };
    }
    return stream.str();
};

void
strPrint(const std::initializer_list<std::string> &strings,
         const std::string &                       sep = std::string(" "))
{
    std::cout << strAppend(strings, sep) << "\n";
};

template<typename T>
std::string
zeroPadStr(const T &value, unsigned n = 2)
{
    std::string        sign = value < 0 ? "-" : "";
    std::ostringstream ss;
    ss << sign << std::setw(n) << std::setfill('0') << std::abs(value);
    return ss.str();
};

int
findNearestTimeStamp(double tick, std::vector<int> ticks)
{
    std::vector<double> tickDiff(ticks.size());
    for (auto i = 0; i < ticks.size(); ++i)
    {
        tickDiff[i] = abs(static_cast<double>(ticks[i]) - tick);
    }
    auto mins      = min_element(tickDiff.begin(), tickDiff.end());
    auto min_index = static_cast<int>(distance(tickDiff.begin(), mins));
    return min_index;
};

std::string
filePrefix()
{
    auto              now = boost::posix_time::second_clock::local_time();
    std::stringstream ts;
    ts << now.date().year() << "_" << static_cast<int>(now.date().month())
       << "_" << now.date().day() << "-at-" << now.time_of_day().hours() << "_"
       << now.time_of_day().minutes();
    std::string file_prefix_string(ts.str());
    return file_prefix_string;
};

bool
fileExists(const std::string &path)
{
    const boost::filesystem::path boost_path(path);
    return boost::filesystem::exists(boost_path);
};

std::tuple<std::string, std::string>
makeDirectory(const std::string &root_path, const std::string &file_prefix = "")
{
    std::string folder_name{root_path};
    if (!file_prefix.empty())
    {
        folder_name = folder_name + "/" + file_prefix;
    }
    boost::filesystem::path boost_path(folder_name);
    auto                    parent = boost_path.parent_path();
    try
    {
        if (boost_path.has_filename())
        {
            boost::filesystem::create_directories(boost_path.parent_path());
        } else
        {
            boost::filesystem::create_directories(boost_path);
        }
    } catch (const boost::filesystem::filesystem_error &err)
    {
        throw err::Runtime(err);
    }
    return std::make_tuple(parent.string(), boost_path.stem().string());
};

/*
std::tuple<std::vector<std::vector<cv::Mat>>, std::vector<std::vector<int>>>
mergePastPresent( std::vector<std::vector<cv::Mat>> &past_mat,
    std::vector<std::vector<cv::Mat>> &present_mat,
    std::vector<std::vector<int>> &past_ts,
    std::vector<std::vector<int>> &present_ts,
    int last_length
){
  auto mat_size = past_mat.size();
  auto ts_size = past_ts.size();
  if (mat_size != ts_size) {
    throw std::length_error("N timestamps not same as N cameras");
  }
  std::vector<std::vector<cv::Mat>> images(mat_size);
  std::vector<std::vector<int>> timestamps(ts_size);
  std::vector<std::future<void>> mat_thread;
  std::vector<std::future<void>> ts_thread;
  for (auto i = 0; i < mat_size; ++i) {
    mat_thread.emplace_back(std::async(std::launch::async, [
        &images,
        &past_mat,
        &present_mat,
        &last_length
    ](int ii){
      auto size_mat_0 = past_mat[ii].size();
      for (auto j = size_mat_0 - 1; j > size_mat_0 - last_length && j >= 0; --j)
{ images[ii].emplace_back(past_mat[ii][j]);
      }
      for (auto j : present_mat[ii]) {
        images[ii].emplace_back(j);
      }
    }, i));
  }
  for (auto i = 0; i < ts_size; ++i) {
    ts_thread.emplace_back(std::async(std::launch::async, [
        &timestamps,
        &past_ts,
        &present_ts,
        &last_length
    ](int ii){
      auto size_ts_0 = past_ts[ii].size();
      for (auto j = size_ts_0 - 1; j > size_ts_0 - last_length && j >= 0; --j) {
        timestamps[ii].emplace_back(past_ts[ii][j]);
      }
      for (auto j : present_ts[ii]) {
        timestamps[ii].emplace_back(j);
      }
    }, i));
  }
  for (auto &i : ts_thread) {
    i.wait();
  }
  for (auto &i : mat_thread) {
    i.wait();
  }
  return make_tuple(images, timestamps);
}
*/
int
msTimestamp(std::chrono::time_point<std::chrono::high_resolution_clock> &tick)
{
    std::chrono::time_point<std::chrono::high_resolution_clock>
                                  tock = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> tdur = tock - tick;
    std::chrono::milliseconds
      ms = std::chrono::duration_cast<std::chrono::milliseconds>(tdur);
    return static_cast<int>(ms.count());
};

void
printTime(const std::string &                                         text,
          std::chrono::time_point<std::chrono::high_resolution_clock> ts)
{
    std::cout << text << ": " << msTimestamp(ts) << "\n";
};

void
removeDirectory(const std::string &folder)
{
    try
    {
        boost::filesystem::path boostPath(folder);
        boost::filesystem::remove_all(boostPath);
    } catch (boost::filesystem::filesystem_error &error)
    {
        throw err::Runtime(error);
    }
};

void
removeFile(const std::string &path)
{
    try
    {
        boost::filesystem::path boostPath(path);
        boost::filesystem::remove(boostPath);
    } catch (boost::filesystem::filesystem_error &error)
    {
        throw err::Runtime(error);
    }
};

void
waitMilliseconds(int t)
{
    std::chrono::milliseconds ms(t);
    std::this_thread::sleep_for(ms);
}

void
writeCSVHeaders(std::ofstream &file, const std::vector<std::string> &name)
{
    size_t n = name.size();
    for (int i = 0; i < n; ++i)
    {
        file << name[i];
        if (i < n - 1)
        {
            file << ",";
        }
    }
    file << "\n";
};

void
writeTimeStampData(std::vector<std::ofstream> &file,
                   std::vector<double> &       timestamps)
{
    for (int i = 0; i < file.size(); ++i)
    {
        file[i] << timestamps[i] << "\n";
    }
};

template<typename T>
T
min(const std::vector<T> &_v)
{
    return *std::min_element(_v.begin(), _v.end());
};

template<typename T>
T
min(const std::initializer_list<T> &_l)
{
    return *std::min_element(_l.begin(), _l.end());
};

template<typename T>
T
max(const std::vector<T> &_v)
{
    return *std::max_element(_v.begin(), _v.end());
};

template<typename T>
T
max(const std::initializer_list<T> &_l)
{
    return *std::max_element(_l.begin(), _l.end());
};

template<typename T>
T
cat(const std::initializer_list<T> &_l)
{
    T fill;
    for (auto v : _l)
    {
        fill.insert(fill.end(), v.begin(), v.end());
    }
    return fill;
};

template<typename T>
bool
is_member(const T &val, std::vector<T> vec)
{
    auto begin = vec.begin();
    auto end   = vec.end();
    return std::find(begin, end, val) != end;
};
};  // namespace misc

namespace timing {
using DefaultClock = std::conditional<
  std::chrono::high_resolution_clock::is_steady,
  std::chrono::high_resolution_clock,
  std::chrono::steady_clock>::type;

// tick intervals / periods
using nano   = std::nano;
using micro  = std::micro;
using milli  = std::milli;
using second = std::ratio<1, 1>;

// integer and floating point representation types
using Int_t   = intmax_t;
using Float_t = long double;

// conversion units
using unit_nano_int = std::chrono::duration<Int_t, nano>;
using unit_nano_flt = std::chrono::duration<Float_t, nano>;
using unit_us_int   = std::chrono::duration<Int_t, micro>;
using unit_us_flt   = std::chrono::duration<Float_t, micro>;
using unit_ms_int   = std::chrono::duration<Int_t, milli>;
using unit_ms_flt   = std::chrono::duration<Float_t, milli>;
using unit_sec_int  = std::chrono::duration<Int_t, second>;
using unit_sec_flt  = std::chrono::duration<Float_t, second>;

// high precision timing aliases
using Duration  = unit_nano_int;
using TimePoint = std::chrono::time_point<DefaultClock, Duration>;

// some values so I don't have to type out zeros
constexpr Int_t   nano_int  = 1000000000;
constexpr Float_t nano_flt  = 1000000000;
constexpr Int_t   micro_int = 1000000;
constexpr Float_t micro_flt = 1000000;
constexpr Int_t   milli_int = 1000;
constexpr Float_t milli_flt = 1000;

template<typename TP>
TP
nowTP()
{
    return std::chrono::time_point_cast<typename TP::duration, DefaultClock>(
      DefaultClock::now());
};

template<typename TP, typename Dur = typename TP::duration>
TP
futureTP(const Dur &duration)
{
    auto epoch = nowTP<TP>();
    epoch += duration;
    return epoch;
};

template<typename TP, typename Dur = typename TP::duration>
TP
pastTP(const Dur &duration)
{
    auto epoch = nowTP<TP>();
    epoch -= duration;
    return epoch;
};

constexpr auto getPast    = pastTP<TimePoint, Duration>;
constexpr auto getPresent = nowTP<TimePoint>;
constexpr auto getFuture  = futureTP<TimePoint, Duration>;

template<typename ToDur, typename FromDur, typename InputType>
auto
convertValue(const InputType &units)
{
    auto new_unit = static_cast<typename FromDur::rep>(units);
    return std::chrono::duration_cast<ToDur>(FromDur(new_unit)).count();
};

template<typename TP, typename Unit = milli>
void
printTimepoint(const TP &tp, size_t places = 0, const std::string &append = "")
{
    auto elapsed  = tp.time_since_epoch();
    auto duration = std::chrono::duration_cast<
                      std::chrono::duration<double, Unit>>(elapsed)
                      .count();
    std::cout << std::fixed << std::setprecision(places) << duration << " "
              << append << "\n";
};

template<typename Dur, typename Unit = milli>
void
printDuration(const Dur &d, size_t places = 0, const std::string &append = "")
{
    auto duration = std::chrono::duration_cast<
      std::chrono::duration<double, Unit>>(d);
    std::cout << std::fixed << std::setprecision(places) << duration.count()
              << " " << append << "\n";
};

/// keep time of events since class instantiation
template<typename Dur = Duration>
class Clock
{
  public:
    using ctype   = typename Dur::rep;
    using stdtype = typename Duration::rep;

    Clock(const Clock<Dur> &clock) { sync(clock); };

    Clock() { setAllTimePoints(getPresent()); };

    explicit Clock(const TimePoint &tp) { setAllTimePoints(tp); };

    void
    sync(const Clock<Dur> &clock)
    {
        start_time_point       = clock.start_time_point;
        captured_time_point    = clock.captured_time_point;
        timer_time_point       = clock.timer_time_point;
        duration_since_elapsed = clock.duration_since_elapsed;
        duration_since_lap     = clock.duration_since_lap;
        timer_duration         = clock.timer_duration;
        timer_duration_c       = clock.timer_duration_c;
        timer_threshold        = clock.timer_threshold;
    };

    void
    set(TimePoint tp)
    {
        setAllTimePoints(tp);
        resetDurations();
    };

    void
    reset()
    {
        set(getPresent());
    };

    void
    syncBaseTime(double proposed_sec)
    {
        Duration actual_sec     = getPresent() - start_time_point;
        Duration proposed_sec_d = std::chrono::duration_cast<Duration>(
          unit_sec_flt(static_cast<Float_t>(proposed_sec)));
        start_time_point = start_time_point + (actual_sec - proposed_sec_d);
    };

    void
    update(bool main_tp = false)
    {
        main_tp ? start_time_point : captured_time_point = getPresent();
    };

    ctype
    stopwatch(TimePoint time_point = getPresent())
    {
        duration_since_lap  = time_point - captured_time_point;
        captured_time_point = time_point;
        return recast(duration_since_lap);
    };

    ctype
    elapsed(TimePoint time_point = getPresent())
    {
        duration_since_elapsed = time_point - start_time_point;
        return recast(duration_since_elapsed);
    };

    ctype
    recast(const Duration &duration)
    {
        return std::chrono::duration_cast<Dur>(duration).count();
    };

    void
    setTimeout(Dur t_minus, Float_t thresh = 1.0)
    {
        if (thresh < 0 || thresh > 1)
        {
            throw err::Runtime("timeout threshold must be between 0 and 1");
        }

        timer_duration   = std::chrono::duration_cast<Duration>(t_minus);
        timer_duration_c = timer_duration.count();

        // anything above this resets timeout
        timer_threshold  = Duration(static_cast<stdtype>(
          std::round(static_cast<Float_t>(timer_duration_c) * thresh)));
        timer_time_point = getPresent();
    };

    void
    setTimeout(ctype t_minus, double thresh = 1.0) {
        setTimeout(Dur(t_minus), static_cast<Float_t>(thresh));
    };

    bool
    timeout(TimePoint time_point = getPresent())
    {
        Duration duration_leftover = time_point - timer_time_point;
        if (timer_threshold <= timer_threshold.zero()) return true;
        if (timer_threshold <= duration_leftover)
        {
            timer_time_point  = time_point;
            duration_leftover = timer_duration - duration_leftover;
            if (duration_leftover < duration_leftover.zero())
            {
                timer_time_point = timer_time_point -
                                   leftOverTime(duration_leftover);
            } else
            {
                timer_time_point = timer_time_point + duration_leftover;
            }
            return true;
        }

        return false;
    };

    /**
     * Time after the timeout period will be rolled over into the new amount of
     * time left, but only for multiples of the timer duration set. A threshold
     * is used since a short amount of time is rolled over, the next time it is
     * called may be more than the timeout period and a multiple used creating
     * more frequent stops
     */
    Duration
    leftOverTime(Duration &neg_dur)
    {
        auto     over_time = static_cast<Float_t>(neg_dur.count());
        auto     over_time_multi = -over_time -
                               timer_duration_c *
                                 std::floor(-over_time / timer_duration_c);
        return Duration(static_cast<stdtype>(over_time_multi));
    };

    const TimePoint &
    getStartTime() const
    {
        return start_time_point;
    };

    const TimePoint &
    getLapTime() const
    {
        return captured_time_point;
    };

    const Duration &
    getElapsedDuration() const
    {
        return duration_since_elapsed;
    };

    const Duration &
    getLapDuration() const
    {
        return duration_since_lap;
    };

    Float_t
    getSecondsMultiplier() const
    {
        auto num       = static_cast<Float_t>(Dur::period::num);
        auto den       = static_cast<Float_t>(Dur::period::den);
        auto inv_ratio = 1.0 / (num / den);
        return inv_ratio;
    };

  private:
    TimePoint start_time_point    = getPresent();
    TimePoint captured_time_point = getPresent();
    TimePoint timer_time_point    = getPresent();
    Duration  duration_since_elapsed{0};
    Duration  duration_since_lap{0};
    Duration  timer_duration{0};
    Duration  timer_threshold{0};
    ctype     timer_duration_c = 0;

    void
    setAllTimePoints(const TimePoint &tp)
    {
        start_time_point    = tp;
        captured_time_point = tp;
        timer_time_point    = tp;
    };

    void
    resetDurations()
    {
        duration_since_elapsed = duration_since_elapsed.zero();
        duration_since_lap     = duration_since_lap.zero();
    };
};

using SecondClock = Clock<unit_sec_flt>;
using MilliClock  = Clock<unit_ms_flt>;
using MicroClock  = Clock<unit_us_int>;
using NanoClock   = Clock<unit_nano_int>;

namespace sleep {
using SleepDuration  = unit_nano_flt;
using SleepTimePoint = std::chrono::time_point<DefaultClock, SleepDuration>;

constexpr auto getPast    = pastTP<SleepTimePoint, SleepDuration>;
constexpr auto getPresent = nowTP<SleepTimePoint>;
constexpr auto getFuture  = futureTP<SleepTimePoint, SleepDuration>;

SleepDuration
sec(const Float_t &sec)
{
    auto sleep_end = getFuture(SleepDuration(sec * nano_flt));
    auto now_time  = getPresent();
    while (now_time < sleep_end)
    {
        now_time = getPresent();
    };
    return now_time - sleep_end;
}

template<typename Dur>
void
thread(const Dur &d)
{
    std::this_thread::sleep_for(d);
}
};  // namespace sleep

std::vector<timing::Float_t>
test_time(const timing::Float_t &wait_sec, size_t n = 500)
{
    const auto second_convert = static_cast<timing::Float_t>(wait_sec *
                                                             timing::milli_flt);
    std::vector<timing::Float_t> store_errors(n);
    timing::MilliClock           clock;
    for (auto &iter : store_errors)
    {
        timing::sleep::sec(wait_sec);
        auto lap = clock.stopwatch();
        // printDuration<Duration, milli>(t.getTimerDuration(), 7, "ms");
        iter = lap - second_convert;
    }

    clock.elapsed();
    timing::printDuration<timing::Duration, timing::milli>(
      clock.getElapsedDuration(), 7, " total time");
    return store_errors;
};
};  // namespace timing

namespace futures {

using SharedFuture = std::shared_future<void>;
using UniqueFuture = std::future<void>;

template<typename Future>
void
futureWait(const Future &_future)
{
    _future.wait();
};

template<typename Future>
bool
futureStatus(const Future &_future)
{
    auto valid = _future.valid();
    if (valid)
    {
        auto status = _future.wait_for(std::chrono::seconds(0));
        valid       = status == std::future_status::ready;
    }
    return valid;
};

template<typename Future>
void
futureValidWait(const Future &_future)
{
    if (_future.valid())
    {
        _future.wait();
    }
};

template<typename Future>
Future
makeVoidFutureValid()
{
    Future _future;
    _future = std::async(std::launch::async, []() {});
    return _future;
}

template<typename Future, typename T>
Future
makeFutureValid()
{
    Future _future;
    _future = std::async(std::launch::async, []() { return T(); });
    return _future;
}
};  // namespace futures

#endif  // __COGDEVCAM_TOOLS_H
