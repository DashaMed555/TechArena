#include <iostream>
#include <fstream>
#include <set>
#include <unordered_map>
#include <float.h>

using cardinalities_map = std::unordered_map<unsigned long long, std::unordered_map<char, unsigned long long>>; // table_id -> (attr_id -> card)
using rows_num_map = std::unordered_map<unsigned long long, double>; // table_id -> rows_num
using join = std::pair<std::pair<unsigned long long, unsigned long long>, std::pair<char, char>>;

class Optimizer {
    public:
        Optimizer(unsigned long long tables_num, rows_num_map rows_num, unsigned long long attributes_num, \
                  cardinalities_map& attributes_cardinality, std::unordered_map<unsigned long long, \
                  std::set<char>>& attributes_in_predicates, \
                  std::set<join>& joins) {

            this->cardinalities = std::move(attributes_cardinality);

            double cost;
            std::string view;
            for (unsigned long long i = 1; i <= tables_num; ++i) {
                cost = rows_num[i];
                view = std::to_string(i);
                if (attributes_in_predicates.find(i) != attributes_in_predicates.end()) {
                    for (auto attribute : attributes_in_predicates[i]) {
                        view += std::string(1, attribute);
                        rows_num[i] /= this->cardinalities[i][attribute];
                    }
                    cost *= 2;
                }
                this->nodes.insert(std::make_pair(node_max_current_id++, new Node(rows_num[i], cost, view)));
            }
            
            this->rows_num = std::move(rows_num);

            for (auto join : joins)
                this->clusters[join.first.first][join.first.second].insert(join);
        }

        std::pair<std::string, double> solve() {
            std::pair<unsigned long long, unsigned long long> preferable_node_ids;
            double min_rows_num;
            double rows;
            Node* left_subtree;
            Node* right_subtree;
            while (not clusters.empty()) {
                min_rows_num = DBL_MAX;
                for (auto& [left_node_id, cluster_map] : clusters) {
                    for (auto& [right_node_id, join_set] : cluster_map) {
                        left_subtree = nodes[left_node_id];
                        right_subtree = nodes[right_node_id];
                        rows = left_subtree->rows * right_subtree->rows;
                        for (auto& join : join_set) {
                            rows /= std::max(cardinalities[join.first.first][join.second.first], \
                                             cardinalities[join.first.second][join.second.second]);
                        }
                        if (rows < min_rows_num) {
                            min_rows_num = rows;
                            preferable_node_ids = std::make_pair(left_node_id, right_node_id);
                        }
                    }
                }
                left_subtree = nodes[preferable_node_ids.first];
                right_subtree = nodes[preferable_node_ids.second];
                auto join_set = clusters[preferable_node_ids.first][preferable_node_ids.second];
                clusters[preferable_node_ids.first].erase(preferable_node_ids.second);
                if (clusters[preferable_node_ids.first].empty())
                    clusters.erase(preferable_node_ids.first);
                if (clusters.find(preferable_node_ids.first) != clusters.end()) {
                    for (auto& [right_node_id, join_set] : clusters[preferable_node_ids.first])
                        clusters[right_node_id][node_max_current_id].insert(join_set.begin(), join_set.end());
                    clusters.erase(preferable_node_ids.first);
                }
                if (clusters.find(preferable_node_ids.second) != clusters.end()) {
                    for (auto& [right_node_id, join_set] : clusters[preferable_node_ids.second])
                        clusters[right_node_id][node_max_current_id].insert(join_set.begin(), join_set.end());
                    clusters.erase(preferable_node_ids.second);
                }
                for (auto& [left_node_id, cluster_map] : clusters) {
                    for (auto& [right_node_id, join_set] : cluster_map) {
                        if (right_node_id == preferable_node_ids.first or right_node_id == preferable_node_ids.second) {
                            cluster_map[node_max_current_id] = join_set;
                            cluster_map.erase(right_node_id);
                        }
                    }
                }
                if (std::min(right_subtree->rows, left_subtree->rows) * \
                   (std::max(right_subtree->rows, left_subtree->rows) - 3.4) - \
                    std::max(right_subtree->rows, left_subtree->rows) * 1.5 < 0) { // then NestLoop is better
                    if (right_subtree->rows > left_subtree->rows) {
                        std::swap(left_subtree, right_subtree);
                        std::set<join> new_join_set;
                        for (auto join : join_set) {
                            new_join_set.insert(std::make_pair(std::make_pair(join.first.second, join.first.first), \
                                                               std::make_pair(join.second.second, join.second.first)));
                        }
                        join_set = std::move(new_join_set);
                    }
                    nodes[node_max_current_id++] = nestLoop_inner_join(left_subtree, right_subtree, join_set);
                }
                else { // otherwise, HashJoin is better
                    if (right_subtree->rows < left_subtree->rows) {
                        std::swap(left_subtree, right_subtree);
                        std::set<join> new_join_set;
                        for (auto join : join_set) {
                            new_join_set.insert(std::make_pair(std::make_pair(join.first.second, join.first.first), \
                                                               std::make_pair(join.second.second, join.second.first)));
                        }
                        join_set = std::move(new_join_set);
                    }
                    nodes[node_max_current_id++] = hash_inner_join(left_subtree, right_subtree, join_set);
                }
                nodes.erase(preferable_node_ids.first);
                nodes.erase(preferable_node_ids.second);
                
            }
            while (this->nodes.size() != 1) {
                min_rows_num = DBL_MAX;
                for (auto& [left_node_id, left_node] : this->nodes) {
                    for (auto& [right_node_id, right_node] : this->nodes) {
                        if (left_node_id != right_node_id) {
                            rows = left_node->rows * right_node->rows;
                            if (rows < min_rows_num) {
                                min_rows_num = rows;
                                preferable_node_ids = std::make_pair(left_node_id, right_node_id);
                            }
                        }
                    }
                }
                left_subtree = nodes[preferable_node_ids.first];
                right_subtree = nodes[preferable_node_ids.second];
                if (right_subtree->rows > left_subtree->rows)
                    std::swap(left_subtree, right_subtree);
                nodes[node_max_current_id++] = cross_join(left_subtree, right_subtree);
                nodes.erase(preferable_node_ids.first);
                nodes.erase(preferable_node_ids.second);
            }
            Node* result = this->nodes[node_max_current_id - 1];
            return std::make_pair(result->view, result->cost);
        }

    private:
        typedef struct Node {
            double rows;
            double cost;
            std::string view;
        } Node;

        cardinalities_map cardinalities;
        rows_num_map rows_num;
        std::unordered_map<unsigned long long, Node*> nodes;
        unsigned long long node_max_current_id = 1;
        std::unordered_map<unsigned long long, std::unordered_map<unsigned long long, std::set<join>>> clusters;
        // left_node_id => (right_node_id => std::set<join>)

        Node* nestLoop_inner_join(Node* left_subtree, Node* right_subtree, std::set<join>& join_set) {
            double rows = left_subtree->rows * right_subtree->rows;
            std::string clauses;
            for (auto& join : join_set) {
                rows /= std::max(cardinalities[join.first.first][join.second.first], \
                                 cardinalities[join.first.second][join.second.second]);
                clauses += std::string(" {") + std::to_string(join.first.first) + std::string(".") + std::string(1, join.second.first) + \
                           std::string(" ") + std::to_string(join.first.second) + std::string(".") + std::string(1, join.second.second) + \
                           std::string("}");
            }
            double cost = left_subtree->cost + right_subtree->cost + \
                          right_subtree->rows * (left_subtree->rows + 0.1) + rows * 0.1;
            
            std::string view = std::string("(") + left_subtree->view + std::string(" ") + \
                               right_subtree->view + clauses + std::string(")");

            Node* node = new Node(rows, cost, view);
            return node;
        }

        Node* hash_inner_join(Node* left_subtree, Node* right_subtree, std::set<join>& join_set) {
            double rows = left_subtree->rows * right_subtree->rows;
            std::string clauses;
            for (auto& join : join_set) {
                rows /= std::max(cardinalities[join.first.first][join.second.first], \
                                 cardinalities[join.first.second][join.second.second]);
                clauses += std::string(" {") + std::to_string(join.first.first) + std::string(".") + std::string(1, join.second.first) + \
                           std::string(" ") + std::to_string(join.first.second) + std::string(".") + std::string(1, join.second.second) + \
                           std::string("}");
            }
            double cost = right_subtree->cost + right_subtree->rows * 1.5 + \
                          left_subtree->cost + left_subtree->rows * 3.5 + rows * 0.1;

            std::string view = std::string("(") + left_subtree->view + std::string(" ") + \
                               right_subtree->view + clauses + std::string(")");

            Node* node = new Node(rows, cost, view);
            return node;
        }

        Node* cross_join(Node* left_subtree, Node* right_subtree) {
            double rows = left_subtree->rows * right_subtree->rows;
            double cost = left_subtree->cost + right_subtree->cost + right_subtree->rows * 0.2 + \
                          (left_subtree->rows - 1) * right_subtree->rows * 0.1;

            std::string view = std::string("(") + left_subtree->view + std::string(" ") + right_subtree->view + std::string(")");

            Node* node = new Node(rows, cost, view);
            return node;
        }
};

int main() {
    std::ifstream input_stream;
    input_stream.open("input.txt");

    unsigned long long tables_num;
    input_stream >> tables_num;

    rows_num_map rows_num;
    for (unsigned long long i = 1; i <= tables_num; ++i)
        input_stream >> rows_num[i];

    unsigned long long attributes_num;
    input_stream >> attributes_num;

    cardinalities_map attributes_cardinality;
    unsigned long long table_num;
    char attribute;
    unsigned long long cardinality;
    for (unsigned long long i = 0; i < attributes_num; ++i) {
        input_stream >> table_num;
        input_stream >> attribute;
        input_stream >> cardinality;
        attributes_cardinality[table_num][attribute] = cardinality;
    }

    unsigned long long predicates_per_scan_num;
    input_stream >> predicates_per_scan_num;

    std::unordered_map<unsigned long long, std::set<char>> attributes_in_predicates;
    for (unsigned long long i = 0; i < predicates_per_scan_num; ++i) {
        input_stream >> table_num;
        input_stream >> attribute;
        attributes_in_predicates[table_num].insert(attribute);
    }

    unsigned long long join_predicates_num;
    input_stream >> join_predicates_num;

    std::set<join> joins;
    unsigned long long table_num_1;
    unsigned long long table_num_2;
    char join_table_1_attribute;
    char join_table_2_attribute;
    for (unsigned long long i = 0; i < join_predicates_num; ++i) {
        input_stream >> table_num_1;
        input_stream >> table_num_2;
        input_stream >> join_table_1_attribute;
        input_stream >> join_table_2_attribute;
        if (table_num_1 > table_num_2) {
            std::swap(table_num_1, table_num_2);
            std::swap(join_table_1_attribute, join_table_2_attribute);
        }
        joins.insert(std::make_pair(std::make_pair(table_num_1, table_num_2), std::make_pair(join_table_1_attribute, join_table_2_attribute)));
    }

    input_stream.close();

    Optimizer optimizer(tables_num, rows_num, attributes_num, attributes_cardinality, attributes_in_predicates, joins);

    std::pair<std::string, double> result = optimizer.solve();

    std::ofstream output_stream;
    output_stream.open("output.txt");
    output_stream << result.first << " " << result.second;
    output_stream.close();

    return 0;
}
