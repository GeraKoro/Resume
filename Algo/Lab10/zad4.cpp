#include <iostream>
#include <set>
#include <string>
#include <vector>

using namespace std;

int min(int a, int b) {
    return (a < b) ? a : b;
}

vector<int> BuildZ(const string& s) {
    int n = static_cast<int>(s.size());
    vector<int> z(n, 0);
    int left_bound = 0;
    int right_bound = 0;
    for (int index = 1; index < n; ++index) {
        if (index <= right_bound) {
            z[index] = min(right_bound - index + 1, z[index - left_bound]);
        }
        while (index + z[index] < n && s[z[index]] == s[index + z[index]]) {
            ++z[index];
        }
        if (index + z[index] - 1 > right_bound) {
            left_bound = index;
            right_bound = index + z[index] - 1;
        }
    }
    return z;
}

vector<int> FindRes(const string& str, const vector<int>& z_val) {
    int n = static_cast<int>(str.size());
    vector<int> answer(n + 1, 0);
    vector<vector<int>> positions(n + 1);
    set<int> start;
    for (int pos = 1; pos < n; ++pos) {
        if (z_val[pos] > 0) {
            start.insert(pos);
            positions[z_val[pos]].push_back(pos);
        }
    }
    int left = n;
    for (int block_len = 1; block_len <= n && left > 0; ++block_len) {
        if (block_len > 1) {
            for (int pos : positions[block_len - 1]) {
                start.erase(pos);
            }
        }
        if (answer[block_len] == 0) {
            answer[block_len] = block_len;
            --left;
        }

        if (start.empty()) {
            break;
        }
        int last_start = 0;
        for (int s : start) {
            if (s - last_start > block_len) {
                break;
            }
            last_start = s;
            int pref_len = s + block_len;
            if (answer[pref_len] == 0) {
                answer[pref_len] = block_len;
                --left;
                if (left == 0) {
                    break;
                }
            }
        }
    }
    vector<int> res(n);
    for (int i = 1; i <= n; ++i) {
        if (answer[i] == 0) {
            answer[i] = i;
        }
        res[i - 1] = answer[i];
    }
    return res;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    string str;
    cin >> str;
    vector<int> z_val = BuildZ(str);
    vector<int> res = FindRes(str, z_val);
    for (int i = 0; i < static_cast<int>(res.size()); ++i) {
        if (i > 0) {
            cout << ' ';
        }
        cout << res[i];
    }
    cout << '\n';
    return 0;
}
