#include <iostream>

#include "../../util/command_line.hpp"
#include "../../util/subgraphiso_verification.hpp"

#include "vf2.hpp"
#include <gms/common/types.h>
#include <gms/common/pipeline.h>

using namespace GMS;
using namespace GMS::SubGraphIso;

class SubGraphIsoPipeline : public Pipeline
{
public:
	const SubGraphIsoCLApp &ClApp;
	CSRGraph Pattern;
	CSRGraph Target;
	Result Results;

	SubGraphIsoPipeline(const SubGraphIsoCLApp &clapp)
		: ClApp(clapp)
	{}

	void ReadAndConvertGraphs()
	{
		Builder b(ClApp);
		Target = b.MakeGraph();
		Builder b2(ClApp);
		Pattern = b2.MakeGraph(ClApp.pattern_filename());
		Results = Result();
	}

	void Solve()
	{
		VF2 solver(Target, Pattern);
		Results = solver.run();
	}


	void Verify()
	{
		if (Results.aborted)
		{
			std::cout << "Aborted" << std::endl;
		} else if (Results.isomorphism.size() == 0)
		{
			std::cout << "No isomorphisms were found" << std::endl;
		}
		bool pass = subgraphisomorphismVerification(Results, Target, Pattern);
		LocalPrinter << (pass ? "pass" : "failed");
	}
};

int main(int argc, char *argv[])
{
    SubGraphIsoCLApp cli = subgraph_iso_parse_args(argc, argv);
	SubGraphIsoPipeline pipeline(cli);

	pipeline.SetPrintInfo("vf2", "parallel");
	pipeline.Run<SubGraphIsoPipeline>(cli, &SubGraphIsoPipeline::ReadAndConvertGraphs,
									  &SubGraphIsoPipeline::Solve, &SubGraphIsoPipeline::Verify);

	return 0;
}