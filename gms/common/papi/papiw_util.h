#ifndef PAPIWUTIL
#define PAPIWUTIL
#ifndef NOPAPIW

#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <iostream>
#include <algorithm>
#include <papi.h>
#include <omp.h>
#include <pthread.h>

/**
 * PapiWrapper abstract class
 * 
 * This interface defines the public interface and provides default functionality
 * and further utility functions
 */
class PapiWrapper
{
public:
    virtual ~PapiWrapper() {}

    virtual void AddEvent(const int eventCode) = 0;
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual long long GetResult(const int eventCode) = 0;
    virtual void Print() = 0;
    virtual void Reset() = 0;

    /**
     * Default 
     *
     * @tparam PapiCodes a variadic list of PAPI eventcodes
     * @warning Exits with an error if called in a parallel region
     */
    template <typename... PapiCodes>
    void Init(PapiCodes const... eventcodes)
    {
        /* Initialize the PAPI library */
        retval = PAPI_library_init(PAPI_VER_CURRENT);
        if (retval != PAPI_VER_CURRENT)
            handle_error("Init", "PAPI library init error!\n", retval);

        /* Some more initialization inside the specialization classes*/
        localInit();

        /* Prepare Events */
        static_assert(std::conjunction<std::is_integral<PapiCodes>...>(),
                      "All parameters to Init must be of integral type");
        int args[]{eventcodes...}; //unpack
        for (auto eventcode : args)
            AddEvent(eventcode);
    }

protected:
    int retval;

    virtual void localInit() {}

    /* Exit with an error message */
    void handle_error(const char *location, const char *msg, const int retval = PAPI_OK)
    {
        if (retval == PAPI_OK)
            fprintf(stderr, "PAPI ERROR in %s: %s\n", location, msg);
        else
            fprintf(stderr, "PAPI ERROR (Code %d) in %s: %s\n", retval, location, msg);

        exit(1);
    }

    /* Print a warning message */
    void issue_waring(const char *location, const char *msg, const int retval = PAPI_OK)
    {
        if (retval == PAPI_OK)
            fprintf(stderr, "PAPI WARNING in %s: %s\n", location, msg);
        else
            fprintf(stderr, "PAPI WARNING (Code %d) in %s: %s\n", retval, location, msg);
    }

    /* Print results */
    void print(const std::vector<int> &events, const long long *values)
    {
        for (auto eventCode : events)
            std::cout << getDescription(eventCode) << ": " << GetResult(eventCode) << std::endl;

        /* Print Headers */
        std::cout << "@%% ";
        for (auto eventCode : events)
        {
            auto description = getDescription(eventCode);
            for (int j = 0; description[j] != '\0' && description[j] != ' ' && j < 20; j++)
                std::cout << description[j];
            std::cout << " ";
        }
        std::cout << std::endl;

        /* Print results */
        int count = events.size();
        std::cout << "@%@ ";
        for (int i = 0; i < count; i++)
            std::cout << values[i] << " ";
        std::cout << std::endl;
    }

    /* Get Descriptiion Text of event */
    const char *getDescription(const int eventCode)
    {
        switch (eventCode)
        {
        case PAPI_L1_DCM:
            return "PAPI_L1_DCM (Level 1 data cache misses)";
        case PAPI_L1_ICM:
            return "PAPI_L1_ICM (Level 1 instruction cache misses)";
        case PAPI_L2_DCM:
            return "PAPI_L2_DCM (Level 2 data cache misses)";
        case PAPI_L2_ICM:
            return "PAPI_L2_ICM (Level 2 instruction cache misses)";
        case PAPI_L3_DCM:
            return "PAPI_L3_DCM (Level 3 data cache misses)";
        case PAPI_L3_ICM:
            return "PAPI_L3_ICM (Level 3 instruction cache misses)";
        case PAPI_L1_TCM:
            return "PAPI_L1_TCM (Level 1 total cache misses)";
        case PAPI_L2_TCM:
            return "PAPI_L2_TCM (Level 2 total cache misses)";
        case PAPI_L3_TCM:
            return "PAPI_L3_TCM (Level 3 total cache misses)";
        case PAPI_CA_SNP:
            return "PAPI_CA_SNP (Snoops)";
        case PAPI_CA_SHR:
            return "PAPI_CA_SHR (Request for shared cache line (SMP))";
        case PAPI_CA_CLN:
            return "PAPI_CA_CLN (Request for clean cache line (SMP))";
        case PAPI_CA_INV:
            return "PAPI_CA_INV (Request for cache line Invalidation (SMP))";
        case PAPI_CA_ITV:
            return "PAPI_CA_ITV (Request for cache line Intervention (SMP))";
        case PAPI_L3_LDM:
            return "PAPI_L3_LDM (Level 3 load misses)";
        case PAPI_L3_STM:
            return "PAPI_L3_STM (Level 3 store misses)";
        case PAPI_BRU_IDL:
            return "PAPI_BRU_IDL (Cycles branch units are idle)";
        case PAPI_FXU_IDL:
            return "PAPI_FXU_IDL (Cycles integer units are idle)";
        case PAPI_FPU_IDL:
            return "PAPI_FPU_IDL (Cycles floating point units are idle)";
        case PAPI_LSU_IDL:
            return "PAPI_LSU_IDL (Cycles load/store units are idle)";
        case PAPI_TLB_DM:
            return "PAPI_TLB_DM (Data translation lookaside buffer misses)";
        case PAPI_TLB_IM:
            return "PAPI_TLB_IM (Instr translation lookaside buffer misses)";
        case PAPI_TLB_TL:
            return "PAPI_TLB_TL (Total translation lookaside buffer misses)";
        case PAPI_L1_LDM:
            return "PAPI_L1_LDM (Level 1 load misses)";
        case PAPI_L1_STM:
            return "PAPI_L1_STM (Level 1 store misses)";
        case PAPI_L2_LDM:
            return "PAPI_L2_LDM (Level 2 load misses)";
        case PAPI_L2_STM:
            return "PAPI_L2_STM (Level 2 store misses)";
        case PAPI_BTAC_M:
            return "PAPI_BTAC_M (BTAC miss)";
        case PAPI_PRF_DM:
            return "PAPI_PRF_DM (Prefetch data instruction caused a miss)";
        case PAPI_L3_DCH:
            return "PAPI_L3_DCH (Level 3 Data Cache Hit)";
        case PAPI_TLB_SD:
            return "PAPI_TLB_SD (Xlation lookaside buffer shootdowns (SMP))";
        case PAPI_CSR_FAL:
            return "PAPI_CSR_FAL (Failed store conditional instructions)";
        case PAPI_CSR_SUC:
            return "PAPI_CSR_SUC (Successful store conditional instructions)";
        case PAPI_CSR_TOT:
            return "PAPI_CSR_TOT (Total store conditional instructions)";
        case PAPI_MEM_SCY:
            return "PAPI_MEM_SCY (Cycles Stalled Waiting for Memory Access)";
        case PAPI_MEM_RCY:
            return "PAPI_MEM_RCY (Cycles Stalled Waiting for Memory Read)";
        case PAPI_MEM_WCY:
            return "PAPI_MEM_WCY (Cycles Stalled Waiting for Memory Write)";
        case PAPI_STL_ICY:
            return "PAPI_STL_ICY (Cycles with No Instruction Issue)";
        case PAPI_FUL_ICY:
            return "PAPI_FUL_ICY (Cycles with Maximum Instruction Issue)";
        case PAPI_STL_CCY:
            return "PAPI_STL_CCY (Cycles with No Instruction Completion)";
        case PAPI_FUL_CCY:
            return "PAPI_FUL_CCY (Cycles with Maximum Instruction Completion)";
        case PAPI_HW_INT:
            return "PAPI_HW_INT (Hardware interrupts)";
        case PAPI_BR_UCN:
            return "PAPI_BR_UCN (Unconditional branch instructions executed)";
        case PAPI_BR_CN:
            return "PAPI_BR_CN (Conditional branch instructions executed)";
        case PAPI_BR_TKN:
            return "PAPI_BR_TKN (Conditional branch instructions taken)";
        case PAPI_BR_NTK:
            return "PAPI_BR_NTK (Conditional branch instructions not taken)";
        case PAPI_BR_MSP:
            return "PAPI_BR_MSP (Conditional branch instructions mispred)";
        case PAPI_BR_PRC:
            return "PAPI_BR_PRC (Conditional branch instructions corr. pred)";
        case PAPI_FMA_INS:
            return "PAPI_FMA_INS (FMA instructions completed)";
        case PAPI_TOT_IIS:
            return "PAPI_TOT_IIS (Total instructions issued)";
        case PAPI_TOT_INS:
            return "PAPI_TOT_INS (Total instructions executed)";
        case PAPI_INT_INS:
            return "PAPI_INT_INS (Integer instructions executed)";
        case PAPI_FP_INS:
            return "PAPI_FP_INS (Floating point instructions executed)";
        case PAPI_LD_INS:
            return "PAPI_LD_INS (Load instructions executed)";
        case PAPI_SR_INS:
            return "PAPI_SR_INS (Store instructions executed)";
        case PAPI_BR_INS:
            return "PAPI_BR_INS (Total branch instructions executed)";
        case PAPI_VEC_INS:
            return "PAPI_VEC_INS (Vector/SIMD instructions executed (could include integer))";
        case PAPI_RES_STL:
            return "PAPI_RES_STL (Cycles processor is stalled on resource)";
        case PAPI_FP_STAL:
            return "PAPI_FP_STAL (Cycles any FP units are stalled)";
        case PAPI_TOT_CYC:
            return "PAPI_TOT_CYC (Total cycles executed)";
        case PAPI_LST_INS:
            return "PAPI_LST_INS (Total load/store inst. executed)";
        case PAPI_SYC_INS:
            return "PAPI_SYC_INS (Sync. inst. executed)";
        case PAPI_L1_DCH:
            return "PAPI_L1_DCH (L1 D Cache Hit)";
        case PAPI_L2_DCH:
            return "PAPI_L2_DCH (L2 D Cache Hit)";
        case PAPI_L1_DCA:
            return "PAPI_L1_DCA (L1 D Cache Access)";
        case PAPI_L2_DCA:
            return "PAPI_L2_DCA (L2 D Cache Access)";
        case PAPI_L3_DCA:
            return "PAPI_L3_DCA (L3 D Cache Access)";
        case PAPI_L1_DCR:
            return "PAPI_L1_DCR (L1 D Cache Read)";
        case PAPI_L2_DCR:
            return "PAPI_L2_DCR (L2 D Cache Read)";
        case PAPI_L3_DCR:
            return "PAPI_L3_DCR (L3 D Cache Read)";
        case PAPI_L1_DCW:
            return "PAPI_L1_DCW (L1 D Cache Write)";
        case PAPI_L2_DCW:
            return "PAPI_L2_DCW (L2 D Cache Write)";
        case PAPI_L3_DCW:
            return "PAPI_L3_DCW (L3 D Cache Write)";
        case PAPI_L1_ICH:
            return "PAPI_L1_ICH (L1 instruction cache hits)";
        case PAPI_L2_ICH:
            return "PAPI_L2_ICH (L2 instruction cache hits)";
        case PAPI_L3_ICH:
            return "PAPI_L3_ICH (L3 instruction cache hits)";
        case PAPI_L1_ICA:
            return "PAPI_L1_ICA (L1 instruction cache accesses)";
        case PAPI_L2_ICA:
            return "PAPI_L2_ICA (L2 instruction cache accesses)";
        case PAPI_L3_ICA:
            return "PAPI_L3_ICA (L3 instruction cache accesses)";
        case PAPI_L1_ICR:
            return "PAPI_L1_ICR (L1 instruction cache reads)";
        case PAPI_L2_ICR:
            return "PAPI_L2_ICR (L2 instruction cache reads)";
        case PAPI_L3_ICR:
            return "PAPI_L3_ICR (L3 instruction cache reads)";
        case PAPI_L1_ICW:
            return "PAPI_L1_ICW (L1 instruction cache writes)";
        case PAPI_L2_ICW:
            return "PAPI_L2_ICW (L2 instruction cache writes)";
        case PAPI_L3_ICW:
            return "PAPI_L3_ICW (L3 instruction cache writes)";
        case PAPI_L1_TCH:
            return "PAPI_L1_TCH (L1 total cache hits)";
        case PAPI_L2_TCH:
            return "PAPI_L2_TCH (L2 total cache hits)";
        case PAPI_L3_TCH:
            return "PAPI_L3_TCH (L3 total cache hits)";
        case PAPI_L1_TCA:
            return "PAPI_L1_TCA (L1 total cache accesses)";
        case PAPI_L2_TCA:
            return "PAPI_L2_TCA (L2 total cache accesses)";
        case PAPI_L3_TCA:
            return "PAPI_L3_TCA (L3 total cache accesses)";
        case PAPI_L1_TCR:
            return "PAPI_L1_TCR (L1 total cache reads)";
        case PAPI_L2_TCR:
            return "PAPI_L2_TCR (L2 total cache reads)";
        case PAPI_L3_TCR:
            return "PAPI_L3_TCR (L3 total cache reads)";
        case PAPI_L1_TCW:
            return "PAPI_L1_TCW (L1 total cache writes)";
        case PAPI_L2_TCW:
            return "PAPI_L2_TCW (L2 total cache writes)";
        case PAPI_L3_TCW:
            return "PAPI_L3_TCW (L3 total cache writes)";
        case PAPI_FML_INS:
            return "PAPI_FML_INS (FM ins)";
        case PAPI_FAD_INS:
            return "PAPI_FAD_INS (FA ins)";
        case PAPI_FDV_INS:
            return "PAPI_FDV_INS (FD ins)";
        case PAPI_FSQ_INS:
            return "PAPI_FSQ_INS (FSq ins)";
        case PAPI_FNV_INS:
            return "PAPI_FNV_INS (Finv ins)";
        case PAPI_FP_OPS:
            return "PAPI_FP_OPS (Floating point operations executed)";
        case PAPI_SP_OPS:
            return "PAPI_SP_OPS (Floating point operations executed: optimized to count scaled single precision vector operations)";
        case PAPI_DP_OPS:
            return "PAPI_DP_OPS (Floating point operations executed: optimized to count scaled double precision vector operations)";
        case PAPI_VEC_SP:
            return "PAPI_VEC_SP (Single precision vector/SIMD instructions)";
        case PAPI_VEC_DP:
            return "PAPI_VEC_DP (Double precision vector/SIMD instructions)";
        case PAPI_REF_CYC:
            return "PAPI_REF_CYC (Reference clock cycles)";
        default:
            return "UNKNOWN CODE";
        }
    }
};

/**
 * PapiWrapper class for Sequential use
 * 
 * It is discoureaged to use this class directly but rather through the utility functions
 * inside the PAPIW namespace.
 */
class PapiWrapperSingle : public PapiWrapper
{
private:
    static int const papiMaxAllowedCounters = 20;
    int eventSet = PAPI_NULL;
    bool running = false;
    long long buffer[papiMaxAllowedCounters];
    long long values[papiMaxAllowedCounters];
    std::vector<int> events;

public:
    PapiWrapperSingle() : ThreadID(0) {}
    PapiWrapperSingle(const unsigned long threadID) : ThreadID(threadID) {}
    ~PapiWrapperSingle() {}

    const unsigned long ThreadID;

    /* Add an event to be counted */
    void AddEvent(const int eventCode) override
    {
        if (running)
            handle_error("AddEvent", "You can't add events while Papi is running\n");

        if (events.size() >= papiMaxAllowedCounters)
            handle_error("AddEvent", "Event count limit exceeded. Check papiMaxAllowedCounters\n");

        if (eventSet == PAPI_NULL)
        {
            retval = PAPI_create_eventset(&eventSet);
            if (retval != PAPI_OK)
                handle_error("AddEvent", "Could not create event set", retval);
        }

        retval = PAPI_add_event(eventSet, eventCode);
        if (retval != PAPI_OK)
            issue_waring("AddEvent. Could not add", getDescription(eventCode), retval);
        else
            events.push_back(eventCode);
    }

    /* Start the counter */
    void Start() override
    {
        if (running)
            handle_error("Start", "You can not start an already running PAPI instance");

        retval = PAPI_start(eventSet);
        if (retval != PAPI_OK)
            handle_error("Start", "Could not start PAPI counters", retval);

        running = true;
    }

    /* Stop the counter */
    void Stop() override
    {
        if (!running)
            handle_error("Stop", "You can not stop an already stopped Papi instance");

        retval = PAPI_stop(eventSet, buffer);
        if (retval != PAPI_OK)
            handle_error("Stop", "Could not stop PAPI counters", retval);

        int count = events.size();
        for (int i = 0; i < count; i++)
            values[i] += buffer[i];

        running = false;
    }

    /* Get the result of a specific event */
    long long GetResult(const int eventCode) override
    {
        if (running)
            handle_error("GetResult", "You can't get results while Papi is running\n");

        auto indexInResult = std::find(events.begin(), events.end(), eventCode);
        if (indexInResult == events.end())
            handle_error("GetResult", "The event is not supported or has not been added to the set");

        return values[indexInResult - events.begin()];
    }

    /* Reset the intermediate counter values */
    void Reset()
    {
        if (running)
            handle_error("Reset", "You can't reset while Papi is running\n");

        localInit();
    }

    /* Getter Method for running state */
    bool IsRunning()
    {
        return running;
    }

    /* Print the results */
    const long long *GetValues()
    {
        return values;
    }

    /* Print the results */
    void Print() override
    {
        if (running)
            handle_error("Print", "You can not print while Papi is running. Stop the counters first!");

        std::cout << "PAPIW Single PapiWrapper instance report:" << std::endl;
        print(events, values);
    }

protected:
    /* Initialize the values array */
    void localInit() override
    {
        for (int i = 0; i < papiMaxAllowedCounters; i++)
            values[i] = 0;
    }
};

#ifdef _OPENMP
/**
 * PapiWrapper class for Parallel use
 * 
 * It is discoureaged to use this class directly but rather through the utility functions
 * inside the PAPIW namespace.
 */
class PapiWrapperParallel : public PapiWrapper
{
private:
    inline static PapiWrapperSingle *localPapi;
#pragma omp threadprivate(localPapi)
    std::vector<int> events;
    std::vector<long long> values;
    int numRunningThreads = 0; //0 is none running
    bool startedFromParallelRegion = false;

public:
    PapiWrapperParallel() {}
    ~PapiWrapperParallel()
    {
        std::cout << "Destructing Local Papis" << std::endl;
        checkNotInParallelRegion("DESTRUCTOR");
#pragma omp parallel
        {
            delete localPapi;
            localPapi = nullptr;
        }
    }

    /* Register events to be counted */
    void AddEvent(const int eventCode) override
    {
        checkNotInParallelRegion("ADD_EVENT");
        checkNoneRunning("ADD_EVENT");
        events.push_back(eventCode);
        values.push_back(0);
    }

    /* Start the counters */
    void Start() override
    {
        if (isInParallelRegion())
        {

#pragma omp single
            checkNoneRunning("START");

            startedFromParallelRegion = true;
            start();
        }
        else
        {
            checkNoneRunning("START");
            startedFromParallelRegion = false;
#pragma omp parallel
            {
                start();
            }
        }
    }

    /* Stop the counters */
    void Stop() override
    {
        checkNumberOfThreads("STOP");

        if (isInParallelRegion())
        {
            if (!startedFromParallelRegion)
                issue_waring("Stop", "The Papi Counters have not been started in a parallel Region. You should not stop them in a parallel region, however, the results should be fine.");
            stop();

#pragma omp barrier

            numRunningThreads = 0;
        }
        else
        {
            if (startedFromParallelRegion)
                issue_waring("Stop", "The Papi Counters have been started in a parallel Region. You should stop them in the same parallel region or move Start/Stop completely out of the parallel region.");
#pragma omp parallel
            {
                stop();
            }
            numRunningThreads = 0;
        }
    }

    /* Get the result of a specific event */
    long long GetResult(const int eventCode) override
    {
        checkNoneRunning("GET_RESULT");

        auto indexInResult = std::find(events.begin(), events.end(), eventCode);
        if (indexInResult == events.end())
            handle_error("GetResult", "The event is not supported or has not been added to the set");

        return values[indexInResult - events.begin()];
    }

    /* Print the values */
    void Print() override
    {
        checkNoneRunning("PRINT");
#pragma omp single
        {
            std::cout << "PAPIW Parallel PapiWrapper instance report:" << std::endl;
            print(events, values.data());
        }
    }

    /* Reset the values */
    void Reset()
    {
        checkNoneRunning("RESET");
#pragma omp single
        std::fill(values.begin(), values.end(), 0);
    }

protected:
    /* Initialize the instance */
    void localInit() override
    {
        checkNotInParallelRegion("INIT");

        retval = PAPI_thread_init(pthread_self);
        if (retval != PAPI_OK)
            handle_error("localInit in PapiWrapperParallel", "Could not initialize OMP Support", retval);
        else
            std::cout << "Papi Parallel support enabled" << std::endl;
    }

    /* Helper function to start the counters */
    void start()
    {
#pragma omp single
        numRunningThreads = omp_get_num_threads();

        retval = PAPI_register_thread();
        if (retval != PAPI_OK)
            handle_error("Start", "Couldn't register thread", retval);

        localPapi = new PapiWrapperSingle(pthread_self());
        for (auto eventCode : events)
            localPapi->AddEvent(eventCode);

        localPapi->Start();
    }

    /* Helper function to stop the counters and accumulate the values to total */
    void stop()
    {
        localPapi->Stop();

        /*Check that same thread is used since starting the counters*/
        if (PAPI_thread_id() != localPapi->ThreadID)
            handle_error("Stop", "Invalid State: The Thread Ids differs from initialization!\nApparently, new threads were use without reassigning the Papi counters. Please Start and Stop more often to avoid this error.");

        int eventCount = events.size();
        for (int i = 0; i < eventCount; i++)
        {
            auto localVal = localPapi->GetResult(events[i]);
#pragma omp atomic
            values[i] += localVal;
        }

        delete localPapi;
        localPapi = nullptr;

        retval = PAPI_unregister_thread();
        if (retval != PAPI_OK)
            handle_error("Stop", "Couldn't unregister thread", retval);
    }

    /* Returns the current OMP team size */
    int GetNumThreads()
    {
        if (isInParallelRegion())
            return omp_get_num_threads();

        int count;
#pragma omp parallel
        {
#pragma omp master
            count = omp_get_num_threads();
        }
        return count;
    }

    /* Returns true if it is called in a parallel region or false otherwise */
    bool isInParallelRegion()
    {
        return omp_get_level() != 0;
    }

    /* Check that the current thread team size is not larger then it was last registered or exit with an error otherwise */
    void checkNumberOfThreads(const char *actionMsg)
    {
        /* State check */
        if (GetNumThreads() != numRunningThreads)
            handle_error(actionMsg, "The OMP teamsize is different than indicated in Start!");
    }

    /* Check that this method is not called from a parallel context or exit with an error otherwise */
    void checkNotInParallelRegion(const char *actionMsg)
    {
        if (isInParallelRegion())
            handle_error(actionMsg, "You may not perform this operation from a parallel region");
    }

    /* Check that no Papi Counter is running or exit with an error otherwise */
    void checkNoneRunning(const char *actionMsg)
    {
        if (numRunningThreads)
            handle_error(actionMsg, "You can not perform this action while Papi is running. Stop the counters first!");
    }
};
#endif

#endif
#endif