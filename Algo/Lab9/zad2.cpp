#include <iostream>
#include <vector>
#include <list>

using namespace std;
using u64 = unsigned long long;


int min(int a, int b) {
    return (a < b) ? a : b;
}


u64 Random(u64& number) {
    number = number * 234235234121LL + 4523423423LL;
    return number;
}


struct Window {
    vector<int> count;
    u64 hash = 0;
    vector<vector<u64>>& hash_code;

    Window(int max_num, vector<vector<u64>>& code) : count(max_num + 1, 0), hash_code(code) {}

    void Add(int x) {
        int old = count[x];
        hash ^= hash_code[x][old];
        count[x]++;
        hash ^= hash_code[x][count[x]];
    }

    void Remove(int x) {
        int old = count[x];
        hash ^= hash_code[x][old];
        count[x]--;
        hash ^= hash_code[x][count[x]];
    }

};


class Hash {
private:
    vector<list<u64>> table_;
    int capacity_;
    int hash(u64 x) {
        return x % capacity_;
    }

public:
    Hash(int capacity) : capacity_(capacity) { table_.resize(capacity); }

    void Insert(u64 h) {
        int index = hash(h);
        for (auto& value : table_[index]) {
            if (value == h) {
                return;
            }
        }
        table_[index].push_back(h);
    }

    bool Contains(u64 h) {
        int index = hash(h);
        for (auto& value : table_[index]) {
            if (value == h) {
                return true;
            }
        }
        return false;
    }
};

bool CanThisLength(int len, const vector<int>& A, const vector<int>& B
    , vector<vector<u64>>& hash_code, int max_num)
{
    if (len == 0) {
        return true;
    }

    int windows_count_A = A.size() - len + 1;
    Hash seen_windows(windows_count_A * 2);
    Window window_A(max_num, hash_code);

    for (int i = 0; i < len; i++) {
        window_A.Add(A[i]);
    }

    seen_windows.Insert(window_A.hash);

    for (int left = 0, right = len; right < A.size(); left++, right++) {
        window_A.Remove(A[left]);
        window_A.Add(A[right]);
        seen_windows.Insert(window_A.hash);
    }


    Window window_B(max_num, hash_code);

    for (int i = 0; i < len; i++) {
        window_B.Add(B[i]);
    }

    if (seen_windows.Contains(window_B.hash)) {
        return true;
    }

    for (int left = 0, right = len; right < B.size(); left++, right++) {
        window_B.Remove(B[left]);
        window_B.Add(B[right]);
        if (seen_windows.Contains(window_B.hash)) {
            return true;
        }
    }

    return false;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    
    int N, M; 
    cin >> N;
    vector<int> A(N);

    int max_num = 0;

    for (int i = 0; i < N; i++) {
        cin >> A[i];
        if (A[i] > max_num) {
            max_num = A[i];
        }
    }

    cin >> M;
    vector<int> B(M);

    for (int i = 0; i < M; i++) {
        cin >> B[i];
        if (B[i] > max_num) {
            max_num = B[i];
        }
    }

    int size = min(N, M);

    vector<vector<u64>> hash_code(max_num + 1, vector<u64>(size + 2));
    u64 start_val = 234235214LL;

    for (int i = 1; i <= max_num; i++) {
        for (int j = 0; j <= size + 1; j++) {
            hash_code[i][j] = Random(start_val);
        }
    }

    for (int len = size; len >= 1; len--) {
        if (CanThisLength(len, A, B, hash_code, max_num)) {
            cout << len;
            return 0;
        }
    }
    cout << 0;
    return 0;
}