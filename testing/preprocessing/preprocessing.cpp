#include "degeneracy_order_tests.h"

int main(int argc, char **argv) {
    //omp_set_num_threads(1);

    // TODO determinism and improvements in genera
    std::cout << "The tests aren't deterministic!" << std::endl;
    std::cout << "Running main() from " << __FILE__ << std::endl;

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}