#pragma once

#include <memory>
#include <set>
#include <vector>

#include "motis/rsl/capacity.h"
#include "motis/rsl/graph.h"

namespace motis::rsl {

struct rsl_data {
  graph graph_;

  std::set<passenger_group*> groups_affected_by_last_update_;
  capacity_map_t capacity_map_;
};

}  // namespace motis::rsl
