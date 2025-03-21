#ifndef OSRM_GUIDANCE_MOTORWAY_HANDLER_HPP_
#define OSRM_GUIDANCE_MOTORWAY_HANDLER_HPP_

#include "extractor/name_table.hpp"

#include "guidance/intersection.hpp"
#include "guidance/intersection_handler.hpp"
#include "guidance/is_through_street.hpp"

#include "util/node_based_graph.hpp"

#include <vector>

namespace osrm::guidance
{

// Intersection handlers deal with all issues related to intersections.
class MotorwayHandler final : public IntersectionHandler
{
  public:
    MotorwayHandler(const util::NodeBasedDynamicGraph &node_based_graph,
                    const extractor::EdgeBasedNodeDataContainer &node_data_container,
                    const std::vector<util::Coordinate> &coordinates,
                    const extractor::CompressedEdgeContainer &compressed_geometries,
                    const extractor::RestrictionMap &node_restriction_map,
                    const extractor::ObstacleMap &obstacle_nodes,
                    const extractor::TurnLanesIndexedArray &turn_lanes_data,
                    const extractor::NameTable &name_table,
                    const extractor::SuffixTable &street_name_suffix_table);

    ~MotorwayHandler() override final = default;

    // check whether the handler can actually handle the intersection
    bool canProcess(const NodeID nid,
                    const EdgeID via_eid,
                    const Intersection &intersection) const override final;

    // process the intersection
    Intersection operator()(const NodeID nid,
                            const EdgeID via_eid,
                            Intersection intersection) const override final;

  private:
    [[nodiscard]] Intersection handleSliproads(const NodeID intersection_node_id,
                                               Intersection intersection) const;

    [[nodiscard]] Intersection fromMotorway(const EdgeID via_edge, Intersection intersection) const;

    [[nodiscard]] Intersection fromRamp(const EdgeID via_edge, Intersection intersection) const;

    [[nodiscard]] Intersection fallback(Intersection intersection) const;
};

} // namespace osrm::guidance

#endif /*OSRM_GUIDANCE_MOTORWAY_HANDLER_HPP_*/
