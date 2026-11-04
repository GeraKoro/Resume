#include <iostream>
#include <vector>
#include <list>

using namespace std;

class Hash {
private:
    vector<vector<pair<int, int>>> table_;
    int capacity_;

    int hash(int x) {
        if (x < 0) {
            x = -x;
        }
        return (x * 131 + 5) % capacity_;
    }

public:
    Hash(int capacity) : capacity_(capacity) {
        table_.resize(capacity);
    }

    int GetValueCount(int key) {
        int h = hash(key);
        for (auto& pair : table_[h]) {
            if (pair.first == key) {
                return pair.second;
            }
        }
        return 0;
    }

    void IncCount(int key) {
        int h = hash(key);
        for (auto& pair : table_[h]) {
            if (pair.first == key) {
                pair.second++;
                return;
            }
        }
        table_[h].push_back({key, 1});
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int N;
    cin >> N;
    
    int capacity = 20003;
    Hash table(capacity);

    int res = 0;

    for (int i = 0; i < N; i++) {
        int hight;
        cin >> hight;

        int t = hight - i;
        int count = table.GetValueCount(t);

        res += count;

        table.IncCount(t);
    }

    cout << res;
    return 0;

}