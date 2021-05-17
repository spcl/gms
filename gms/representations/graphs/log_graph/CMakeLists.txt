set(KERNELS kbit_bfs kbit_bc kbit_cc kbit_pr kbit_tc kbit_sssp)
set(VARIANTS LG LG_gap LG_local LG_local_gap LG_bittree LG_varint_byte_based LG_varint_word_based)

set(COMPRESSED_VARIANTS LG_varint_byte_based LG_varint_word_based)
set(PERMUTERS in_degree_ascending in_degree_descending out_degree_ascending out_degree_descending)

set(CPLEX_PERMUTERS
        optimal_diff_nn_ilp_unconstr_permuter
        optimal_diff_nn_lp_unconstr_permuter
        optimal_diff_nn_ilp_constr_permuter
        optimal_diff_nn_lp_constr_permuter
        optimal_diff_vn_ilp_unconstr_permuter
        optimal_diff_vn_lp_unconstr_permuter
        optimal_diff_vn_ilp_constr_permuter
        optimal_diff_vn_lp_constr_permuter
        o_ilp_nn_un_n_permuter
        o_ilp_nn_con_n_permuter
        o_ilp_vn_un_n_permuter
        o_ilp_vn_con_n_permuter)

# TODO limit what we want to keep

set(LG           -DLOCAL_APPROACH=0     -DSIMPLE_GAP_ENCODING=0)
set(LG_gap       -DLOCAL_APPROACH=0     -DSIMPLE_GAP_ENCODING=1)
set(LG_local     -DLOCAL_APPROACH=1     -DSIMPLE_GAP_ENCODING=0)
set(LG_local_gap -DLOCAL_APPROACH=1     -DSIMPLE_GAP_ENCODING=1)
set(LG_bittree   -DLOCAL_APPROACH=1     -DSIMPLE_GAP_ENCODING=0    -DBIT_TREE=1)

set(LG_varint_byte_based   -DLOCAL_APPROACH=0     -DSIMPLE_GAP_ENCODING=0    -DCOMPRESSED=1     -DVARINT_BYTE_BASED=1)
set(LG_varint_word_based   -DLOCAL_APPROACH=0     -DSIMPLE_GAP_ENCODING=0    -DCOMPRESSED=1     -DVARINT_WORD_BASED=1)

set(in_degree_ascending   -DPERMUTED=1 -DIN_ASCENDING=1)
set(in_degree_descending  -DPERMUTED=1 -DIN_DESCENDING=1)
set(out_degree_ascending  -DPERMUTED=1 -DOUT_ASCENDING=1)
set(out_degree_descending -DPERMUTED=1 -DOUT_DESCENDING=1)
set(optimal_diff_nn_ilp_unconstr_permuter -DPERMUTED=1 -DCPLEX_ENABLED=1 -DOPTIMAL_DIFF_NN_ILP_UNCONSTR=1)
set(optimal_diff_nn_lp_unconstr_permuter  -DPERMUTED=1 -DCPLEX_ENABLED=1 -DOPTIMAL_DIFF_NN_LP_UNCONSTR=1)
set(optimal_diff_nn_ilp_constr_permuter   -DPERMUTED=1 -DCPLEX_ENABLED=1 -DOPTIMAL_DIFF_NN_ILP_CONSTR=1)
set(optimal_diff_nn_lp_constr_permuter    -DPERMUTED=1 -DCPLEX_ENABLED=1 -DOPTIMAL_DIFF_NN_LP_CONSTR=1)
set(optimal_diff_vn_ilp_unconstr_permuter -DPERMUTED=1 -DCPLEX_ENABLED=1 -DOPTIMAL_DIFF_VN_ILP_UNCONSTR=1)
set(optimal_diff_vn_lp_unconstr_permuter  -DPERMUTED=1 -DCPLEX_ENABLED=1 -DOPTIMAL_DIFF_VN_LP_UNCONSTR=1)
set(optimal_diff_vn_ilp_constr_permuter   -DPERMUTED=1 -DCPLEX_ENABLED=1 -DOPTIMAL_DIFF_VN_ILP_CONSTR=1)
set(optimal_diff_vn_lp_constr_permuter    -DPERMUTED=1 -DCPLEX_ENABLED=1 -DOPTIMAL_DIFF_VN_LP_CONSTR=1)
set(o_ilp_nn_un_n_permuter                -DPERMUTED=1 -DCPLEX_ENABLED=1 -DO_ILP_NN_UN_N=1)
set(o_ilp_nn_con_n_permuter               -DPERMUTED=1 -DCPLEX_ENABLED=1 -DO_ILP_NN_CON_N=1)
set(o_ilp_vn_un_n_permuter                -DPERMUTED=1 -DCPLEX_ENABLED=1 -DO_ILP_VN_UN_N=1)
set(o_ilp_vn_con_n_permuter               -DPERMUTED=1 -DCPLEX_ENABLED=1 -DO_ILP_VN_CON_N=1)

function(gapbs_benchmark target source)
    add_executable(${target} ${source})
    set_target_properties(${target} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/gapbs")
endfunction()

foreach(KERNEL ${KERNELS})
    foreach(VARIANT ${VARIANTS})
        gapbs_benchmark(${KERNEL}_${VARIANT} ${KERNEL}.cc)
        target_compile_options(${KERNEL}_${VARIANT} PUBLIC -DIL_STD)
        target_compile_definitions(${KERNEL}_${VARIANT} PUBLIC ${${VARIANT}})
    endforeach()

    foreach(VARIANT ${COMPRESSED_VARIANTS})
        foreach(PERM ${PERMUTERS})
            gapbs_benchmark(${KERNEL}_${VARIANT}_${PERM} ${KERNEL}.cc)
            target_compile_options(${KERNEL}_${VARIANT}_${PERM} PUBLIC)
            target_compile_definitions(${KERNEL}_${VARIANT}_${PERM} PUBLIC ${${VARIANT}} ${${PERM}})
        endforeach()

        if (CPLEX_FOUND)
            foreach(PERM ${CPLEX_PERMUTERS})
                gapbs_benchmark(${KERNEL}_${VARIANT}_${PERM} ${KERNEL}.cc)
                include_directories(SYSTEM ${CPLEX_INCLUDE_DIRS})
                target_link_libraries(${KERNEL}_${VARIANT}_${PERM} ${CPLEX_LIBRARIES} dl)
                target_compile_options(${KERNEL}_${VARIANT}_${PERM} PUBLIC -DIL_STD)
                target_compile_definitions(${KERNEL}_${VARIANT}_${PERM} PUBLIC ${${VARIANT}} ${${PERM}})
            endforeach()
        endif ()
    endforeach()
endforeach()