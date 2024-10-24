#include <iosteam>

class NestLoop_inner{
    private:
        int cost;
        int rows;
        struct NestLoop_inner* left_subtree;
        struct NestLoop_inner* right_subtree;
    public:
        int get_cost(){
            return cost;
        }
        int rows(){
            return rows;
        }
        struct NestLoop_inner* grt_left_subtree(){
            return left_subtree;
        }
        struct NestLoop_inner* grt_right_subtree(){
            return right_subtree;
        }
};

double cost(NestLoop_inner* nest){
    right_subtree = nest.grt_right_subtree();
    left_subtree = nest.get_left_subtree();

    cost_value = nest.get_cost() + right_subtree.get_cost() + right_subtree_get_rows() * 1.1 
    + (left_subtree.get_rows() - 1) * (right_subtree.get_rows()) * 1 + (right_subtree.get_rows() * left_subtree.get_rows()) * 0.1;
    
    return cost_value
}

int main(){
    NestLoop_inner nest = NestLoop_inner();
}