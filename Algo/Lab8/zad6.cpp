#include <iostream>
#include <vector>
#include <queue>
#include <climits>
using namespace std;

struct Edge {
    int to;
    long long cap;
    int reversed;
};

struct Discount { 
    int a, b; 
    long long price; 
};

int main() {
    int N, M, P;
    cin >> N >> M >> P;
    vector<long long> delivery_profit(N);
    vector<vector<int>> delivery_modules(N);
    for (int i = 0; i < N; ++i) {
        int K;
        cin >> delivery_profit[i] >> K;
        delivery_modules[i].resize(K);
        for (int j = 0; j < K; ++j) {
            cin >> delivery_modules[i][j];
            --delivery_modules[i][j];
        }
    }
    vector<long long> module_cost(M);
    for (int i = 0; i < M; ++i) {
        cin >> module_cost[i];
    }
    vector<Discount> discounts(P);
    for (int i = 0; i < P; ++i) {
        cin >> discounts[i].a >> discounts[i].b >> discounts[i].price;
        --discounts[i].a; 
        --discounts[i].b;
    }
    int delivery_start = 0;
    int module_start = N;
    int bonus_start = N + M;
    int bonus_count = 0;
    for (int i = 0; i < P; ++i) {
        long long bonus = module_cost[discounts[i].a] + module_cost[discounts[i].b] - discounts[i].price;
        if (bonus > 0) {
            ++bonus_count;
        }
    }
    int S = bonus_start + bonus_count;
    int T = S + 1;
    vector<vector<Edge>> graph(T + 1);
    long long total_positive = 0;
    for (int i = 0; i < N; ++i) {
        int node = delivery_start + i;
        total_positive += delivery_profit[i];
        graph[S].push_back({node, delivery_profit[i], (int)graph[node].size()});
        graph[node].push_back({S, 0, (int)graph[S].size()-1});
        for (int mod : delivery_modules[i]) {
            int mnode = module_start + mod;
            graph[node].push_back({mnode, LLONG_MAX, (int)graph[mnode].size()});
            graph[mnode].push_back({node, 0, (int)graph[node].size()-1});
        }
    }
    for (int i = 0; i < M; ++i) {
        int mnode = module_start + i;
        graph[mnode].push_back({T, module_cost[i], (int)graph[T].size()});
        graph[T].push_back({mnode, 0, (int)graph[mnode].size()-1});
    }
    int bonus_id = 0;
    for (int i = 0; i < P; ++i) {
        long long bonus = module_cost[discounts[i].a] + module_cost[discounts[i].b] - discounts[i].price;
        if (bonus <= 0) {
            continue;
        }
        int node = bonus_start + bonus_id++;
        total_positive += bonus;
        graph[S].push_back({node, bonus, (int)graph[node].size()});
        graph[node].push_back({S, 0, (int)graph[S].size()-1});
        int a_node = module_start + discounts[i].a;
        int b_node = module_start + discounts[i].b;
        graph[node].push_back({a_node, LLONG_MAX, (int)graph[a_node].size()});
        graph[a_node].push_back({node, 0, (int)graph[node].size()-1});
        graph[node].push_back({b_node, LLONG_MAX, (int)graph[b_node].size()});
        graph[b_node].push_back({node, 0, (int)graph[node].size()-1});
    }
    long long flow = 0;
    while (true) {
        vector<int> parent_v(T + 1, -1);
        vector<int> parent_e(T + 1, -1);
        queue<int> q;
        q.push(S);
        parent_v[S] = S;
        bool found = false;
        while (!q.empty() && !found) {
            int v = q.front(); q.pop();
            for (int i = 0; i < graph[v].size(); ++i) {
                Edge &e = graph[v][i];
                if (e.cap > 0 && parent_v[e.to] == -1) {
                    parent_v[e.to] = v;
                    parent_e[e.to] = i;
                    if (e.to == T) { 
                        found = true; 
                        break; 
                    }
                    q.push(e.to);
                }
            }
        }
        if (!found) {
            break;
        }
        long long pushed = LLONG_MAX;
        for (int v = T; v != S; v = parent_v[v]) {
            int u = parent_v[v], id = parent_e[v];
            pushed = min(pushed, graph[u][id].cap);
        }
        for (int v = T; v != S; v = parent_v[v]) {
            int u = parent_v[v], id = parent_e[v];
            graph[u][id].cap -= pushed;
            graph[v][graph[u][id].reversed].cap += pushed;
        }
        flow += pushed;
    }
    cout << total_positive - flow;
    return 0;
}