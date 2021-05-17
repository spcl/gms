# PAPIW (A Papi wrapper)

This subdriectory contains the code for a Papi wrapper, a Header-only library that simplifies the use of Papi, especially when using Openmp.

### Include

include `<gms/common/papi/papiw.h>` in your `GMS` subproject file to inject the `PAPIW` namespace.  
Then activate papi measurement by providing `PAPIW` to the `gms_benchmark` cmake function, something like: `gms_benchmark(maximal_clique_enum_bron_kerbosch.cc PAPIW)`. If the flag is not provided for compilation, than any call to `PAPIW` in code is turned into a No-op.

### Usage

Initialization (Supports variadic Papi eventcode arguments):

```c++
    // Use either
    PAPIW::INIT_SINGLE(PAPI_L2_TCA, PAPI_L3_TCA); // Init PAPIW for sequential use only
    // Or
    PAPIW::INIT_PARALLEL(PAPI_L2_TCA, PAPI_L3_TCA); // Init PAPIW for parallel use
```

Benchmarking:

```c++
    PAPIW::START();
    doSomethingInteressting();
    PAPIW::STOP();

    doSomethingUnimportant(); // Do not measure

    PAPIW::START();
    doSomethingInteresstingAgain();
    PAPIW::STOP();
```

Benchmarking parallel regions:

```c++
// This setting should be preferred
#pragma omp parallel
{
    PAPIW::START();
    doSomethingInteressting();
    PAPIW::STOP();
}

// But this also works
PAPIW::START();
#pragma omp parallel
{
    doSomethingInteressting();
}
PAPIW::STOP();
```

Resetting:

```c++
    PAPIW::RESET(); // Set the intermediate counter values to zero
```

Printing:

```c++
    PAPIW::PRINT();
```

The output could look something like:

```
PAPIW Parallel PapiWrapper instance report:
PAPI_L2_TCA (L2 total cache accesses): 68743998
PAPI_TOT_CYC (Total cycles executed): 9800773029
PAPI_L3_TCA (L3 total cache accesses): 32864360
PAPI_L3_TCM (Level 3 total cache misses): 17234237
@%% PAPI_L2_TCA PAPI_TOT_CYC PAPI_L3_TCA PAPI_L3_TCM
@%@ 68743998 9800773029 32864360 17234237
```

### Info

- The recommended cmake setup aims for a soft dependency: If Papi is not available on the system, most code will not get compiled and any call to `PAPIW` is turned into a No-op. The same effect can be achieved by setting `NOPAPIW` for building
- Since `PAPIW` needs threadprivate states and Papi itself needs to be refreshed whenever an underlying kernel LWP was killed, one should stop and start between different parallel regions, whenever possible.
- `PAPIW::START()` assigns and starts the counter to the threads. The number of threads is the current opm team size
- If `omp_set_num_threads` is used, `PAPIW::STOP()` has to be called right before. Certainly, `PAPIW::START()` may be called immediately afterwards.
- Assuming `PAPIW` was initialized using `INIT_PARALLEL`, it can be started and stopped inside a parallel region or outside. It will always use the omp team size based on a call to `omp_get_num_threads` in a parallel region.
- Whenever possible, `PAPIW:START()` and `PAPIW::STOP()` should be called directly inside one parallel region
- `PAPIW::INIT_SINGLE` and `PAPIW::INIT_PARALLEL` may not be called inside a parallel region
- `PAPIW::RESET` and `PAPIW::PRINT` may not be called while the counters are still running
- If an event, which is not available on the system, is added in `PAPIW::INIT`, then only a warning is displayed and the program continues. Of course no data can be gathered and hence, no output for that specific event is printed out
- A lot of state checks are used for `PAPIW`. In the event of an invalid state, the program aborts and a human-readable error message is printed out
- The output is optimized for easy extraction, e.g. for some plotting programs:
  - First all observed counters are displayed with a short description and their measured values
  - Then `@%%` indicates the header (Papi Counter name)
  - And `@%@` indicates the values

### Dos and Don'ts

Stop before changing the current teamsize

```c++
PAPIW::STOP();
omp_set_num_threads(4);
PAPIW::START();
```

Although it should work, don't use the following

```c++
#pragma omp parallel
{
    PAPIW::START();
    doSomethingInteressting();
}
PAPIW::STOP();
```

Following is nonsense:

```c++
#pragma omp parallel for
for(int i = 0; i<n; i++)
{
    PAPIW::START();
    doSomethingInteressting();
    PAPIW::STOP();
}
```

### Limitations

- Was created with OpenMp in mind
- `omp_set_dynamic` should be false. Otherwise make sure that `PAPIW:START()` and `PAPIW:STOP()` are only used inside Parallel regions
