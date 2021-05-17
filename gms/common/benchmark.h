#pragma once

#include <string>
#include <gms/third_party/gapbs/util.h>
#include <gms/third_party/gapbs/timer.h>

#include "cli/args.h"

// This file contains various versions of BenchmarkKernel functions, inspired by the original GAPBS BenchmarkKernel
// function, which has been renamed to BenchmarkKernelLegacy (in `third_party/gapbs/benchmark.h`), but with several
// extensions depending on the function:
// - use CLI::Args for parameters
// - support for SetGraph
// - support for different preprocessing functions

namespace GMS {

// Calls (and times) GAPBSF according to command line arguments
template <typename GraphT_, typename GAPBSFunc,
        typename VerifierFunc, class... print_T>
void BenchmarkKernel(const CLI::Args &args, const GraphT_ &g,
                     GAPBSFunc GAPBSF,
                     VerifierFunc verify, print_T... printInfo)
{
    static_assert(string_check<std::is_convertible<print_T, std::string>::value...>::value, "printInfo not convertible to string!");
    g.PrintStats();
    double total_seconds = 0;
    Timer trial_timer;
    for (int iter = 0; iter < args.num_trials; iter++) {
        trial_timer.Start();
        auto result = GAPBSF(g);
        trial_timer.Stop();
        PrintTime("Trial Time", trial_timer.Seconds());
        const double trialTime = trial_timer.Seconds();
        total_seconds += trial_timer.Seconds();
        if (args.verify) {
            trial_timer.Start();
            std::string verifyMark = verify(std::ref(g), std::ref(result)) ? "PASS" : "FAIL";
            PrintLabel("Verification",
                       verifyMark);
            trial_timer.Stop();
            PrintTime("Verification Time", trial_timer.Seconds());
            const double verifyTime = trial_timer.Seconds();

            PrintBenchmarkOutput("@@@", trialTime, verifyMark, verifyTime, printInfo...);
        }
        else
        {
            PrintBenchmarkOutput("@@@", trialTime, printInfo...);
        }
    }
    PrintTime("Average Time", total_seconds / args.num_trials);
}

//benchmarking kernels with additional integer parameter (e.g., clique size)
//non-const version of the graph argument
template <typename GraphT_, typename Func,
        typename VerifierFunc, class... print_T>
void BenchmarkKernel(const CLI::Args &args, GraphT_ &g,
                     Func GraphF,
                     VerifierFunc verify,
                     size_t param,
                     print_T... printInfo)
{
    static_assert(string_check<std::is_convertible<print_T, std::string>::value...>::value, "printInfo not convertible to string!");
    g.PrintStats();
    double total_seconds = 0;
    Timer trial_timer;
    for (int iter = 0; iter < args.num_trials; iter++) {
        trial_timer.Start();
        auto result = GraphF(g, param);
        trial_timer.Stop();
        PrintTime("Trial Time", trial_timer.Seconds());
        const double trialTime = trial_timer.Seconds();
        total_seconds += trial_timer.Seconds();
        if (args.verify) {
            trial_timer.Start();
            std::string verifyMark = verify(std::ref(g), std::ref(result), param) ? "PASS" : "FAIL";
            PrintLabel("Verification", verifyMark);
            trial_timer.Stop();
            PrintTime("Verification Time", trial_timer.Seconds());
            const double verifyTime = trial_timer.Seconds();

            PrintBenchmarkOutput("@@@", trialTime, verifyMark, verifyTime, printInfo...);
        } else {
            PrintBenchmarkOutput("@@@", trialTime, printInfo...);
        }
    }
    PrintTime("Average Time", total_seconds / args.num_trials);
}

//Added by Zur 11.01.2019 for better controlling of RoaringGraph building
// Calls (and times) GAPBSF according to command line arguments
template <typename GraphExec, typename GraphT_, typename GAPBSFunc,
        typename VerifierFunc, class... print_T>
void BenchmarkKernelBk(const CLI::Args &args, const GraphT_ &g,
                       GAPBSFunc GAPBSF,
                       VerifierFunc verify, print_T... printInfo)
{
    static_assert(string_check<std::is_convertible<print_T, std::string>::value...>::value, "printInfo not convertible to string!");
    g.PrintStats();
    double total_seconds = 0;
    Timer trial_timer;

    //Building Roaring Graph
    trial_timer.Start();
    GraphExec rgraph = GraphExec::FromCGraph(g);
    trial_timer.Stop();
    PrintTime("GraphExec buildTime", trial_timer.Seconds());

    for (int iter = 0; iter < args.num_trials; iter++)
    {
        trial_timer.Start();
        auto result = GAPBSF(rgraph);
        trial_timer.Stop();
        PrintTime("Trial Time", trial_timer.Seconds());
        const double trialTime = trial_timer.Seconds();
        total_seconds += trial_timer.Seconds();
        if (args.verify)
        {
            trial_timer.Start();
            std::string verifyMark = verify(std::ref(g), std::ref(result)) ? "PASS" : "FAIL";
            PrintLabel("Verification",
                       verifyMark);
            trial_timer.Stop();
            PrintTime("Verification Time", trial_timer.Seconds());
            const double verifyTime = trial_timer.Seconds();

            PrintBenchmarkOutput("@@@", trialTime, verifyMark, verifyTime, printInfo...);
        }
        else
        {
            PrintBenchmarkOutput("@@@", trialTime, printInfo...);
        }
    }
    PrintTime("Average Time", total_seconds / args.num_trials);
}

//Added by Zur 11.02.2020,
// allows to choose set-based kernel as well as preprocessing function.
template <typename GraphExec, typename GraphT_, typename GAPBSFunc, typename PPFunc,
        typename VerifierFunc, class... print_T>
void BenchmarkKernelBkPP(const CLI::Args &args, const GraphT_ &g,
                         PPFunc preprocess,
                         GAPBSFunc GAPBSF,
                         VerifierFunc verify,
                         print_T... printInfo)
{
    static_assert(string_check<std::is_convertible<print_T, std::string>::value...>::value, "printInfo not convertible to string!");
    g.PrintStats();
    double total_seconds = 0;
    double pp_total_seconds = 0;
    double vv_total_seconds = 0;
    Timer trial_timer;

    //Building Roaring Graph
    trial_timer.Start();
    GraphExec rgraph = GraphExec::FromCGraph(g);
    trial_timer.Stop();
    PrintTime("GraphExec buildTime", trial_timer.Seconds());

    for (int iter = 0; iter < args.num_trials; iter++) {
        // do preprocessing
        trial_timer.Start();
        pvector<NodeId> order(rgraph.num_nodes());
        preprocess(rgraph, order);
        trial_timer.Stop();
        PrintTime("Preprocess Time", trial_timer.Seconds());
        const double preprocTime = trial_timer.Seconds();
        pp_total_seconds += trial_timer.Seconds();

        trial_timer.Start();
        auto result = GAPBSF(rgraph, order);
        trial_timer.Stop();
        PrintTime("Trial Time", trial_timer.Seconds());
        const double trialTime = trial_timer.Seconds();
        total_seconds += trial_timer.Seconds();
        if (args.verify) {
            trial_timer.Start();
            std::string verifyMark = verify(std::ref(g), std::ref(result)) ? "PASS" : "FAIL";
            PrintLabel("Verification",
                       verifyMark);
            trial_timer.Stop();
            PrintTime("Verification Time", trial_timer.Seconds());
            const double verifyTime = trial_timer.Seconds();
            vv_total_seconds += trial_timer.Seconds();

            PrintBenchmarkOutput("@@@", trialTime, verifyMark, verifyTime, preprocTime, printInfo...);
        } else {
            PrintBenchmarkOutput("@@@", trialTime, preprocTime, printInfo...);
        }
    }
    PrintTime("Average pp Time", pp_total_seconds / args.num_trials);
    PrintTime("Average Time", total_seconds / args.num_trials);
    PrintTime("Average Verification Time", vv_total_seconds / args.num_trials);
}

// Calls (and times) GAPBSF according to command line arguments
// and performs preprocessing specified in preprocess
// added by Yannick Schaffner, 4.11.2019
template <typename GraphT_, typename GAPBSFunc, typename PPFunc,
        typename VerifierFunc, class... print_T>
void BenchmarkKernelPP(const CLApp &cli, const GraphT_ &g,
                       PPFunc preprocess,
                       GAPBSFunc GAPBSF,
                       VerifierFunc verify,
                       print_T... printInfo)
{
    static_assert(string_check<std::is_convertible<print_T, std::string>::value...>::value, "printInfo not convertible to string!");
    g.PrintStats();
    double total_seconds = 0;
    double pp_total_seconds = 0;
    double vv_total_seconds = 0;
    Timer trial_timer;
    for (int iter = 0; iter < cli.num_trials(); iter++)
    {
        // do preprocessing
        trial_timer.Start();
        auto ppG = preprocess(g, cli);
        trial_timer.Stop();
        PrintTime("Preprocess Time", trial_timer.Seconds());
        const double preprocTime = trial_timer.Seconds();
        pp_total_seconds += trial_timer.Seconds();

        trial_timer.Start();
        auto result = GAPBSF(ppG, cli);
        trial_timer.Stop();
        PrintTime("Trial Time", trial_timer.Seconds());
        const double trialTime = trial_timer.Seconds();
        total_seconds += trial_timer.Seconds();
        // if (cli.do_analysis() && (iter == (cli.num_trials()-1)))
        //   gapbsF(g, result);
        if (cli.do_verify())
        {
            trial_timer.Start();
            std::string verifyMark = verify(std::ref(ppG), std::ref(result), cli) ? "PASS" : "FAIL";
            PrintLabel("Verification",
                       verifyMark);
            trial_timer.Stop();
            PrintTime("Verification Time", trial_timer.Seconds());
            const double verifyTime = trial_timer.Seconds();
            vv_total_seconds += trial_timer.Seconds();

            PrintBenchmarkOutput("@@@", trialTime, verifyMark, verifyTime, preprocTime, printInfo...);
        }
        else
        {
            PrintBenchmarkOutput("@@@", trialTime, preprocTime, printInfo...);
        }
    }
    PrintTime("Average pp Time", pp_total_seconds / cli.num_trials());
    PrintTime("Average Time", total_seconds / cli.num_trials());
    PrintTime("Average Verification Time", vv_total_seconds / cli.num_trials());
}

/*
// Calls (and times) kernel according to command line arguments
// and performs preprocessing specified in preprocess
// added by Yannick Schaffner, 4.11.2019
template<typename GraphT_,
          typename KernelFunc, typename KernelSetupFunc,
         typename VerifyFunc, typename VerifySetupFunc, typename VerifyTearDownFunc,
         class ... print_T>
void BenchmarkKernelPPP(const CLI::Args &args, const GraphT_ &g,
                     KernelSetupFunc kernelSetup,
                     KernelFunc kernel,
                     VerifySetupFunc verifySetup,
                     VerifyFunc verify,
                     VerifyTearDownFunc verifyTeardown,
                     print_T ... printInfo) {
  static_assert( string_check<std::is_convertible<print_T, std::string>::value...>::value, "printInfo not convertible to string!");
  g.PrintStats();
  double total_seconds = 0;
  double pp_total_seconds = 0;
  double vv_total_seconds = 0;
  Timer trial_timer;
  for (int iter=0; iter < args.num_trials; iter++) {
    // do preprocessing
    trial_timer.Start();
    auto ppG = kernelSetup(g, args);
    trial_timer.Stop();
    PrintTime("Preprocess Time", trial_timer.Seconds());
    const double setupTime = trial_timer.Seconds();
    pp_total_seconds += trial_timer.Seconds();

    trial_timer.Start();
    auto result = kernel(ppG, args);
    trial_timer.Stop();
    PrintTime("Trial Time", trial_timer.Seconds());
    const double trialTime = trial_timer.Seconds();
    total_seconds += trial_timer.Seconds();

    if (args.verify) {
      auto vppG = verifySetup(ppG, g, args);

      trial_timer.Start();
      std::string verifyMark = verify(vppG, std::ref(result), args) ? "PASS" : "FAIL";
      PrintLabel("Verification",
                 verifyMark);
      trial_timer.Stop();
      PrintTime("Verification Time", trial_timer.Seconds());
      const double verifyTime = trial_timer.Seconds();
      vv_total_seconds += trial_timer.Seconds();

      verifyTeardown(vppG, args);

      PrintBenchmarkOutput("@@@", trialTime, verifyMark, verifyTime, setupTime, printInfo...);
    }
    else {
      PrintBenchmarkOutput("@@@", trialTime, setupTime, printInfo...);
    }
  }
  PrintTime("Average pp Time", pp_total_seconds/ args.num_trials);
  PrintTime("Average Time", total_seconds / args.num_trials);
  PrintTime("Average Verification Time", vv_total_seconds/ args.num_trials);
}
*/

}