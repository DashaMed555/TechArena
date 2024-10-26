#include <iostream>
#include <fstream>
#include <set>
#include <unordered_map>

using cardinalities_map = std::unordered_map<unsigned long long, std::unordered_map<char, unsigned long long>>; // table_id -> (attr_id -> card)
using rows_num_map = std::unordered_map<unsigned long long, double>; // table_id -> rows_num
using join = std::pair<std::pair<unsigned long long, unsigned long long>, std::pair<char, char>>;

class Optimizer {
    public:
        Optimizer(unsigned long long tables_num, rows_num_map rows_num, unsigned long long attributes_num, cardinalities_map& attributes_cardinality, \
                  std::unordered_map<unsigned long long, std::set<char>>& attributes_in_predicates, std::set<join>& joins) {

            this->cardinalities = std::move(attributes_cardinality);

            double cost;
            for (unsigned long long i = 1; i <= tables_num; ++i) {
                cost = rows_num[i];
                if (attributes_in_predicates.find(i) != attributes_in_predicates.end()) {
                    for (auto attribute : attributes_in_predicates[i])
                        rows_num[i] /= this->cardinalities[i][attribute];
                    cost *= 2;
                }
                this->nodes.insert(std::make_pair(node_max_current_id++, new Node(rows_num[i], cost, std::to_string(i))));
            }
            
            this->rows_num = std::move(rows_num);

            for (auto join : joins)
                this->clusters[join.first.first][join.first.second].insert(join);
        }

        std::pair<std::string, double> solve() {
            std::set<join> preferable_cluster;
            double min_rows_num;
            double rows;
            Node* left_subtree;
            Node* right_subtree;
            while (not clusters.empty()) {
                for (auto& [left_node_id, cluster_map] : clusters) {
                    for (auto& [right_node_id, join_set] : cluster_map) {

                    }
                }
            }
            // Проверить cross joins

            std::pair<std::string, double> fake{" ", 10};
            return fake;
        }

    private:
        class Node {
            private:
                double rows;
                double cost;
                std::string view;
                std::set<unsigned long long> tables;
            public:
                Node(double rows, double cost, std::string view) {
                    this->rows = rows;
                    this->cost = cost;
                    this->view = std::move(view);
                }

                double get_rows() {
                    return rows;
                }

                double get_cost() {
                    return cost;
                }

                std::string get_view() {
                    return view;
                }
        };

        cardinalities_map cardinalities;
        rows_num_map rows_num;
        std::unordered_map<unsigned long long, Node*> nodes;
        unsigned long long node_max_current_id = 1;
        std::unordered_map<unsigned long long, std::unordered_map<unsigned long long, std::set<join>>> clusters; // left_node_id => (right_node_id => std::set<join>)

        Node* nestLoop_inner_join(Node* left_subtree, Node* right_subtree, unsigned long long join_clauses_num, \
                                  std::pair<unsigned long long, unsigned long long>* table_nums, std::pair<char, char>* attribute_chars) {
            double rows = left_subtree->get_rows() * right_subtree->get_rows();
            for (int i = 0; i < join_clauses_num; ++i) {
                rows /= std::max(cardinalities[table_nums[i].first][attribute_chars[i].first], \
                                 cardinalities[table_nums[i].second][attribute_chars[i].second]);
            }
            double cost = left_subtree->get_cost() + right_subtree->get_cost() + \
                          right_subtree->get_rows() * 1.1 + (left_subtree->get_rows() - 1) * right_subtree->get_rows() + rows * 0.1;
            
            std::string clauses;
            for (int i = 0; i < join_clauses_num; ++i)
                clauses += std::string(" {") + std::to_string(table_nums[i].first) + std::string(".") + std::string(1, attribute_chars[i].first) + \
                           std::string(" ") + std::to_string(table_nums[i].second) + std::string(".") + std::string(1, attribute_chars[i].second) + std::string("}");
            std::string view = std::string("(") + left_subtree->get_view() + std::string(" ") + right_subtree->get_view() + clauses + std::string(")");

            Node* node = new Node(rows, cost, view);
            return node;
        }

        Node* hash_inner_join(Node* left_subtree, Node* right_subtree, unsigned long long join_clauses_num, \
                              std::pair<unsigned long long, unsigned long long>* table_nums, std::pair<char, char>* attribute_chars) {
            double rows = left_subtree->get_rows() * right_subtree->get_rows();
            for (int i = 0; i < join_clauses_num; ++i) {
                rows /= std::max(cardinalities[table_nums[i].first][attribute_chars[i].first], \
                                 cardinalities[table_nums[i].second][attribute_chars[i].second]);
            }
            double cost = right_subtree->get_cost() + right_subtree->get_rows() * 1.5 + \
                          left_subtree->get_cost() + left_subtree->get_rows() * 3.5 + rows * 0.1;

            std::string clauses;
            for (int i = 0; i < join_clauses_num; ++i)
                clauses += std::string(" {") + std::to_string(table_nums[i].first) + std::string(".") + std::string(1, attribute_chars[i].first) + \
                           std::string(" ") + std::to_string(table_nums[i].second) + std::string(".") + std::string(1, attribute_chars[i].second) + std::string("}");
            std::string view = std::string("(") + left_subtree->get_view() + std::string(" ") + right_subtree->get_view() + clauses + std::string(")");

            Node* node = new Node(rows, cost, view);
            return node;
        }

        Node* cross_join(Node* left_subtree, Node* right_subtree) {
            double rows = left_subtree->get_rows() * right_subtree->get_rows();
            double cost = left_subtree->get_cost() + right_subtree->get_cost() + right_subtree->get_rows() * 0.2 + \
                          (left_subtree->get_rows() - 1) * right_subtree->get_rows() * 0.1;

            std::string view = std::string("(") + left_subtree->get_view() + std::string(" ") + right_subtree->get_view() + std::string(")");

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
