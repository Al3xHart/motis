#pragma once

#include <cstdint>
#include <vector>

namespace motis::rsl {

struct tick_statistics {
  std::uint64_t system_time_{};

  // rt update counts
  std::uint64_t rt_updates_{};
  std::uint64_t rt_delay_updates_{};
  std::uint64_t rt_reroute_updates_{};
  std::uint64_t rt_track_updates_{};
  std::uint64_t rt_free_text_updates_{};

  // rt delay details
  std::uint64_t rt_delay_event_updates_{};
  std::uint64_t rt_delay_is_updates_{};
  std::uint64_t rt_delay_propagation_updates_{};
  std::uint64_t rt_delay_forecast_updates_{};
  std::uint64_t rt_delay_repair_updates_{};
  std::uint64_t rt_delay_schedule_updates_{};

  // affected passengers
  std::uint64_t affected_groups_{};
  std::uint64_t affected_passengers_{};
  std::uint64_t ok_groups_{};
  std::uint64_t broken_groups_{};
  std::uint64_t broken_passengers_{};
  std::uint64_t combined_groups_{};
  std::uint64_t combined_groups_size_{};

  // totals
  std::uint64_t total_ok_groups_{};
  std::uint64_t total_broken_groups_{};

  // tracking
  std::uint64_t tracked_ok_groups_{};
  std::uint64_t tracked_broken_groups_{};
};

}  // namespace motis::rsl
