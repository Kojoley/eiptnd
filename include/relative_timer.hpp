#ifndef RELATIVE_TIMER_HPP
#define RELATIVE_TIMER_HPP

#include <boost/asio/basic_deadline_timer.hpp>
#include <boost/chrono/duration.hpp>
#include <boost/chrono/system_clocks.hpp>

/* This timer is independent of the system clock and can be safely used
 * for the relative time measurement (eg timeouts)
 * Credits: http://pastebin.com/mmQ9VWje
 */

struct steady_time_traits
{
    typedef boost::chrono::high_resolution_clock::time_point    time_type;
 
    struct castable_duration {
 
        typedef time_type::duration                 chrono_duration;
        typedef boost::posix_time::time_duration    posix_duration;
 
        castable_duration(chrono_duration const & cd = chrono_duration())
            : chrono_duration_(cd)
        {}
 
        castable_duration(posix_duration const & pos)
            : chrono_duration_(boost::chrono::microseconds(pos.total_microseconds()))
        {}
 
        template <class ChronoRep, class ChronoPeriod>
        castable_duration(boost::chrono::duration<ChronoRep, ChronoPeriod> const & cd)
            : chrono_duration_(cd)
        {}
 
        template <class CastableToPosix>
        castable_duration(CastableToPosix const & ctp)
            : chrono_duration_(boost::chrono::microseconds(static_cast<posix_duration>(ctp).total_microseconds()))
        {}
 
        operator chrono_duration & ()               { return chrono_duration_; }
        operator chrono_duration const & () const   { return chrono_duration_; }
 
        operator posix_duration () const {
            return boost::posix_time::microseconds(
                boost::chrono::duration_cast<boost::chrono::microseconds>(chrono_duration_).count());
        }
 
    private:
        time_type::duration     chrono_duration_;
    };
 
    typedef castable_duration                                   duration_type;
 
    /// all below is required by asio
    static time_type now() {
        return time_type::clock::now();
    }
 
    static time_type add(time_type const & t, duration_type const & d) {
        return t + static_cast<duration_type::chrono_duration>(d);
    }
 
    static duration_type subtract(time_type const & t1, time_type const & t2) {
        return t1 - t2;
    }
 
    static bool less_than(time_type const & t1, time_type const & t2) {
        return t1 < t2;
    }
 
    static boost::posix_time::time_duration to_posix_duration(duration_type const & d) {
        return static_cast<boost::posix_time::time_duration>(d);
    }
};
 
typedef boost::asio::basic_deadline_timer<
    steady_time_traits::time_type
  , steady_time_traits
> relative_timer;

#endif // RELATIVE_TIMER_HPP
