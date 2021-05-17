// Copyright (c) 2015, The Regents of the University of California (Regents)
// See LICENSE.txt for license details

#include <iostream>

#include <gms/third_party/gapbs/benchmark.h>
#include <gms/third_party/gapbs/builder.h>
#include <gms/third_party/gapbs/command_line.h>
#include <gms/third_party/gapbs/graph.h>
#include <gms/third_party/gapbs/reader.h>
#include <gms/third_party/gapbs/writer.h>

using namespace std;

int main(int argc, char* argv[]) {
  CLConvert cli(argc, argv, "converter");
  cli.ParseArgs();
  if (cli.out_weighted()) {
    WeightedBuilder bw(cli);
    WGraph wg = bw.MakeGraph();
    wg.PrintStats();
    WeightedWriter ww(wg);
    ww.WriteGraph(cli.out_filename(), cli.out_sg());
  } else {
    Builder b(cli);
    CSRGraph g = b.MakeGraph();
    g.PrintStats();
    Writer w(g);
    w.WriteGraph(cli.out_filename(), cli.out_sg());
  }
  return 0;
}
