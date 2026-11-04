#include <iostream>
#include <vector>
#include <climits>
#include <queue>
using namespace std;

struct Edge {
    int to;
    int reversed;
    int cap;
};

void AddEdge(int u, int v, int cap, vector<vector<Edge>>& graph) {
    Edge a = {v, (int)graph[v].size(), cap};
    Edge b = {u, (int)graph[u].size(), 0};
    graph[u].push_back(a);
    graph[v].push_back(b);
}

int PixelIndex(int x, int y, int M) {
    return x * M + y;
}

int main() {
    int N, M, cost_white, cost_black, cost_gray;
    cin >> N >> M >> cost_white >> cost_black >> cost_gray;
    vector<string> matrix(N);
    for (int i = 0; i < N; ++i) {
        cin >> matrix[i];
    }
    int total_pixels = N * M;
    int S = total_pixels;
    int T = total_pixels + 1;
    int vertex_count = total_pixels + 2;
    vector<vector<Edge>> graph(vertex_count);
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < M; ++j) {
            int index = PixelIndex(i, j, M);
            if (matrix[i][j] == 'W') {
                AddEdge(S, index, 0, graph);
                AddEdge(index, T, cost_black, graph);
            } 
            else if (matrix[i][j] == 'B') {
                AddEdge(S, index, cost_white, graph);
                AddEdge(index, T, 0, graph);
            }
        }
    }
    int dx[2] = {0, 1};
    int dy[2] = {1, 0};
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < M; ++j) {
            int index = PixelIndex(i, j, M);
            for (int dir = 0; dir < 2; ++dir) {
                int ni = i + dx[dir];
                int nj = j + dy[dir];
                if (ni < N && nj < M) {
                    int nindex = PixelIndex(ni, nj, M);
                    AddEdge(index, nindex, cost_gray, graph);
                    AddEdge(nindex, index, cost_gray, graph);
                }
            }
        }
    }
    int flow = 0;
    while (true) {
        vector<int> parent(vertex_count, -1);
        vector<int> parent_edge(vertex_count, -1);
        queue<int> q;
        q.push(S);
        parent[S] = -2;
        vector<int> min_cap(vertex_count, 0);
        min_cap[S] = INT_MAX;
        while (!q.empty()) {
            int v = q.front();
            q.pop();
            for (int i = 0; i < (int)graph[v].size(); ++i) {
                Edge &edge = graph[v][i];
                int to = edge.to;
                if (parent[to] == -1 && edge.cap > 0) {
                    parent[to] = v;
                    parent_edge[to] = i;
                    min_cap[to] = min(min_cap[v], edge.cap);
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
            int ind = parent_edge[cur];
            graph[prev][ind].cap -= pushed;
            int reversed = graph[prev][ind].reversed;
            graph[cur][reversed].cap += pushed;
            cur = prev;
        }
    }
    cout << flow;
    return 0;
}