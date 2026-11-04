#include <iostream>
#include <vector>
#include <queue>
using namespace std;

bool dfs(int u, vector<bool>& visited, vector<vector<int>>& graph, vector<int>& part_u, vector<int>& part_v) {
    if (visited[u]) {
        return false;
    }
    visited[u] = true;
    for (int v : graph[u]) {
        if (part_v[v] == -1 || dfs(part_v[v], visited, graph, part_u, part_v)) {
            part_u[u] = v;
            part_v[v] = u;
            return true;
        }
    }
    return false;
}

int main() {
    int n1, n2, m;
    cin >> n1 >> n2 >> m;
    vector<vector<int>> graph_u(n1), graph_v(n2);
    for (int i = 0; i < m; ++i) {
        int u, v;
        cin >> u >> v;
        u--; v--;
        graph_u[u].push_back(v);
        graph_v[v].push_back(u);
    }
    vector<int> part_u(n1, -1);
    vector<int> part_v(n2, -1);
    vector<bool> visited(n1);
    for (int i = 0; i < n1; ++i) {
        visited.assign(n1, false);
        dfs(i, visited, graph_u, part_u, part_v);
    }
    vector<bool> reachable_u(n1, false);
    vector<bool> reachable_v(n2, false);
    queue<int> q;
    for (int i = 0; i < n1; ++i) {
        if (part_u[i] == -1) {
            reachable_u[i] = true;
            q.push(i);
        }
    }
    while (!q.empty()) {
        int u = q.front();
        q.pop();
        for (int v : graph_u[u]) {
            if (part_u[u] != v && !reachable_v[v]) {
                reachable_v[v] = true;
                if (part_v[v] != -1 && !reachable_u[part_v[v]]) {
                    reachable_u[part_v[v]] = true;
                    q.push(part_v[v]);
                }
            }
        }
    }
    vector<bool> reachable_u2(n1, false);
    vector<bool> reachable_v2(n2, false);
    queue<int> q2;
    for (int i = 0; i < n2; ++i) {
        if (part_v[i] == -1) {
            reachable_v2[i] = true;
            q2.push(i);
        }
    }
    while (!q2.empty()) {
        int v = q2.front();
        q2.pop();
        for (int u : graph_v[v]) {
            if (part_v[v] != u && !reachable_u2[u]) {
                reachable_u2[u] = true;
                if (part_u[u] != -1 && !reachable_v2[part_u[u]]) {
                    reachable_v2[part_u[u]] = true;
                    q2.push(part_u[u]);
                }
            }
        }
    }
    for (int i = 0; i < n1; ++i) {
        cout << (reachable_u[i] ? 'P' : 'N');
    }
    cout << endl;
    for (int i = 0; i < n2; ++i) {
        cout << (reachable_v2[i] ? 'P' : 'N');
    }
    return 0;
}