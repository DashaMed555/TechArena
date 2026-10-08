#include <iostream>
#include <fstream>
#include <set>
#include <unordered_map>
#include <float.h>
#include <iomanip>

// table_id -> (attr_id -> card)
using cardinalities_map = std::unordered_map<unsigned long long, std::unordered_map<char, unsigned long long>>;

// table_id -> rows_num
using rows_num_map = std::unordered_map<unsigned long long, double>;

// ((table_num_1, table_num_2), (join_table_1_attribute, join_table_2_attribute))
using join = std::pair<std::pair<unsigned long long, unsigned long long>, std::pair<char, char>>;

// left_node_id => (right_node_id => std::set<join>)
using cluster = std::unordered_map<unsigned long long, std::unordered_map<unsigned long long, std::set<join>>>;

// { (table_num_1, table_num_2) }
using cross_joins = std::set<std::pair<unsigned long long, unsigned long long>>;

typedef struct Node {
    double rows;
    double cost;
    std::string view;
} Node;

class Optimizer {
    public:
        Optimizer(unsigned long long tables_num, \
                  rows_num_map& rows_num, \
                  unsigned long long attributes_num, \
                  cardinalities_map& attributes_cardinality, \
                  std::unordered_map<unsigned long long, std::set<char>>& attributes_in_predicates, \
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
                this->nodes.insert(std::make_pair(node_max_current_id, new Node));
                this->nodes[node_max_current_id]->rows = rows_num[i];
                this->nodes[node_max_current_id]->cost = cost;
                this->nodes[node_max_current_id]->view = view;
                node_max_current_id++;
            }
            
            this->rows_num = std::move(rows_num);

            for (auto& join : joins)
                this->clusters[join.first.first][join.first.second].insert(join);
            
            for (int i = 1; i < tables_num; ++i)
                for (int j = i + 1; j <= tables_num; ++j)
                    if ((clusters.find(i) == clusters.end()) or (clusters[i].find(j) == clusters[i].end()))
                        possible_cross_joins.insert(std::make_pair(i, j));
        }
    
        std::pair<std::string, double> solve() {
            std::pair<std::string, double> result;
            result = step(nodes, clusters, possible_cross_joins, node_max_current_id);
            return result;
        }

    private:
        cardinalities_map cardinalities;
        rows_num_map rows_num;
        unsigned long long node_max_current_id = 1;
        std::unordered_map<unsigned long long, Node*> nodes;
        cluster clusters;
        cross_joins possible_cross_joins;
    
        std::pair<std::string, double> step(const std::unordered_map<unsigned long long, Node*>& nodes, \
                                            const cluster& clusters, \
                                            const cross_joins& possible_cross_joins, \
                                            const unsigned long long node_max_current_id) {
            if (nodes.size() == 1) {
                auto result_node = (*nodes.begin()).second;
                return std::make_pair(result_node->view, result_node->cost);
            }
            
            std::set<std::pair<std::string, double>> results;
            
            Node* left_subtree;
            Node* right_subtree;
            
            std::unordered_map<unsigned long long, Node*> next_nodes;
            cluster next_clusters;
            cross_joins next_possible_cross_joins;
            double next_node_max_current_id;
            
            std::set<join> temp_join_set;
            cluster temp_clusters;
            
            unsigned long long temp_first;
            unsigned long long temp_second;
            cross_joins temp_cross_join_set;
            
            for (auto& [left_node_id, cluster_map] : clusters) {
                for (auto& [right_node_id, join_set] : cluster_map) {
                    next_nodes = nodes;
                    next_node_max_current_id = node_max_current_id;
                    left_subtree = next_nodes[left_node_id];
                    right_subtree = next_nodes[right_node_id];
                    
                    next_clusters = clusters;
                    auto join_set_copy = next_clusters[left_node_id][right_node_id];
                    
                    next_clusters[left_node_id].erase(right_node_id);
                    if (next_clusters[left_node_id].empty())
                        next_clusters.erase(left_node_id);
                    else {
                        for (auto& [right_node_id_, join_set_] : next_clusters[left_node_id]) {
                            for (auto& join : join_set_) {
                                temp_join_set.insert(std::make_pair(std::make_pair(join.first.second, join.first.first), \
                                                                    std::make_pair(join.second.second, join.second.first)));
                            }
                            join_set_ = std::move(temp_join_set);
                            next_clusters[right_node_id_][next_node_max_current_id].insert(join_set_.begin(), join_set_.end());
                        }
                        next_clusters.erase(left_node_id);
                    }
                    if (next_clusters.find(right_node_id) != next_clusters.end()) {
                        for (auto& [right_node_id_, join_set_] : next_clusters[right_node_id]) {
                            for (auto& join : join_set_) {
                                temp_join_set.insert(std::make_pair(std::make_pair(join.first.second, join.first.first), \
                                                                    std::make_pair(join.second.second, join.second.first)));
                            }
                            join_set_ = std::move(temp_join_set);
                            next_clusters[right_node_id_][next_node_max_current_id].insert(join_set_.begin(), join_set_.end());
                        }
                        next_clusters.erase(right_node_id);
                    }
                    for (auto& [left_node_id_, cluster_map_] : next_clusters) {
                        for (auto& [right_node_id_, join_set_] : cluster_map_) {
                            if (right_node_id_ == left_node_id or right_node_id_ == right_node_id)
                                temp_clusters[left_node_id_][next_node_max_current_id].insert(join_set_.begin(), join_set_.end());
                            else
                                temp_clusters[left_node_id_][right_node_id_].insert(join_set_.begin(), join_set_.end());
                        }
                    }
                    next_clusters = std::move(temp_clusters);
                    
                    next_possible_cross_joins = possible_cross_joins;
                    next_possible_cross_joins.erase(std::make_pair(left_node_id, right_node_id));
                    next_possible_cross_joins.erase(std::make_pair(right_node_id, left_node_id));
                    for (auto& cross_join : next_possible_cross_joins) {
                        temp_first = cross_join.first;
                        temp_second = cross_join.second;
                        if (temp_first == left_node_id or temp_first == right_node_id)
                            temp_first = next_node_max_current_id;
                        if (temp_second == left_node_id or temp_second == right_node_id)
                            temp_second = next_node_max_current_id;
                        if (temp_first > temp_second)
                            std::swap(temp_first, temp_second);
                        if (temp_second != next_node_max_current_id or \
                            next_clusters.find(temp_first) == next_clusters.end() or \
                            next_clusters[temp_first].find(temp_second) == next_clusters[temp_first].end())
                            temp_cross_join_set.insert(std::make_pair(temp_first, temp_second));
                    }
                    next_possible_cross_joins = std::move(temp_cross_join_set);
                    
                    if (std::min(right_subtree->rows, left_subtree->rows) * \
                        (std::max(right_subtree->rows, left_subtree->rows) - 3.4) - \
                        std::max(right_subtree->rows, left_subtree->rows) * 1.5 < 0) { // then NestLoop is better
                        if (right_subtree->rows > left_subtree->rows) {
                            std::swap(left_subtree, right_subtree);
                            for (auto& join : join_set_copy) {
                                temp_join_set.insert(std::make_pair(std::make_pair(join.first.second, join.first.first), \
                                                                    std::make_pair(join.second.second, join.second.first)));
                            }
                            join_set_copy = std::move(temp_join_set);
                        }
                        next_nodes[next_node_max_current_id++] = nestLoop_inner_join(left_subtree, right_subtree, join_set_copy);
                    }
                    else { // otherwise, HashJoin is better
                        if (right_subtree->rows < left_subtree->rows) {
                            std::swap(left_subtree, right_subtree);
                            for (auto& join : join_set_copy) {
                                temp_join_set.insert(std::make_pair(std::make_pair(join.first.second, join.first.first), \
                                                                    std::make_pair(join.second.second, join.second.first)));
                            }
                            join_set_copy = std::move(temp_join_set);
                        }
                        next_nodes[next_node_max_current_id++] = hash_inner_join(left_subtree, right_subtree, join_set_copy);
                    }
                    next_nodes.erase(left_node_id);
                    next_nodes.erase(right_node_id);
                    results.insert(step(next_nodes, next_clusters, next_possible_cross_joins, next_node_max_current_id));
                }
            }
            
            for (auto& [left_node_id, right_node_id] : possible_cross_joins) {
                next_nodes = nodes;
                next_node_max_current_id = node_max_current_id;
                left_subtree = next_nodes[left_node_id];
                right_subtree = next_nodes[right_node_id];
                
                next_clusters = clusters;
                for (auto& [left_node_id_, cluster_map_] : next_clusters) {
                    for (auto& [right_node_id_, join_set_] : cluster_map_) {
                        if (right_node_id_ == left_node_id or right_node_id_ == right_node_id)
                            temp_clusters[left_node_id_][next_node_max_current_id].insert(join_set_.begin(), join_set_.end());
                        else
                            temp_clusters[left_node_id_][right_node_id_].insert(join_set_.begin(), join_set_.end());
                    }
                }
                next_clusters = std::move(temp_clusters);
                
                next_possible_cross_joins = possible_cross_joins;
                next_possible_cross_joins.erase(std::make_pair(left_node_id, right_node_id));
                next_possible_cross_joins.erase(std::make_pair(right_node_id, left_node_id));
                for (auto& cross_join : next_possible_cross_joins) {
                    temp_first = cross_join.first;
                    temp_second = cross_join.second;
                    if (temp_first == left_node_id or temp_first == right_node_id)
                        temp_first = next_node_max_current_id;
                    if (temp_second == left_node_id or temp_second == right_node_id)
                        temp_second = next_node_max_current_id;
                    if (temp_first > temp_second)
                        std::swap(temp_first, temp_second);
                    temp_cross_join_set.insert(std::make_pair(temp_first, temp_second));
                }
                next_possible_cross_joins = std::move(temp_cross_join_set);
                
                if (right_subtree->rows > left_subtree->rows)
                    std::swap(left_subtree, right_subtree);
                next_nodes[next_node_max_current_id++] = cross_join(left_subtree, right_subtree);
                next_nodes.erase(left_node_id);
                next_nodes.erase(right_node_id);
                results.insert(step(next_nodes, next_clusters, next_possible_cross_joins, next_node_max_current_id));
            }
            
            double min_cost = DBL_MAX;
            std::string min_view;
            
            for (auto& result : results) {
                if (result.second < min_cost) {
                    min_cost = result.second;
                    min_view = result.first;
                }
            }
            results.clear();
            
            return std::make_pair(min_view, min_cost);
        }
    
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

            Node* node = new Node;
            node->rows = rows;
            node->cost = cost;
            node->view = view;
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

            Node* node = new Node;
            node->rows = rows;
            node->cost = cost;
            node->view = view;
            return node;
        }

        Node* cross_join(Node* left_subtree, Node* right_subtree) {
            double rows = left_subtree->rows * right_subtree->rows;
            double cost = left_subtree->cost + right_subtree->cost + 0.1 * right_subtree->rows * (1 + left_subtree->rows);

            std::string view = std::string("(") + left_subtree->view + std::string(" ") + right_subtree->view + std::string(")");

            Node* node = new Node;
            node->rows = rows;
            node->cost = cost;
            node->view = view;
            return node;
        }
};

int main() {
    std::ifstream input_stream;
    input_stream.open("tests/1.in");

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
    output_stream.open("output2.txt");
    output_stream << result.first << " " << std::fixed << std::setprecision(2) << result.second;
    output_stream.close();

    return 0;
}

