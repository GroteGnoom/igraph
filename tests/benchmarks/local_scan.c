#include <igraph.h>

#include "bench.h"

int main(void) {
    igraph_t graph;
    igraph_vector_t result;
    igraph_vector_t weights;

    BENCH_INIT();

    igraph_vector_init(&result, 0);
    igraph_ring(&graph, 50000, IGRAPH_UNDIRECTED, false, false);
    igraph_vector_init(&weights, igraph_ecount(&graph));
    igraph_vector_fill(&weights, 2);
    printf("line graph:\n");
    BENCH("local_scan_1",
          igraph_local_scan_1_ecount(&graph, &result, &weights, IGRAPH_ALL));

    BENCH("local_scan_k",
          igraph_local_scan_k_ecount(&graph, 1, &result, &weights, IGRAPH_ALL));

    igraph_destroy(&graph);
    igraph_full(&graph, 1000, IGRAPH_UNDIRECTED, false);
    igraph_vector_resize(&weights, igraph_ecount(&graph));
    igraph_vector_fill(&weights, 2);
    printf("full:\n");
    BENCH("local_scan_1",
          igraph_local_scan_1_ecount(&graph, &result, &weights, IGRAPH_ALL));

    BENCH("local_scan_k",
          igraph_local_scan_k_ecount(&graph, 1, &result, &weights, IGRAPH_ALL));

    igraph_destroy(&graph);
    igraph_vector_destroy(&weights);
    igraph_vector_destroy(&result);

    return 0;
}
