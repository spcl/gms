#include <gms/algorithms/set_based/triangle_count/triangle_count.h>
#include <gms/algorithms/set_based/triangle_count/verifier.h>
#include <gms/common/cli/cli.h>
#include <gms/common/benchmark.h>
#include <gms/representations/graphs/set_graph.h>

using namespace GMS;

class SimpleSet {
public:
    using SetElement = NodeId;
    SimpleSet(const SetElement *data, size_t count) : data(data, data + count) {}

    auto begin() const { return data.begin(); }
    auto end() const { return data.end(); }

    bool contains(SetElement element) const {
        for (NodeId u : *this) {
            if (u == element) {
                return true;
            }
        }
        return false;
    }

    size_t intersect_count(const SimpleSet &set) const {
        size_t count = 0;
        for (NodeId u : *this) {
            count += set.contains(u);
        }
        return count;
    }
protected:
    std::vector<SetElement> data;
};

class BetterSet : public SimpleSet {
public:
    BetterSet(const SetElement *data, size_t count) : SimpleSet(data, count) {
        std::sort(this->data.begin(), this->data.end());
    }

    size_t intersect_count(const SimpleSet &set) const {
        size_t count = 0;
        auto it0 = begin();
        auto it1 = set.begin();
        while (it0 != end() && it1 != set.end()) {
            if (*it0 == *it1) {
                ++count;
                ++it0;
                ++it1;
            } else if (*it0 > *it1) {
                ++it1;
            } else {
                ++it0;
            }
        }
        return count;
    }
};

int main(int argc, char *argv[]) {
    using namespace GMS::CLI;

    auto [args, g] = Parser().parse_and_load(argc, argv);

    using SimpleGraph = SetGraph<SimpleSet>;
    BenchmarkKernelBk<SimpleGraph>(args, g, TriangleCount::Seq::count_total<SimpleGraph>, TriangleCount::Verify::total_count, "SimpleGraph");

    using BetterGraph = SetGraph<BetterSet>;
    BenchmarkKernelBk<BetterGraph>(args, g, TriangleCount::Seq::count_total<BetterGraph>, TriangleCount::Verify::total_count, "BetterGraph");
}