#gms_benchmark(maximal_clique_enum_bron_kerbosch.cc PAPIW) # With PAPIW support
gms_benchmark(maximal_clique_enum_bron_kerbosch.cc) # Without PAPIW support
gms_benchmark(maximal_clique_enum_bron_kerbosch_papiw.cc PAPIW)
gms_benchmark(maximal_clique_enum_bron_kerbosch_sequential.cc)

# TODO convert into CMake option
#Compile with this Definition for counting the maximum cliques
target_compile_definitions(maximal_clique_enum_bron_kerbosch PUBLIC -DBK_COUNT)
target_compile_definitions(maximal_clique_enum_bron_kerbosch_papiw PUBLIC -DBK_COUNT)
target_compile_definitions(maximal_clique_enum_bron_kerbosch_sequential PUBLIC -DBK_COUNT)