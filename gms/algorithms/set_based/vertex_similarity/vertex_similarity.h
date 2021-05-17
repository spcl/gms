#pragma once
#include <gms/common/types.h>
#include <cassert>

/**
 * @brief Implementation of several vertex similarity measures.
 *
 * Larger values indicate higher vertex similarity.
 * All metrics are symmetric and non-negative.
 */
namespace GMS::VertexSim {
    
/**
 * Different similarity metric specifiers which can be used with the vertex_similarity template in generic code.
 *
 * See the other functions in this namespace for further information about the particular similarity measure.
 */
enum class Metric {Jaccard, Overlap, AdamicAdar, Resource, CommNeigh, TotalNeigh, PrefAtt};

/**
 * Computes the Jaccard index for two sample sets.
 *
 * @tparam SetA Type of the first set
 * @tparam SetB Type of the second set, usually SetA = SetB
 * @param A The first set
 * @param B The second set
 * @return
 */
template <class SetA, class SetB = SetA>
inline double vertex_similarity_jaccard(const SetA &A, const SetB &B) {
    if (A.cardinality() == 0 && B.cardinality() == 0) {
        return 1.0;
    } else {
        double count = A.intersect_count(B);
        return count / (A.cardinality() + B.cardinality() + count);
    }
}

/**
 * Compute the Jaccard vertex similarity measure for two vertices.
 *
 * @tparam SGraph SetGraph compatible graph representation type
 * @param a ID of the first vertex
 * @param b ID of the second vertex
 * @param g The input graph
 * @return
 */
template <class SGraph>
inline double vertex_similarity_jaccard(NodeId a, NodeId b, const SGraph &g)
{
    return vertex_similarity_jaccard(g.out_neigh(a), g.out_neigh(b));
}

/**
 * Computes the overlap coefficient for two sample sets.
 *
 * @tparam SetA Type of the first set
 * @tparam SetB Type of the second set, usually SetA = SetB
 * @param A The first set
 * @param B The second set
 * @return
 */
template <class SetA, class SetB = SetA>
inline double vertex_similarity_overlap(const SetA &A, const SetB &B) {
    return double(A.intersect_count(B)) / std::min(A.cardinality(), B.cardinality());
}

/**
 * Compute the overlap vertex similarity measure for two vertices.
 *
 * @tparam SGraph SetGraph compatible graph representation type
 * @param a ID of the first vertex
 * @param b ID of the second vertex
 * @param g The input graph
 * @return
 */
template <class SGraph>
inline double vertex_similarity_overlap(NodeId a, NodeId b, const SGraph &g)
{
    return vertex_similarity_overlap(g.out_neigh(a), g.out_neigh(b));
}

/**
 * Computes the Adamic Adar vertex similarity index for two vertices.
 *
 * Originally described in https://doi.org/10.1016/S0378-8733(03)00009-1.
 *
 * @tparam SGraph SetGraph compatible graph representation type
 * @param a ID of the first vertex
 * @param b ID of the second vertex
 * @param g The input graph
 * @return
 */
template <class SGraph>
inline double vertex_similarity_adamic_adar(NodeId a, NodeId b, const SGraph &g)
{
    double sum = 0;
    auto intersection = g.out_neigh(a).intersect(g.out_neigh(b));

    for (NodeId u : intersection) {
        double count = g.out_degree(u);
        sum += 1. / std::log(count);
    }

    return sum;
}

/**
 * Computes the Resource Allocation vertex similarity for two vertices.
 *
 * @tparam SGraph SetGraph compatible graph representation type
 * @param a ID of the first vertex
 * @param b ID of the second vertex
 * @param g The input graph
 * @return
 */
template <class SGraph>
inline double vertex_similarity_resource(NodeId a, NodeId b, const SGraph &g)
{
    auto intersection = g.out_neigh(a).intersect(g.out_neigh(b));
    double s = 0;
    for (auto w : intersection) {
        s += 1.0/ g.out_degree(w);
    }
    return s;
}

/**
 * Computes the Common Neighbors vertex similarity for two vertices. *
 *
 * @tparam SGraph SetGraph compatible graph representation type
 * @param a ID of the first vertex
 * @param b ID of the second vertex
 * @param g The input graph
 * @return
 */
template <class SGraph>
inline double vertex_similarity_common_neighbors(NodeId a, NodeId b, const SGraph &g)
{
    return g.out_neigh(a).intersect_count(g.out_neigh(b));
}

/**
 * Computes the Total Neighbors vertex similarity for two vertices. *
 *
 * @tparam SGraph SetGraph compatible graph representation type
 * @param a ID of the first vertex
 * @param b ID of the second vertex
 * @param g The input graph
 * @return
 */
template <class SGraph>
inline double vertex_similarity_total_neighbors(NodeId a, NodeId b, const SGraph &g)
{
    return g.out_neigh(a).union_count(g.out_neigh(b));
}

/**
 * Computes a preferential attachment index for two sample sets.
 *
 * @tparam SetA Type of the first set
 * @tparam SetB Type of the second set, usually SetA = SetB
 * @param A The first set
 * @param B The second set
 * @return
 */
template <class SetA, class SetB = SetA>
inline double vertex_similarity_preferential_attachment(const SetA &A, const SetB &B) {
    return A.cardinality() * B.cardinality();
}

/**
 * Computes the preferential attachment vertex similarity for two vertices.
 * 
 * Described in https://doi.org/10.1002/asi.20591.
 *
 * @tparam SGraph SetGraph compatible graph representation type
 * @param a ID of the first vertex
 * @param b ID of the second vertex
 * @param g The input graph
 * @return
 */
template <class SGraph>
inline double vertex_similarity_preferential_attachment(NodeId a, NodeId b, const SGraph &g)
{
    return vertex_similarity_preferential_attachment(g.out_neigh(a), g.out_neigh(b));
}

/**
 * Generic helper method to invoke any of the vertex_similarity functions on a pair of vertices,
 * given its `Metric` descriptor.
 * 
 * @tparam metric The metric which should be used, the names are descriptive for the individual vertex similarity
 *                measure used.
 * @tparam SGraph SetGraph compatible graph representation type
 * @param a       ID of the first vertex
 * @param b       ID of the second vertex
 * @param g       The input graph
 * @return 
 */
template <Metric metric, class SGraph>
inline double vertex_similarity(NodeId a, NodeId b, const SGraph &g)
{
    if constexpr (metric == Metric::Jaccard) {
        return vertex_similarity_jaccard(a, b, g);
    } else if constexpr (metric == Metric::Overlap) {
        return vertex_similarity_overlap(a, b, g);
    } else if constexpr (metric == Metric::AdamicAdar) {
        return vertex_similarity_adamic_adar(a, b, g);
    } else if constexpr (metric == Metric::Resource) {
        return vertex_similarity_resource(a, b, g);
    } else if constexpr (metric == Metric::CommNeigh) {
        return vertex_similarity_common_neighbors(a, b, g);
    } else if constexpr (metric == Metric::TotalNeigh) {
        return vertex_similarity_total_neighbors(a, b, g);
    } else if constexpr (metric == Metric::PrefAtt) {
        return vertex_similarity_preferential_attachment(a, b, g);
    } else {
        static_assert(always_false<metric, SGraph>, "invalid similarity measure");
    }
}

} // namespace GMS::VertexSim