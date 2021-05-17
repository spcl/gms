# GMS: Third-party Libraries
This directory contains the following third-party libraries:

- `gapbs/`: Infrastructure code of the [GAP Benchmark Suite](https://github.com/sbeamer/gapbs) with several modifications by GMS, in particular `gapbs/builder.h` and `gapbs/graph.h` have been extended by custom functionality.
  This functionality is used to read input files.
- `roaring/`: [Roaring Bitmap](https://github.com/RoaringBitmap/CRoaring) which is wrapped by the class `RoaringSet`. 
- `clipp.h`: [Command line parser library](https://github.com/muellan/clipp).
- `fast_range.h`: [Fair maps to intervals without division](http://lemire.me/blog/2016/06/27/a-fast-alternative-to-the-modulo-reduction/).
- `fast_statistics.h`: Several random number generators from different sources.
- `robin_hood.h`: [Robin hood hashing implementation](https://github.com/martinus/robin-hood-hashing), used for GMS class `RobinHoodSet`. 