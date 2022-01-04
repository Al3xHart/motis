#include "motis/routing/routing.h"

#include "boost/date_time/gregorian/gregorian_types.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/program_options.hpp"

#include "utl/to_vec.h"
#include "utl/zip.h"

#include "motis/core/common/logging.h"
#include "motis/core/common/timing.h"
#include "motis/core/schedule/schedule.h"
#include "motis/core/access/edge_access.h"
#include "motis/core/conv/trip_conv.h"
#include "motis/core/journey/journeys_to_message.h"

#include "motis/routing/additional_edges.h"
#include "motis/routing/build_query.h"
#include "motis/routing/error.h"
#include "motis/routing/eval/commands.h"
#include "motis/routing/label/configs.h"
#include "motis/routing/mem_manager.h"
#include "motis/routing/mem_retriever.h"
#include "motis/routing/search.h"
#include "motis/routing/search_dispatch.h"
#include "motis/routing/start_label_generators/ontrip_gen.h"
#include "motis/routing/start_label_generators/pretrip_gen.h"

// 64MB default start size
constexpr auto LABEL_STORE_START_SIZE = 64 * 1024 * 1024;

using namespace motis::logging;
using namespace motis::module;

namespace motis::routing {

routing::routing() : module("Routing", "routing") {}

routing::~routing() = default;

void routing::reg_subc(motis::module::subc_reg& r) {
  r.register_cmd("print", "prints journeys", eval::print);
  r.register_cmd("generate", "generate routing queries", eval::generate);
  r.register_cmd("rewrite", "rewrite query targets", eval::rewrite_queries);
  r.register_cmd("analyze", "print result statistics", eval::analyze_results);
  r.register_cmd("compare", "print difference between results", eval::compare);
  r.register_cmd("xtract", "extract timetable from connections", eval::xtract);
}

void routing::init(motis::module::registry& reg) {
  reg.register_op("/routing",
                  [this](msg_ptr const& msg) { return route(msg); });
  reg.register_op("/trip_to_connection", [this](msg_ptr const& msg) {
    return trip_to_connection(msg);
  });
}

msg_ptr routing::route(msg_ptr const& msg) {
  MOTIS_START_TIMING(routing_timing);

  auto const req = motis_content(RoutingRequest, msg);
  auto const& sched = get_sched();
  auto query = build_query(sched, req);

  mem_retriever mem(mem_pool_mutex_, mem_pool_, LABEL_STORE_START_SIZE);
  query.mem_ = &mem.get();

  auto res = search_dispatch(query, req->start_type(), req->search_type(),
                             req->search_dir());

  MOTIS_STOP_TIMING(routing_timing);
  res.stats_.total_calculation_time_ = MOTIS_TIMING_MS(routing_timing);
  res.stats_.labels_created_ = query.mem_->allocations();
  res.stats_.num_bytes_in_use_ = query.mem_->get_num_bytes_in_use();

  message_creator fbb;
  std::vector<flatbuffers::Offset<Statistics>> stats{
      to_fbs(fbb, "routing", res.stats_)};
  fbb.create_and_finish(
      MsgContent_RoutingResponse,
      CreateRoutingResponse(
          fbb, fbb.CreateVectorOfSortedTables(&stats),
          fbb.CreateVector(utl::to_vec(
              res.journeys_,
              [&](journey const& j) { return to_connection(fbb, j); })),
          motis_to_unixtime(sched, res.interval_begin_),
          motis_to_unixtime(sched, res.interval_end_),
          fbb.CreateVector(
              std::vector<flatbuffers::Offset<DirectConnection>>{}))
          .Union());
  return make_msg(fbb);
}

msg_ptr routing::trip_to_connection(msg_ptr const& msg) {
  using label = default_label<search_dir::FWD>;

  auto const& sched = get_sched();
  auto trp = from_fbs(sched, motis_content(TripId, msg));

  if (trp.trp_->edges_->empty()) {
    throw std::system_error(access::error::service_not_found);
  }

  auto const first = trp.trp_->edges_->front()->from_;
  auto const last = trp.trp_->edges_->back()->to_;

  auto const e_0 = make_foot_edge(nullptr, first->get_station());
  auto const e_1 = make_enter_edge(first->get_station(), first);
  auto const e_n = make_exit_edge(last, last->get_station());

  auto const dep_time = trp.get_first_dep_time();

  auto const make_label = [&](label* pred, edge const* e,
                              light_connection const* lcon, day_idx_t const day,
                              time now) {
    auto l = label();
    l.pred_ = pred;
    l.edge_ = e;
    l.connection_ = lcon;
    l.day_ = day;
    l.start_ = dep_time;
    l.now_ = now;
    l.dominated_ = false;
    return l;
  };

  auto labels = std::vector<label>{trp.trp_->edges_->size() + 3};
  labels[0] = make_label(nullptr, &e_0, nullptr, day_idx_t{}, dep_time);
  labels[1] = make_label(&labels[0], &e_1, nullptr, day_idx_t{}, dep_time);

  int i = 2;
  for (auto const& [e, day_offset] :
       utl::zip(*trp.trp_->edges_, trp.trp_->day_offsets_)) {
    auto const& lcon = get_lcon(e, trp.trp_->lcon_idx_);
    auto const day = trp.day_idx_ + day_offset;
    labels[i] = make_label(&labels[i - 1], e, &lcon, day,
                           lcon.event_time(event_type::ARR, day));
    ++i;
  }

  labels[i] = make_label(&labels[i - 1], &e_n, nullptr, day_idx_t{},
                         labels[i - 1].now_);

  message_creator fbb;
  fbb.create_and_finish(
      MsgContent_Connection,
      to_connection(
          fbb, output::labels_to_journey(sched, &labels[i], search_dir::FWD))
          .Union());
  return make_msg(fbb);
}

}  // namespace motis::routing
