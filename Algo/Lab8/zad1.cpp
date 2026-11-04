#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <limits>

using namespace std;

struct Vertex {
    int x, y;
};

struct Edge {
    int to;
    int id;
};

double Angle(Vertex a, Vertex b, Vertex c) {
    double v1x = b.x - a.x;
    double v1y = b.y - a.y;
    double v2x = c.x - b.x;
    double v2y = c.y - b.y;
    
    double len1 = sqrt(v1x * v1x + v1y * v1y);
    double len2 = sqrt(v2x * v2x + v2y * v2y);
    
    if (len1 < 1e-9 || len2 < 1e-9) return 0;
    
    double dot = v1x * v2x + v1y * v2y;
    double cos = dot / (len1 * len2);
    if (cos > 1) cos = 1;
    if (cos < -1) cos = -1;
    
    double skal = v1x * v2y - v1y * v2x;
    double angle = acos(cos);
    
    if (skal < 0) {
        return -angle;
    } 
    else if (skal > 0) {
        return angle;
    } 
    else {
        return 0;
    }
}

int main() {
    int N, M;
    cin >> N >> M;
    vector<Vertex> vertices(N);
    for (int i = 0; i < N; i++) {
        cin >> vertices[i].x >> vertices[i].y;
    }
    vector<vector<Edge>> graph(N);
    for (int i = 0; i < M; i++) {
        int u, v;
        cin >> u >> v;
        graph[u].push_back({v, i});
        graph[v].push_back({u, i});
    }
    double res = 0;
    for (int v = 0; v < N; v++) {
        int deg = graph[v].size();
        if (deg == 2) {
            int a = graph[v][0].to;
            int c = graph[v][1].to;
            double angle = Angle(vertices[a], vertices[v], vertices[c]);
            res += abs(angle);
        } 
        else if (deg == 4) {
            vector<int> neighbors(4);
            for (int i = 0; i < 4; i++) {
                neighbors[i] = graph[v][i].to;
            }
            double weights[16];
            for (int i = 0; i < 4; i++) {
                for (int j = 0; j < 4; j++) {
                    if (i == j) {
                        weights[i * 4 + j] = 0;
                    } 
                    else {
                        double angle = Angle(vertices[neighbors[i]], vertices[v], vertices[neighbors[j]]);
                        weights[i * 4 + j] = angle;
                    }
                }
            }
            
            double sum1 = abs(weights[0 * 4 + 1]) + abs(weights[2 * 4 + 3]);
            double sum2 = abs(weights[0 * 4 + 2]) + abs(weights[1 * 4 + 3]);
            double sum3 = abs(weights[0 * 4 + 3]) + abs(weights[1 * 4 + 2]);
            
            double best = sum1;
            if (sum2 < best) best = sum2;
            if (sum3 < best) best = sum3;
            
            res += best;
        }
    }
    
    cout << fixed << setprecision(6) << res << endl;
    
    return 0;
}