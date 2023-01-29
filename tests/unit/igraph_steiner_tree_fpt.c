/*
   IGraph library.
   Copyright (C) 2022  The igraph development team <igraph@igraph.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <igraph.h>
#include "test_utilities.h"

int is_tree(igraph_t *graph, igraph_vector_int_t *tree_edges) {
    igraph_t tree;
    igraph_subgraph_from_edges(graph, &tree, igraph_ess_vector(tree_edges), /* delete_vertices= */ true);
    igraph_bool_t res;
    igraph_is_tree(&tree, &res, NULL, IGRAPH_ALL);
    igraph_destroy(&tree);
    return res;
}

int reaches_terminals(igraph_t *graph, igraph_vector_int_t *terminals, igraph_vector_int_t *tree_edges) {
    for (int t = 0; t < igraph_vector_int_size(terminals); t++) {
        int terminal = VECTOR(*terminals)[t];
        int found = 0;
        for (int e = 0; e < igraph_vector_int_size(tree_edges); e ++) {
            igraph_integer_t edge = VECTOR(*tree_edges)[e];
            if (IGRAPH_FROM(graph, edge) == terminal || IGRAPH_TO(graph, edge) == terminal) {
                found = 1;
                break;
            }
        }
        if (!found) {
            return 0;
        }
    }
    return 1;
}

igraph_real_t get_tree_weight(igraph_vector_int_t *tree_edges, igraph_vector_t *weights) {

    igraph_real_t value = 0.0;
    igraph_integer_t tree_size = igraph_vector_int_size(tree_edges);
    for (igraph_integer_t i = 0; i < tree_size; i++) {
        value += VECTOR(*weights)[VECTOR(*tree_edges)[i]];
    }
    return value;
}

int backtrack(igraph_t *graph, igraph_vector_int_t *terminals, igraph_vector_t *weights, igraph_vector_int_t *tree_edges, igraph_integer_t edge, igraph_real_t max, igraph_bool_t better) {
    igraph_real_t tree_weight = get_tree_weight(tree_edges, weights);
    if (better && tree_weight >= max) {
        return 0;
    }
    if (tree_weight > max) {
        return 0;
    }
    if (reaches_terminals(graph, terminals, tree_edges) &&
        is_tree(graph, tree_edges)) {
        return 1;
    }
    if (edge >= igraph_ecount(graph)) {
        return 0;
    }

    igraph_vector_int_t tree_edges_cp;
    igraph_vector_int_init_copy(&tree_edges_cp, tree_edges);

    if (backtrack(graph, terminals, weights, tree_edges, edge+1, max, better)) {
        igraph_vector_int_destroy(&tree_edges_cp);
        return 1;
    }

    igraph_vector_int_destroy(tree_edges);
    igraph_vector_int_init_copy(tree_edges, &tree_edges_cp);
    igraph_vector_int_destroy(&tree_edges_cp);

    igraph_vector_int_push_back(tree_edges, edge);
    if (backtrack(graph, terminals, weights, tree_edges, edge+1, max, better)) {
        return 1;
    }
    return 0;
}

void compare_brute_force(igraph_t *graph, igraph_vector_int_t *terminals, igraph_vector_t *weights, igraph_real_t tree_weight) {
    if (igraph_ecount(graph) == 0) {
        return;
    }
    igraph_vector_int_t tree_edges;

    igraph_vector_int_init(&tree_edges, 0);

    int found_equal = backtrack(graph, terminals, weights, &tree_edges, 0, tree_weight, 0);
    igraph_vector_int_clear(&tree_edges);
    IGRAPH_ASSERT(found_equal);

    int found_better = backtrack(graph, terminals, weights, &tree_edges, 0, tree_weight, 1);
    IGRAPH_ASSERT(!found_better);

    igraph_vector_int_destroy(&tree_edges);
}

void check_graph(igraph_t *graph, igraph_vector_int_t *terminals, igraph_vector_t *weights) {
    igraph_real_t value;
    igraph_vector_int_t tree_edges;

    igraph_vector_int_init(&tree_edges, 0);

    igraph_error_t x = igraph_steiner_dreyfus_wagner(graph, terminals, weights, &value, &tree_edges);
    IGRAPH_ASSERT(x == IGRAPH_SUCCESS);
    compare_brute_force(graph, terminals, weights, value);
    IGRAPH_ASSERT(reaches_terminals(graph, terminals, &tree_edges));

    print_vector_int(&tree_edges);
    printf("Total Steiner tree weight: %g\n", value);

    /* Check total tree weight. */
    igraph_real_t value2 = 0.0;
    igraph_integer_t tree_size = igraph_vector_int_size(&tree_edges);
    for (igraph_integer_t i = 0; i < tree_size; i++) {
        value2 += VECTOR(*weights)[ VECTOR(tree_edges)[i] ];
    }
    IGRAPH_ASSERT(value == value2);

    /* Check that the result is indeed a tree. */
    if (igraph_vector_int_size(terminals) > 0) {
        igraph_t tree;
        igraph_subgraph_from_edges(graph, &tree, igraph_ess_vector(&tree_edges), /* delete_vertices= */ true);

        igraph_bool_t is_tree;
        igraph_is_tree(&tree, &is_tree, NULL, IGRAPH_ALL);
        IGRAPH_ASSERT(is_tree);

        igraph_destroy(&tree);
    }

    igraph_vector_int_destroy(&tree_edges);
}


int main(void) {
    igraph_t g_null, g_k7, g_k6_k1, g_k7_n, g_k7_n1, g_k7_real, g_k7_non_simple;
    igraph_vector_int_t terminals_null, terminals_k7, terminals_k6_k1, terminals_k7_real;
    igraph_vector_t weights_null, weights_k7, weights_k6_k1, weights_k7_n, weights_k7_n1, weights_k7_real, weights_k7_non_simple;

    /* Null graph */
    igraph_empty(&g_null, 0, 0);
    igraph_vector_init(&weights_null, 0);
    igraph_vector_int_init(&terminals_null, 0);

    /* K_7 complete graph with a specific edge ordering. */
    igraph_small(&g_k7, 7, IGRAPH_UNDIRECTED,
                 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6,
                 1, 2, 1, 3, 1, 4, 1, 5, 1, 6,
                 2, 3, 2, 4, 2, 5, 2, 6,
                 3, 4, 3, 5, 3, 6,
                 4, 5, 4, 6,
                 5, 6,
                 -1);
    // 1 , 1
    igraph_vector_init_int(&weights_k7, 21,
                           2, 2, 2, 1, 1, 2,
                           2, 2, 2, 1, 2,
                           2, 2, 2, 1,
                           1, 2, 1,
                           2, 1,
                           1);

    igraph_vector_int_init_int(&terminals_k7, 4,
                               0, 1, 2, 3);


    /* K_7 non-complete graph with edges 0-4,0-5 removed */
    igraph_small(&g_k7_n, 7, IGRAPH_UNDIRECTED,
                 0, 1, 0, 2, 0, 3, 0, 6,
                 1, 2, 1, 3, 1, 4, 1, 5, 1, 6,
                 2, 3, 2, 4, 2, 5, 2, 6,
                 3, 4, 3, 5, 3, 6,
                 4, 5, 4, 6,
                 5, 6,
                 -1);

    igraph_vector_init_int(&weights_k7_n, 19,
                           2, 2, 2, 2,
                           2, 2, 2, 1, 2,
                           2, 2, 2, 1,
                           1, 2, 1,
                           2, 1,
                           1);


    /* K_7 non-complete graph with edges 0-4 removed */
    igraph_small(&g_k7_n1, 7, IGRAPH_UNDIRECTED,
                 0, 1, 0, 2, 0, 3, 0, 4, 0, 6,
                 1, 2, 1, 3, 1, 4, 1, 5, 1, 6,
                 2, 3, 2, 4, 2, 5, 2, 6,
                 3, 4, 3, 5, 3, 6,
                 4, 5, 4, 6,
                 5, 6,
                 -1);

    igraph_vector_init_int(&weights_k7_n1, 20,
                           2, 2, 2, 1, 2,
                           2, 2, 2, 1, 2,
                           2, 2, 2, 1,
                           1, 2, 1,
                           2, 1,
                           1);


    /* Disconnected graph: K_6 with a specific edge order and an additional isolated vertex. */
    igraph_small(&g_k6_k1, 7, IGRAPH_UNDIRECTED,
                 0, 1, 0, 2, 0, 3, 0, 4, 0, 5,
                 1, 2, 1, 3, 1, 4, 1, 5,
                 2, 3, 2, 4, 2, 5,
                 3, 4, 3, 5,
                 4, 5,
                 -1);

    igraph_vector_init_int(&weights_k6_k1, 15,
                           2, 2, 2, 1, 1,
                           2, 2, 2, 1,
                           2, 2, 2,
                           1, 2,
                           2);

    igraph_vector_int_init_int(&terminals_k6_k1, 4,
                               0, 1, 2, 6);

    igraph_small(&g_k7_real, 7, IGRAPH_UNDIRECTED,
                 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 6,
                 1, 2, 1, 3, 1, 4, 1, 5, 1, 6,
                 2, 3, 2, 4, 2, 5, 2, 6,
                 3, 4, 3, 5, 3, 6,
                 4, 5, 4, 6,
                 5, 6,
                 -1);
    // 1 , 1
    igraph_vector_init_real(&weights_k7_real, 21,
                            2.5, 2.5, 2.5, 0.5, 0.5, 2.5,
                            2.5, 2.5, 2.5, 0.5, 2.5,
                            2.5, 2.5, 2.5, 0.5,
                            0.5, 2.5, 0.5,
                            2.5, 0.5,
                            0.5);

    igraph_vector_int_init_int(&terminals_k7_real, 4,
                               0, 1, 2, 3);


    /* K_7 complete graph with a specific edge ordering. */
    igraph_small(&g_k7_non_simple, 7, IGRAPH_UNDIRECTED,
                 0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0, 5, 0, 6,
                 1, 2, 1, 3, 1, 4, 1, 5, 1, 6,
                 2, 3, 2, 4, 2, 5, 2, 6,
                 3, 4, 3, 5, 3, 6,
                 4, 5, 4, 6,
                 5, 6,
                 -1);
    // 1 , 1
    igraph_vector_init_int(&weights_k7_non_simple, 22,
                           2, 2, 2, 1, 1, 2, 2,
                           2, 2, 2, 1, 2,
                           2, 2, 2, 1,
                           1, 2, 1,
                           2, 1,
                           1);


    printf("Null graph:\n");
    check_graph(&g_null, &terminals_null, &weights_null);

    printf("\nK_7 graph:\n");
    check_graph(&g_k7, &terminals_k7, &weights_k7);

    printf("\nK_7 Non Complete graph-1:\n");
    check_graph(&g_k7_n, &terminals_k7, &weights_k7_n);

    printf("\nK_7 Non Complete graph-2:\n");
    check_graph(&g_k7_n1, &terminals_k7, &weights_k7_n1);

    igraph_real_t value;
    igraph_vector_int_t tree_edges;
    igraph_vector_int_init(&tree_edges, 0);

    CHECK_ERROR(igraph_steiner_dreyfus_wagner(&g_k6_k1, &terminals_k6_k1, &weights_k6_k1, &value, &tree_edges), IGRAPH_EINVAL);

    igraph_vector_int_destroy(&tree_edges);


    igraph_real_t value_unit;
    igraph_vector_int_t tree_edges_unit;
    igraph_vector_int_init(&tree_edges_unit, 0);

    printf("\nK_7_Unit graph:\n");
    igraph_steiner_dreyfus_wagner(&g_k7, &terminals_k7, NULL, &value_unit, &tree_edges_unit);
    print_vector_int(&tree_edges_unit);
    printf("Total Steiner tree weight: %g\n", value_unit);

    igraph_vector_int_destroy(&tree_edges_unit);

    printf("\nK_7_Real graph:\n");
    check_graph(&g_k7_real, &terminals_k7_real, &weights_k7_real);

    printf("\nK_7 graph with parallel edges:\n");
    check_graph(&g_k7_non_simple, &terminals_k7, &weights_k7_non_simple);


    igraph_destroy(&g_null);
    igraph_destroy(&g_k7);
    igraph_destroy(&g_k6_k1);
    igraph_destroy(&g_k7_real);
    igraph_destroy(&g_k7_n1);
    igraph_destroy(&g_k7_n);
    igraph_destroy(&g_k7_non_simple);

    igraph_vector_destroy(&weights_null);
    igraph_vector_destroy(&weights_k7);
    igraph_vector_destroy(&weights_k6_k1);
    igraph_vector_destroy(&weights_k7_n);
    igraph_vector_destroy(&weights_k7_n1);
    igraph_vector_destroy(&weights_k7_real);
    igraph_vector_destroy(&weights_k7_non_simple);

    igraph_vector_int_destroy(&terminals_k7);
    igraph_vector_int_destroy(&terminals_null);
    igraph_vector_int_destroy(&terminals_k6_k1);
    igraph_vector_int_destroy(&terminals_k7_real);

    VERIFY_FINALLY_STACK();

    return 0;
}
