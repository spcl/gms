#ifndef OPTIMAL_DIFF_VN_LP_CONSTR_PERMUTER_H
#define OPTIMAL_DIFF_VN_LP_CONSTR_PERMUTER_H


#include <ilcplex/ilocplex.h>
ILOSTLBEGIN



template <class NodeID_, class DestID_, bool invert>
class OptimalDiffVNLpConstrPermuter {

public:

    static std::map<NodeID_, NodeID_> permutation_map(CSRGraphBase<NodeID_, DestID_, invert>& graph) {

        std::map<NodeID_, NodeID_> trans;
        IloEnv env;

        try {
            IloModel model(env);
            IloNumVarArray vars(env);
            IloExpr obj_fun(env);

            int64_t n = graph.num_nodes();

            for(int64_t i = 0; i < n; ++i) {
                vars.add(IloNumVar(env, 0, n-1, ILOFLOAT));
            }

            for(auto v1 : graph.vertices()) {

                bool first = true;
                NodeID_ prev_v2;
                for (auto v2: graph.out_neigh(v1)) {

                    if (first) {
                        obj_fun += vars[v2] - vars[v1];
                        model.add(vars[v1] - vars[v2] >= 1 || vars[v1] - vars[v2] <= 1);

                        first = false;
                        prev_v2 = v2;
                        continue;
                    }

                    obj_fun += vars[v2] - vars[v1];
                    model.add(vars[v2] - vars[v1] >= 1);
                    prev_v2 = v2;
                }
            }

            for(auto v1 : graph.vertices()) {
                for(auto v2 : graph.vertices()) {

                    if(v1 == v2) {
                        continue;
                    }
                    model.add(vars[v1] - vars[v2] >= 1 || vars[v1] - vars[v2] <= 1);
                }
            }

            model.add(IloMinimize(env, obj_fun));
            IloCplex cplex(model);
            int ret_val = cplex.solve();

            if ( !ret_val ) {
                env.error() << "Failed to optimize LP." << endl;
                throw(-1);
            }

            IloNumArray vals(env);
            cplex.getValues(vals, vars);

            std::vector<bool> occupied(n, false);

            for(auto old_v : graph.vertices()) {
                assert(old_v < graph.num_nodes() && old_v >= 0);
                NodeID_ val = round(vals[old_v]);

                if(val < 0) {
                    val = 0;
                }
                if(val >= n) {
                    val = n-1;
                }

                NodeID_ spot = val;
                while(occupied[spot]) {
                    ++spot;
                    if(spot >= n) {
                        spot = 0;
                    }
                }
                occupied[spot] = true;
                trans[old_v] = spot;
            }



        }
        catch (...) {
            cerr << "Exception caught" << endl;
            for(auto v1 : graph.vertices()) {
                (trans)[v1] = v1;
            }

            return trans;
        }

        env.end();

        return trans;

    }
};


#endif
