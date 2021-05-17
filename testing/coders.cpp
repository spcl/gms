#include "test_helper.h"
#include <gms/representations/graphs/coders/coders-utils/varint_utils.h>

using testing::UnorderedElementsAre;

TEST(CodersUtils, ToVarint) {
    std::vector<uint64_t> input{2, 4, 10, 100000, 300};
    unsigned char *out = new unsigned char[8];

    ASSERT_EQ(toVarint(input[0], out), 1);
    ASSERT_EQ(toVarint(input[1], out), 1);
    ASSERT_EQ(toVarint(input[2], out), 1);
    ASSERT_EQ(toVarint(input[3], out), 3);
    ASSERT_EQ(toVarint(input[4], out), 2);
    
    delete[] out;
}

TEST(CodersUtils, ToVarintRoundTrip) {
    std::vector<uint64_t> input{2, 4, 10, 100000, 300};
    unsigned char *out = new unsigned char[8];

    int size = 0;
    size += toVarint(input[0], out + size);
    size += toVarint(input[1], out + size);
    size += toVarint(input[2], out + size);
    size += toVarint(input[3], out + size);
    size += toVarint(input[4], out + size);

    int offset = 0;
    uint64_t *decoded = new uint64_t[5];

    offset += fromVarint(out + offset, decoded);
    ASSERT_EQ(decoded[0], 2);
    offset += fromVarint(out + offset, &decoded[1]);
    ASSERT_EQ(decoded[1], 4);
    offset += fromVarint(out + offset, &decoded[2]);
    ASSERT_EQ(decoded[2], 10);
    offset += fromVarint(out + offset, &decoded[3]);
    ASSERT_EQ(decoded[3], 100000);
    offset += fromVarint(out + offset, &decoded[4]);
    ASSERT_EQ(decoded[4], 300);

    delete[] out;
    delete[] decoded;
}

TEST(CodersUtils, ToVarint64Bit)
{
    int64_t num;
    unsigned char out[10];
    uint64_t decoded[2];

    num = 42;
    toVarint(num, &out[0]);
    fromVarint(&out[0], &decoded[0]);
    ASSERT_EQ(decoded[0], 42);

    // 2^42
    num = 4398046511104;
    toVarint(num, &out[0]);
    fromVarint(&out[0], &decoded[0]);
    ASSERT_EQ(decoded[0], 4398046511104);
}

TEST(CodersNeighborhoods, VarintNeighborhoodFromCSR) {
    using namespace GMS::CLI;
    Args args;
    args.symmetrize = true;
    Builder builder((GapbsCompat(args)));
    pvector<EdgePair<NodeId, NodeId>> el;
    el.push_back(EdgePair(0, 10));
    el.push_back(EdgePair(0, 500));
    el.push_back(EdgePair(0, 3000));

    auto check_neigh = [](auto &g) {
        std::vector<NodeId> neigh_vec;
        for (NodeId v : g.out_neigh(0)) {
            neigh_vec.push_back(v);
        }

        ASSERT_THAT(neigh_vec, UnorderedElementsAre(10, 500, 3000));
    };

    auto csrgraph = builder.MakeGraphFromEL(el);
    check_neigh(csrgraph);
    auto bytegraph = builder.csrToCGraphGeneric<VarintByteBasedGraph>(csrgraph);
    check_neigh(bytegraph);
    auto wordgraph = builder.csrToCGraphGeneric<VarintWordBasedGraph>(csrgraph);
    check_neigh(wordgraph);
}

TEST(CodersNeighborhoods, VarintNeighborhoodFromCSRLastNeigh) {
    using namespace GMS::CLI;
    Args args;
    args.symmetrize = true;
    Builder builder((GapbsCompat(args)));
    pvector<EdgePair<NodeId, NodeId>> el;
    el.push_back(EdgePair(3, 0));
    el.push_back(EdgePair(3, 1));
    el.push_back(EdgePair(3, 2));

    auto check_neigh = [](auto &g) {
        std::vector<NodeId> neigh_vec;
        for (NodeId v : g.out_neigh(3)) {
            neigh_vec.push_back(v);
        }

        ASSERT_THAT(neigh_vec, UnorderedElementsAre(0, 1, 2));
    };

    auto csrgraph = builder.MakeGraphFromEL(el);
    check_neigh(csrgraph);
    auto bytegraph = builder.csrToCGraphGeneric<VarintByteBasedGraph>(csrgraph);
    check_neigh(bytegraph);
    auto wordgraph = builder.csrToCGraphGeneric<VarintWordBasedGraph>(csrgraph);
    check_neigh(wordgraph);
}

/*
TEST(CodersNeighborhoods, ByteBasedNeighborhood) {
    // TODO(doc):
    //  VarintByteBasedGraph packs neighborhood of node i as concatenation of the following bytes
    //    [ENC(deg[i]), pack_first(i, N(i, 0)), ENC(diff(i, 1)), ..., ENC(diff(i, deg[i] - 1))]
    //  where
    //    pack_first(i, n) = i < n : [0x00, ENC(i - n)] : [0x01, ENC(n - i)]
    //    ENC(uint64_t x) = toVarint variable-length encoding of x
    //    diff(i, n) = N(i, n) - N(i, n - 1)

    NodeId n = 0;
    uint64_t offsets[1];
    unsigned char *adj_data = new unsigned char[200];

    int size = 0;
    // deg = 3
    size += toVarint(3 - 0, adj_data + size);
    // 10 > node 0
    adj_data[size++] = NEXT_VAL_GREATER;
    offsets[0] = size;
    // 10 - node 0
    size += toVarint(10 - 0, adj_data + size);
    size += toVarint(500 - 10, adj_data + size);
    size += toVarint(3000 - 500, adj_data + size);
    
    VarintByteBasedGraph::CompressedNeighbourhood neighborhood(n, adj_data, offsets);
    std::vector<NodeId> neigh_vec;
    for (NodeId v : neighborhood) {
        neigh_vec.push_back(v);
    }

    ASSERT_THAT(neigh_vec, UnorderedElementsAre(10, 500, 3000));
    
    delete[] adj_data;
}
*/
/*
TEST(CodersNeighbors, WordBasedNeighborhood) {
    // TODO the code could become more modular and unit testable by extracting these encoder operations (for buffers)

    NodeId n = 0;
    uint64_t offsets[1] = {0};
    unsigned char *adj_data = new unsigned char[50];

    int size = 0;
    size += toVarint(3, adj_data + size);
    adj_data[size++] = NEXT_VAL_GREATER;
    size += toVarint(10 - 3, adj_data + size);
    size += toVarint(500 - 10, adj_data + size);
    size += toVarint(3000 - 500, adj_data + size);

    VarintWordBasedGraph::CompressedNeighbourhood neighborhood(n, adj_data, offsets);
    std::vector<NodeId> neigh_vec;
    for (NodeId v : neighborhood) {
        neigh_vec.push_back(v);
    }

    ASSERT_THAT(neigh_vec, UnorderedElementsAre(10, 500, 3000));

    delete[] adj_data;
}*/