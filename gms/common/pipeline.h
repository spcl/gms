#pragma once
#ifndef GMS_PIPELINE_H
#define GMS_PIPELINE_H

#include <tuple>
#include <vector>
#include <string>
#include <sstream>

#include <gms/third_party/gapbs/timer.h>
#include <gms/third_party/gapbs/command_line.h>

#include "printer.h"


namespace GMS {

/**
 * @brief Pipeline to run bench. Takes a collection of functions, calls them in order and reports timings.
 * 
 * The output of the functions are chained. Functions must return a tuple (an empty tuple if no output) that
 * will be used as input for the next function in the chain. Each function-execution is timed and reported at the end
 * by the printer. Outputs of the functions are not reported.
 * 
 * If your time measurements are extremely sensitive or your memory requirements are very tight, it might be better
 * to use a custom benchmarking function.
 *  
 * @tparam Functions 
 */
class Pipeline
{
private:

    std::vector<std::string> _printInfos;

    template<unsigned int N, unsigned int I, typename Pipeline>
    struct PipeIt
    {
        template<typename Functions>
        static void Pipe(Pipeline *pl, Functions functions)
        {
            auto func = std::get<I>(functions);
            pl->LocalTimer.Start();
            (pl->*func)();
            //decltype(auto) result = std::apply(func, params);
            pl->LocalTimer.Stop();
            pl->LocalPrinter << pl->LocalTimer.Seconds();
            PipeIt<N-1,I+1,Pipeline>::Pipe(pl, functions);
        }

        // static void Pipe(Pipeline_t& pl)
        // {
        //     auto func = std::get<I>(pl._pipeline);
        //     pl.LocalTimer.Start();
        //     func();
        //     pl.LocalTimer.Stop();
        //     pl.LocalPrinter << pl.LocalTimer.Seconds();
        //     PipeIt<N-1,I+1,Pipeline_t>::Pipe(pl);
        // }
    };

    template<unsigned int I, typename Pipeline>
    struct PipeIt<0, I, Pipeline>
    {
        template<typename Functions>
        static void Pipe(Pipeline *pl, Functions functions)
        {}

        // static void Pipe(Pipeline_t  &pl)
        // {}
    };

    template<typename Info, typename...PrintInfos>
    void BufferInfo(Info info, PrintInfos...printInfos)
    {
        std::stringstream ss;
        ss << info;
        _printInfos.push_back(ss.str());
        BufferInfo(printInfos...);
    }

    void BufferInfo()
    {}

public:
    /**
     * @brief Simple timer instance. Relies on std::high_resolution_clock.
     *
     */
    Timer LocalTimer;

    /**
     * @brief Printer instance for output. See 'Printer' for possible configuration.
     *
     */
    Printer LocalPrinter;

    // constexpr Pipeline(PrintInfos... printInfos)
    // : _printInfos(std::tuple<PrintInfos...>(printInfos...)),
    //   LocalTimer(Timer()), LocalPrinter(Printer())
    // {}
    Pipeline() {}


    /**
     * @brief Run the pipeline with initial arguments params and additional output with printInfos
     *
     * @tparam Param_t
     * @tparam PrintInfos
     * @param cli Holds parameters for the run.
     * @param params Arguments to the first function in the pipeline. Must be a std::tuple (which will get unpacked)
     * @param printInfos Additional information. Must be writable to a stringstream.
     */
    // template<typename Param_t, typename...PrintInfos>
    // void RunWithParams(BenchCLApp &cli, Param_t params, PrintInfos...printInfos)
    // {
    //     for(int i = 0; i < cli.num_trials(); i++)
    //     {
    //         PipeIt<std::tuple_size<std::tuple<Functions...>>::value, 0, Pipeline<Functions...>>::Pipe(*this, params);
    //         LocalPrinter.Enqueue(printInfos...);
    //         std::cout << LocalPrinter << std::endl;
    //     }
    // }

    template<typename...PrintInfos>
    void SetPrintInfo(PrintInfos...printInfos)
    {
        _printInfos.clear();
        BufferInfo(printInfos...);
    }

    template<typename DerivedType, typename...Functions>
    void Run(BenchCLApp &cli, Functions...functions)
    {
        std::tuple<Functions...> tfuncs(functions...);
        for(int i = 0; i < cli.num_trials(); i++)
        {
            DerivedType *derived = static_cast<DerivedType*>(this);
            PipeIt<std::tuple_size<std::tuple<Functions...>>::value, 0, DerivedType>::Pipe(derived, tfuncs);
            for( std::string info : _printInfos)
            {
                LocalPrinter << info;
            }
            std::cout << LocalPrinter << std::endl;
        }
    }

    template<typename DerivedType, typename...Functions>
    void Run(const CLI::Args &args, Functions...functions)
    {
        std::tuple<Functions...> tfuncs(functions...);
        for(int i = 0; i < args.num_trials; i++)
        {
            DerivedType *derived = static_cast<DerivedType*>(this);
            PipeIt<std::tuple_size<std::tuple<Functions...>>::value, 0, DerivedType>::Pipe(derived, tfuncs);
            for( std::string info : _printInfos)
            {
                LocalPrinter << info;
            }
            std::cout << LocalPrinter << std::endl;
        }
    }
};

} // namespace GMS

#endif