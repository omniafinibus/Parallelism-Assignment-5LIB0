Libimage is an example of how you could make a library our of the image
processing functions.

The downside of making a library that is used by multiple partitions is that
the code for the library is compiled independently of the partitions (e.g.
TILE_ID and PARTITION_ID are not defined), and thus the same for all
partitions.  If you do want to differentiate between the partitions then you
have to do this through function parameters.

The downside of not using a library is that any change you make in inmage
processing functions has to be made for each partition separately.  It's up to
you to decide what works best for you: a library or just copying the image
functions to each partition.

An intermediate solution is to not use a library but to have one source in e.g.
partition_0_1 and then use a symbolic link from the other partitions to the
original.  You can then use TILE_ID & PARTITION_ID to differentiate between the
different partitions.
