#include "motis/rsl/reachability.h"

#include "utl/enumerate.h"
#include "utl/verify.h"

#include "motis/core/access/trip_access.h"

namespace motis::rsl {

reachability_info get_reachability(rsl_data const& data, schedule const& sched,
                                   compact_journey const& j) {
  utl::verify(!j.legs_.empty(), "empty journey");

  auto reachability = reachability_info{};
  auto ok = true;
  auto const& first_leg = j.legs_.front();
  auto station_arrival_time = first_leg.enter_time_;

  reachability.reachable_interchange_stations_.emplace_back(
      reachable_station{first_leg.enter_station_id_, first_leg.enter_time_,
                        first_leg.enter_time_});

  for (auto const& [leg_idx, leg] : utl::enumerate(j.legs_)) {
    auto const td = data.graph_.trip_data_.at(leg.trip_).get();
    auto in_trip = false;
    auto entry_ok = false;
    auto exit_ok = false;
    for (auto [edge_idx, e] : utl::enumerate(td->edges_)) {
      if (!in_trip) {
        if (e->from_->station_ == leg.enter_station_id_ &&
            e->from_->schedule_time_ == leg.enter_time_) {
          auto required_arrival_time_at_station = e->from_->time_;
          if (leg.enter_transfer_) {
            required_arrival_time_at_station -= leg.enter_transfer_->duration_;
          }
          if (station_arrival_time > required_arrival_time_at_station) {
            ok = false;
            break;
          }
          in_trip = true;
          reachability.reachable_trips_.emplace_back(reachable_trip{
              get_trip(sched, leg.trip_), td, &leg, e->from_->time_,
              INVALID_TIME, edge_idx, reachable_trip::INVALID_INDEX});
          entry_ok = true;
        }
      }
      if (in_trip) {
        if (e->to_->schedule_time_ > leg.exit_time_) {
          ok = false;
          break;
        }
        if (e->to_->station_ == leg.exit_station_id_ &&
            e->to_->schedule_time_ == leg.exit_time_) {
          station_arrival_time = e->to_->time_;
          auto const station_ic_time =
              sched.stations_[e->to_->station_]->transfer_time_;
          // TODO(pablo): footpaths
          auto& rt = reachability.reachable_trips_.back();
          rt.exit_real_time_ = station_arrival_time;
          rt.exit_edge_idx_ = edge_idx;
          reachability.reachable_interchange_stations_.emplace_back(
              reachable_station{
                  e->to_->station_,
                  static_cast<time>(e->to_->schedule_time_ + station_ic_time),
                  static_cast<time>(station_arrival_time + station_ic_time)});
          exit_ok = true;
          break;
        }
      }
    }
    if (!entry_ok || !exit_ok) {
      ok = false;
    }
    if (!ok) {
      auto const is_first_leg = leg_idx == 0;
      auto const is_last_leg = leg_idx == j.legs_.size() - 1;
      if (!entry_ok) {
        if (is_first_leg) {
          reachability.status_ = reachability_status::BROKEN_INITIAL_ENTRY;
        } else {
          reachability.status_ = reachability_status::BROKEN_TRANSFER_ENTRY;
        }
      } else if (!exit_ok) {
        if (is_last_leg) {
          reachability.status_ = reachability_status::BROKEN_FINAL_EXIT;
        } else {
          reachability.status_ = reachability_status::BROKEN_TRANSFER_EXIT;
        }
      }
      break;
    }
  }

  reachability.ok_ = ok;
  return reachability;
}

}  // namespace motis::rsl
