#include <iostream>
#include <unordered_map>

std::unordered_map<unsigned long long, std::unordered_map<char, unsigned long long>> cardinalities;

class Node {
    private:
        double rows;
        double cost;
        Node* left_subtree;
        Node* right_subtree;
    public:
        Node(double rows, double cost) {
            this->rows = rows;
            this->cost = cost;
        }
        double get_rows() {
            return rows;
        }
       double get_cost() {
            return cost;
        }
        Node* get_left_subtree() {
            return left_subtree;
        }
        Node* get_right_subtree() {
            return right_subtree;
        }
};

Node* nestLoop_inner_join(Node* left_subtree, Node* right_subtree, unsigned long long n, std::pair<unsigned long long, unsigned long long>* table_nums, std::pair<char, char>* attributes_chars) {
    double rows = left_subtree->get_rows() * right_subtree->get_rows();
    for (int i = 0; i < n; ++i) {
        rows /= std::max(cardinalities.find(table_nums[i].first)->second.find(attributes_chars[i].first)->second, \
                         cardinalities.find(table_nums[i].second)->second.find(attributes_chars[i].second)->second);
    }
    double cost = left_subtree->get_cost() + right_subtree->get_cost() + \
                  right_subtree->get_rows() * 1.1 + (left_subtree->get_rows() - 1) * right_subtree->get_rows() + rows * 0.1;
    
    Node* node = new Node(rows, cost);
    return node;
}

Node* hash_inner_join(Node* left_subtree, Node* right_subtree, unsigned long long n, std::pair<unsigned long long, unsigned long long>* table_nums, std::pair<char, char>* attributes_chars) {
    double rows = left_subtree->get_rows() * right_subtree->get_rows();
    for (int i = 0; i < n; ++i) {
        rows /= std::max(cardinalities.find(table_nums[i].first)->second.find(attributes_chars[i].first)->second, \
                         cardinalities.find(table_nums[i].second)->second.find(attributes_chars[i].second)->second);
    }
    double cost = right_subtree->get_cost() + right_subtree->get_rows() * 1.5 + left_subtree->get_cost() + left_subtree->get_rows() * 3.5 + rows * 0.1;
    
    Node* node = new Node(rows, cost);
    return node;
}

int main() {
    Node* table1 = new Node(10, 10);
    Node* table2 = new Node(12, 12);
    Node* table3 = new Node(15, 15);
    Node* table4 = new Node(8, 8);

    cardinalities[1]['a'] = 3;
    cardinalities[2]['b'] = 5;
    cardinalities[3]['b'] = 10;
    cardinalities[2]['c'] = 1;
    cardinalities[4]['d'] = 5;

    std::pair<unsigned long long, unsigned long long> table_nums1[1] = {std::make_pair(1, 3)};
    std::pair<char, char> attributes_chars1[1] = {std::make_pair('a', 'b')};
    Node* nestLoop_join1_3 = nestLoop_inner_join(table1, table3, 1, table_nums1, attributes_chars1);
    Node* hash_join1_3 = hash_inner_join(table1, table3, 1, table_nums1, attributes_chars1);
    std::cout << "nestLoop_join1_3->get_cost() = " << nestLoop_join1_3->get_cost() << std::endl;
    std::cout << "hash_join1_3->get_cost() = " << hash_join1_3->get_cost() << std::endl;

    std::pair<unsigned long long, unsigned long long> table_nums2[1] = {std::make_pair(4, 2)};
    std::pair<char, char> attributes_chars2[1] = {std::make_pair('d', 'c')};
    Node* nestLoop_join4_2 = nestLoop_inner_join(table4, table2, 1, table_nums2, attributes_chars2);
    Node* hash_join4_2 = hash_inner_join(table4, table2, 1, table_nums2, attributes_chars2);
    std::cout << "nestLoop_join4_2->get_cost() = " << nestLoop_join4_2->get_cost() << std::endl;
    std::cout << "hash_join4_2->get_cost() = " << hash_join4_2->get_cost() << std::endl;

    std::pair<unsigned long long, unsigned long long> table_nums3[1] = {std::make_pair(1, 2)};
    std::pair<char, char> attributes_chars3[1] = {std::make_pair('a', 'b')};
    Node* nestLoop_result = nestLoop_inner_join(hash_join1_3, hash_join4_2, 1, table_nums3, attributes_chars3);
    Node* hash_result = hash_inner_join(hash_join1_3, hash_join4_2, 1, table_nums3, attributes_chars3);
    std::cout << "nestLoop_result->get_cost() = " << nestLoop_result->get_cost() << std::endl;
    std::cout << "hash_result->get_cost() = " << hash_result->get_cost() << std::endl;

    delete table1;
    delete table2;
    delete table3;
    delete table4;

    delete nestLoop_join1_3;
    delete nestLoop_join4_2;
    delete nestLoop_result;

    return 0;
}