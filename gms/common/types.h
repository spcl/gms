#pragma once

#include <cinttypes>
#include <type_traits>

namespace GMS {

/// Default type for graph node ids.
using NodeId = int32_t;

/**
 * Can be used for static assertions which should never be compiled.
 *
 * See for more information: https://stackoverflow.com/a/53945549/595304
 */
template<class...> constexpr std::false_type always_false{};

} // namespace gms

// TODO(namespaces) this is mainly transitional and for GAPBS code
using NodeId = GMS::NodeId;

#include <gms/third_party/gapbs/graph.h>
