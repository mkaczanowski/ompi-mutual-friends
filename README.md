# Setup
This experiment aims to boost up data processing speed by spreading the workload on a few low-end ARM boards. So, I'd recommend setting up a few Raspberry PIs within the same network pod, but if you can't do that, the OpenMPI framework also allows local execution (handy for testing).

My setup consists of 4x [Odroid U3](https://wiki.odroid.com/old_product/odroid-x_u_q/odroid_u3/odroid-u3) boards connected by a cheap 1 GBps network switch. The eMMC speed is a clear advantage, and since we heavily depend on data access speed, the odroid board sounds like a good fit.

## Accessing Twitter's data
In the real-world big data cluster, the data likely is stored on some distributed filesystem to provide faster access and data redundancy. As an example, the figure below explains how HDFS stores files.

![HDFS data distribution](http://mkaczanowski.com/wp-content/uploads/2015/05/hdfs-data-distribution.png "HDFS data distribution")

Because we operate on low-end ARM devices, we won't use HDFS, but instead, we split the file into N parts and copy them across the nodes.
```
$ wget https://snap.stanford.edu/data/twitter_combined.txt.gz
$ gunzip -d twitter_combined.txt.gz

$ split -l 10000 twitter_combined.txt tsplit
$ mv tsplit* data/
```

## Intermediate storage
The OpenMPI library takes care of message passing but doesn't store the intermediate results of the map-reduce steps; therefore, we need to spawn up some remote database, i.e., Memcache:
```
$ docker run --network host --name my-memcache -d memcached
```

## Building project
Running on multiple machines (via ssh):
```
mpirun --allow-run-as-root -np 16 --host odroid0,odroid1,odroid2,odroid3 MEMCACHED_SERVER=<host> ~/friends ~/data/
```

Testing on a single node:
```
$ mkdir build && cd build
$ cmake ../
$ make

$ MEMCACHED_SERVER=localhost ./friends ../data/twitter_combined.txt
```

The results can be optionally dumped to a file, for example:
```
$ cat output.txt | grep "[17711130, 99712896]"
[17711130, 99712896]: 21447363 22462180 34428380 36475508 43003845 107512718 126391227 222851879 223990701
```

# How does it work?
Map-reduce tutorials are abundant on the internet, so I'll skip the deep-dive and point you to the figure below that briefly explains the overall flow.

![Map Reduce structure](http://mkaczanowski.com/wp-content/uploads/2015/05/MapReduce_Work_Structure.png "Map Reduce structure")

The example run for A & B friends who have D, Z, F, G friends in common:
1. Transform data to form A -> D,Z,E (friend list)
2. Perform a Cartesian product:
```
(A, D) -> D, Z, E
(A, Z) -> D, Z, E
(A, E) -> D, Z, E
```

3. Reduce all tuples to a form (A, D) -> (D,Z,E)(Z,E,F,G)
4. Intersect friends lists (A, D) -> (Z,E)
5. Save output

In comparison to far simpler (IMHO) Hadoop API the OpenMPI adds a few new operations:
* collate - method of a MapReduce object, that aggregates a KeyValue object across processors and converts it into a KeyMultiValue object
* gather - method of a MapReduce object, that collects the key/value pairs of a KeyValue object spread across all processors to form a new KeyValue object on a subset (nprocs) of processors.

# Summary
The OpenMPI version of map-reduce is not as straightforward as using Hadoop, as it requires a few extra steps and a bit of coding work. However, resource-wise, it's a better fit for the cluster of ARM prototype boards.

Here's the results of the test I conducted on 4x odroids U3 (16 nprocs):

| procs  | time  |
| :------------ | :------------ |
|  1 |  623.784  |
|  2 |  562.679 |
|  3 |  202.538 |
|  4 |  179.679 |
|  5 |  197.639 |
|  6 |  262.464 |
|  7 |  56.529 |
|  8 |  34.286 |
|  9 |  31.052 |
|  10 |  31.041 |
|  11 |  28.649 |
|  12 |  26.256 |
|  13 |  23.924 |
|  14 | 23.61  |
|  15 |  22.775 |
|  16 |  21.538 |

![OpenMPI map reduce results](http://mkaczanowski.com/wp-content/uploads/2015/05/openmpi-map-reduce-results1.png "OpenMPI map reduce results")

It took 21.538s to process 421000 tuples on 16 cores (4 machines), where on 1 core it takes 623.784s. Is that satisfactory? To me, yes... somewhat, I expected worse performance.

# Demo
[![](http://mkaczanowski.com/wp-content/uploads/2015/05/5f2c954e910fddownload-1.png)](https://www.youtube.com/watch?v=_u7X21Gj6XE)
