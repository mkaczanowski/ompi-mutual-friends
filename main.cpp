#include <keyvalue.h>
#include <mapreduce.h>
#include <mpi.h>
#include <iostream>
#include "common.h"
#include "memcached.h"
#include "mfriends.h"

using namespace MAPREDUCE_NS;

int main(int narg, char** args) {
    MPI_Init(&narg, &args);

    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    if (narg <= 1) {
        if (rank == 0)
            std::cerr << "Syntax: mfriends file1 file2 ..." << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    auto rc = init_memcached("localhost");
    if (rc != 0) {
        std::cerr << "Failed to initialize memcached" << std::endl;
        return 255;
    }

    MapReduce* mr = new MapReduce(MPI_COMM_WORLD);
    mr->verbosity = 1;
    mr->timer = 1;

    /** STEP 1 **/
    MPI_Barrier(MPI_COMM_WORLD);
    double tstart = MPI_Wtime();

    if (rank == 0) {
        std::cout << "1. Map to form A -> B C D" << std::endl;
    }

    mr->map(narg - 1, &args[1], 0, 1, 0, parse_file, NULL);
    mr->collate(NULL);
    mr->reduce(mpi_reduce, NULL);

    /** STEP 2 **/
    MPI_Barrier(MPI_COMM_WORLD);
    double tstop = MPI_Wtime();

    if (rank == 0) {
        std::cout << "2. Map to form (A,B) -> B C D" << std::endl;
    }

    mr->map(mr, mpi_map, NULL);

    /** STEP 3 **/
    if (rank == 0) {
        std::cout << "3. Reduce to form (A,B) -> (B C D)(B D E)" << std::endl;
    }

    mr->collate(NULL);
    mr->reduce(mpi_reduce_intersect, NULL);

    /** FINAL STEP **/
    if (OUTPUT_TO_FILE) {
        char fname[] = "output.txt";
        FILE* fp = fopen(fname, "w");

        if (fp == NULL) {
            std::cerr << "ERROR: Could not open output file" << std::endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        mr->gather(1);
        mr->map(mr, mpi_output, (void*)fp);
        fclose(fp);
    }

    delete mr;
    if (rank == 0) {
        std::cout << "Time to process " << mr->mapfilecount
                  << " files on " << nprocs
                  << " = " << tstop - tstart << " (secs)" << std::endl;
    }

    MPI_Finalize();
}
