#include <iostream>
#include <vector>
#include <list>
using namespace std;


struct Number {
    int number;
    int count;
};


class Hash {
private:
    int M;
    vector<list<Number>> table_;

    int hash(int x) {
        int h = x;
        h = (h % M + M) % M;
        return h;
    }

public:

    Hash(int N) : M(N * 2 + 1) {
        table_.assign(M, {});
    }

    void Insert(int x) {
        int h = hash(x);
        for (auto& v : table_[h]) {
            if (v.number == x) {
                ++v.count;
                return;
            }
        }
        table_[h].push_back({x, 1});
    }

    bool Contains(int x) {
        int h = hash(x);
        for (auto& v : table_[h]) {
            if (v.number == x && v.count > 0) {
                return true;
            }
        }
        return false;
    }

    void Delete(int x) {
        int h = hash(x);
        for (auto& v : table_[h]) {
            if (v.number == x) {
                --v.count;
                return;
            }
        }
    }
};




int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    
    int N;
    std::cin >> N;
    
    vector<int> a_arr(N);
    Hash b_arr(N);
    for (int i = 0; i < N; i++) {
        int x;
        std::cin >> x;
        a_arr[i] = x;
    }

    for (int i = 0; i < N; i++) {
        int x;
        std::cin >> x;
        b_arr.Insert(x);
    }

    bool res = true;

    for (int i = 0; i < N; i++) {
        int x = a_arr[i];

        if (!b_arr.Contains(x)) {
            res = false;
            break;
        }
        else {
            b_arr.Delete(x);
        }
    }

    std::cout << (res ? "YES" : "NO");
    return 0;
}