#include <iostream>
#include <vector>
#include <climits>
#include <queue>
using namespace std;

void AddEdge(int u, int v, int cap, vector<vector<int>>& capacity, vector<vector<int>>& graph) {
    capacity[u][v] += cap;
    graph[u].push_back(v);
    graph[v].push_back(u);
}

int main() {
    int n;
    cin >> n;
    vector<int> wins(n), left(n);
    for (int i = 0; i < n; ++i) {
        cin >> wins[i];
    }
    for (int i = 0; i < n; ++i) {
        cin >> left[i];
    }
    vector<vector<int>> games(n, vector<int>(n));
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            cin >> games[i][j];
        }
    }
    int max_wins = wins[0] + left[0];
    for (int i = 1; i < n; ++i) {
        if (wins[i] > max_wins) {
            cout << "NO";
            return 0;
        }
    }
    int game_vertex = 0;
    vector<vector<int>> game_number(n, vector<int>(n, -1));
    for (int i = 1; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            game_number[i][j] = game_vertex++;
        }
    }
    int S = 0;
    int games_offset = 1;
    int teams_offset = games_offset + game_vertex;
    int T = teams_offset + n;
    int N = T + 1;
    vector<vector<int>> capacity(N, vector<int>(N, 0));
    vector<vector<int>> graph(N);
    int total_games = 0;
    for (int i = 1; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            int cnt = games[i][j];
            if (cnt == 0) {
                continue;
            }
            int node = games_offset + game_number[i][j];
            AddEdge(S, node, cnt, capacity, graph);
            AddEdge(node, teams_offset + i, INT_MAX, capacity, graph);
            AddEdge(node, teams_offset + j, INT_MAX, capacity, graph);
            total_games += cnt;
        }
    }

    for (int i = 1; i < n; ++i) {
        int cap = max_wins - wins[i];
        if (cap < 0) {
            cout << "NO";
            return 0;
        }
        AddEdge(teams_offset + i, T, cap, capacity, graph);
    }
    int flow = 0;
    while (true) {
        vector<int> parent(N, -1);
        queue<int> q;
        q.push(S);
        parent[S] = -2;
        vector<int> min_cap(N, 0);
        min_cap[S] = INT_MAX;
        while (!q.empty()) {
            int v = q.front();
            q.pop();
            for (int to : graph[v]) {
                if (parent[to] == -1 && capacity[v][to] > 0) {
                    parent[to] = v;
                    min_cap[to] = min(min_cap[v], capacity[v][to]);
                    if (to == T) {
                        break;
                    }
                    q.push(to);
                }
            }
        }
        if (parent[T] == -1) {
            break;
        }
        int pushed = min_cap[T];
        flow += pushed;
        int cur = T;
        while (cur != S) {
            int prev = parent[cur];
            capacity[prev][cur] -= pushed;
            capacity[cur][prev] += pushed;
            cur = prev;
        }
    }

    if (flow == total_games) {
        cout << "YES";
    } 
    else {
        cout << "NO";
    }

    return 0;
}