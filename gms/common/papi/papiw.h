#ifndef PAPIW
#define PAPIW

#include "./papiw_util.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
/**
 * Papi Wrapper Highlevel Module
 * 
 * This namespace groups all relevant control functions at one place.
 * It is encouraged to use this utility functions, rather than operating
 * directly on the PapiWrapper instances.
 * 
 * @note If NOPAPIW is defined, all calls to PAPIW become No-Ops
 * @note If Openmp is missing, then all parallel counters are turned into sequential ones
 *
 * Example of use:
 *     PAPIW::INIT_SINGLE(PAPI_L2_TCA, PAPI_L2_TCM, PAPI_L3_TCA, PAPI_L3_TCM);
 *     PAPIW::Start();
 *     doWork();
 *     PAPIW::Stop();
 *     PAPIW::Print();
 */
namespace PAPIW
{
        /* Anonymous Namespace to hide PapiWrapper Object */
        namespace
        {
#if !defined(NOPAPIW)
                PapiWrapper *papiwrapper = nullptr;
#else
                /* Helper Function to ignore unused warning parameter warning if PAPIW is not used */
                struct sink
                {
                        template <typename... Args>
                        sink(Args const &...) {}
                };
#endif
        } // namespace

        /**
     * Initialize Papi wrapper module for sequential use only
     *
     * @tparam PapiCodes a variadic list of PAPI eventcodes
     * @warning Exits with an error if called in a parallel region
     */
        template <typename... PapiCodes>
        void INIT_SINGLE(PapiCodes const... eventcodes)
        {
#if !defined(NOPAPIW)
                delete papiwrapper;
                papiwrapper = static_cast<PapiWrapper *>(new PapiWrapperSingle());
                papiwrapper->Init(eventcodes...);
#else
                sink{eventcodes...};
#endif
        }

        /**
     * Initialize Papi wrapper module for parallel use
     *
     * @tparam PapiCodes a variadic list of PAPI eventcodes
     * @warning Exits with an error if called in a parallel region
     */
        template <typename... PapiCodes>
        void INIT_PARALLEL(PapiCodes const... eventcodes)
        {
#if !defined(_OPENMP)
                INIT_SINGLE(eventcodes...);
#elif !defined(NOPAPIW)
                delete papiwrapper;
                papiwrapper = static_cast<PapiWrapper *>(new PapiWrapperParallel());
                papiwrapper->Init(eventcodes...);
#else
                sink{eventcodes...};
#endif
        }

        /* Start the counters */
        void START()
        {
#if !defined(NOPAPIW)
                papiwrapper->Start();
#endif
        }

        /**
     * Stop the Papi Counters. The current state persists and you may start again to continue counting
     *
     * Example of use:
     *     PAPIW::Start();
     *     doWork();
     *     PAPIW::Stop();
     *     doWorkWhichIsNotMeasured();
     *     PAPIW::Start();
     *     doWork();
     *     PAPIW::Stop();
     */
        void STOP()
        {
#if !defined(NOPAPIW)
                papiwrapper->Stop();
#endif
        }

        /**
     * Reset the Counters. Use this if you want to start fresh counters after a print
     *
     * @warning Exits with an error if the counters are running while calling RESET
     */
        void RESET()
        {
#if !defined(NOPAPIW)
                papiwrapper->Reset();
#endif
        }

        /**
     * Print the values for all initialized PAPI Events
     *
     * @warning Exits with an error if the counters are running while calling PRINT
     */
        void PRINT()
        {
#if !defined(NOPAPIW)
                papiwrapper->Print();
#endif
        }
} // namespace PAPIW
#pragma GCC diagnostic pop

#ifdef NOPAPIW
/* Provide PAPI Counter Macros, s.t. a program with deactivated PAPIW compiles nevertheless */
#define PAPI_L1_DCM 0
#define PAPI_L1_ICM 0
#define PAPI_L2_DCM 0
#define PAPI_L2_ICM 0
#define PAPI_L3_DCM 0
#define PAPI_L3_ICM 0
#define PAPI_L1_TCM 0
#define PAPI_L2_TCM 0
#define PAPI_L3_TCM 0
#define PAPI_CA_SNP 0
#define PAPI_CA_SHR 0
#define PAPI_CA_CLN 0
#define PAPI_CA_INV 0
#define PAPI_CA_ITV 0
#define PAPI_L3_LDM 0
#define PAPI_L3_STM 0
#define PAPI_BRU_IDL 0
#define PAPI_FXU_IDL 0
#define PAPI_FPU_IDL 0
#define PAPI_LSU_IDL 0
#define PAPI_TLB_DM 0
#define PAPI_TLB_IM 0
#define PAPI_TLB_TL 0
#define PAPI_L1_LDM 0
#define PAPI_L1_STM 0
#define PAPI_L2_LDM 0
#define PAPI_L2_STM 0
#define PAPI_BTAC_M 0
#define PAPI_PRF_DM 0
#define PAPI_L3_DCH 0
#define PAPI_TLB_SD 0
#define PAPI_CSR_FAL 0
#define PAPI_CSR_SUC 0
#define PAPI_CSR_TOT 0
#define PAPI_MEM_SCY 0
#define PAPI_MEM_RCY 0
#define PAPI_MEM_WCY 0
#define PAPI_STL_ICY 0
#define PAPI_FUL_ICY 0
#define PAPI_STL_CCY 0
#define PAPI_FUL_CCY 0
#define PAPI_HW_INT 0
#define PAPI_BR_UCN 0
#define PAPI_BR_CN 0
#define PAPI_BR_TKN 0
#define PAPI_BR_NTK 0
#define PAPI_BR_MSP 0
#define PAPI_BR_PRC 0
#define PAPI_FMA_INS 0
#define PAPI_TOT_IIS 0
#define PAPI_TOT_INS 0
#define PAPI_INT_INS 0
#define PAPI_FP_INS 0
#define PAPI_LD_INS 0
#define PAPI_SR_INS 0
#define PAPI_BR_INS 0
#define PAPI_VEC_INS 0
#define PAPI_RES_STL 0
#define PAPI_FP_STAL 0
#define PAPI_TOT_CYC 0
#define PAPI_LST_INS 0
#define PAPI_SYC_INS 0
#define PAPI_L1_DCH 0
#define PAPI_L2_DCH 0
#define PAPI_L1_DCA 0
#define PAPI_L2_DCA 0
#define PAPI_L3_DCA 0
#define PAPI_L1_DCR 0
#define PAPI_L2_DCR 0
#define PAPI_L3_DCR 0
#define PAPI_L1_DCW 0
#define PAPI_L2_DCW 0
#define PAPI_L3_DCW 0
#define PAPI_L1_ICH 0
#define PAPI_L2_ICH 0
#define PAPI_L3_ICH 0
#define PAPI_L1_ICA 0
#define PAPI_L2_ICA 0
#define PAPI_L3_ICA 0
#define PAPI_L1_ICR 0
#define PAPI_L2_ICR 0
#define PAPI_L3_ICR 0
#define PAPI_L1_ICW 0
#define PAPI_L2_ICW 0
#define PAPI_L3_ICW 0
#define PAPI_L1_TCH 0
#define PAPI_L2_TCH 0
#define PAPI_L3_TCH 0
#define PAPI_L1_TCA 0
#define PAPI_L2_TCA 0
#define PAPI_L3_TCA 0
#define PAPI_L1_TCR 0
#define PAPI_L2_TCR 0
#define PAPI_L3_TCR 0
#define PAPI_L1_TCW 0
#define PAPI_L2_TCW 0
#define PAPI_L3_TCW 0
#define PAPI_FML_INS 0
#define PAPI_FAD_INS 0
#define PAPI_FDV_INS 0
#define PAPI_FSQ_INS 0
#define PAPI_FNV_INS 0
#define PAPI_FP_OPS 0
#define PAPI_SP_OPS 0
#define PAPI_DP_OPS 0
#define PAPI_VEC_SP 0
#define PAPI_VEC_DP 0
#define PAPI_REF_CYC 0
#endif
#endif