#ifndef ABSTRACTIONOPTIMIZING_TESTS_MINEBNECH_CLIQUECOUNTING_UCLAPP_H
#define ABSTRACTIONOPTIMIZING_TESTS_MINEBNECH_CLIQUECOUNTING_UCLAPP_H

#include <gms/third_party/gapbs/gapbs.h>
#include <gtest/gtest.h>
#include <gms/algorithms/non_set_based/k_clique_list/clique_counting.h>

class UCLApp : public CLApp
{
public:
    UCLApp(int argc, char** argv, std::string name) :
    CLApp(argc, argv, name)
    {
        this->symmetrize_ = true;
    }
};

class ToSpecialSparseTest : public ::testing::Test
{
protected:
public:

    ToSpecialSparseTest() {}
    ~ToSpecialSparseTest() {}

    virtual void SetUp() override
    {
    }

    virtual void TearDown() override
    {
    }
};


class FixedCLApp : public GMS::KClique::CLCliqueApp
{
public:
    FixedCLApp(const int cliqueSize) :
        CLCliqueApp(GMS::CLI::Args(), GMS::CLI::Param(std::make_shared<std::string>(std::to_string(cliqueSize))))
    {
        this->clique_size_ = cliqueSize;
    }
};

#endif