#include <stdio.h>

#include <atomic>

#include <string.h>
#include <stdlib.h>

using namespace std;

template <typename T>
class MPSCQueue{
public:
    struct Node{
        Node *next;
        T data;
    };
    
    MPSCQueue(){
        push_at = &empty_node;
        pop_at = &empty_node;
        empty_node.next = nullptr;
    }
    
    void push(T val){
        Node *node = (Node*)malloc(sizeof(Node));
        node->data = val;
        node->next = nullptr;
        
        push_at.exchange( node )->next = node;
    }
    void pop(){
        Node *next = pop_at->next;
        
        if( pop_at == &empty_node ){
            if( next && next->next ){
                free( pop_at->next );
                
                pop_at = next->next;
            }
        }
        else{
            if( next == nullptr ){
                empty_node.next = nullptr;
                push_at.exchange( &empty_node );
                
                pop_at = &empty_node;
            }
            else{
                free( pop_at );
                pop_at = next;
            }
        }
    }
    
    T &front(){
        if( pop_at == &empty_node ){
            return empty_node.next->data;
        }
        else
            return pop_at->data;
    }
    
//protected:
    Node *pop_at;
    atomic<Node*> push_at;
    
    Node empty_node;
};

#include <thread>
#include <vector>

int countQueueSize(MPSCQueue<int> &q){
    MPSCQueue<int>::Node *node = q.pop_at;
    int cnt = 0;
    
    while( true ){
        node = node->next;
        if( node == nullptr )
            break;
        
        cnt ++;
    }
    
    return cnt;
}

int main(){
    MPSCQueue<int> q;
    std::vector<std::thread> ts;
    
    printf("insert\n");
    for(int i=0;i<20;i++){
        ts.push_back(std::thread( [&q](){
            for(int i=0;i<10000000;i++){
                q.push( i % 128 );
            }
        }) );
    }
    
    for(auto &t : ts){
        t.join();
    }
    
    printf("count\n");
    
    int cnt = countQueueSize(q);
    printf("q size : %d\n", cnt);
    
    printf("pop\n");
    int sum[128];
    
    memset( sum, 0, sizeof(int) * 128 );
    for(int i=0;i<cnt;i++){
        sum[ q.front() ] ++;
        q.pop();
    }
    
    printf("check\n");
    for(int i=0;i<128;i++){
        if( sum[0] != sum[i] ){
            printf("data error\n");
            break;
        }
    }
    
    printf("q size : %d\n", countQueueSize(q));
}
