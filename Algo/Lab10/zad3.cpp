#include <iostream>
#include <string>
#include <vector>

using namespace std;

int main() {
    string s;
    cin >> s;
    int n = s.size();
    vector<int> pref(n, 0);
    for (int i = 1; i < n; ++i) {
        int j = pref[i - 1];
        while (j > 0 && s[i] != s[j]) {
            j = pref[j - 1];
        }
        if (s[i] == s[j]) {
            ++j;
        }
        pref[i] = j;
    }
    vector<int> number(n + 1, 0);
    int max_number = -1;
    int res_len = 0;
    for (int len = 1; len <= n; ++len) {
        number[len] = 1 + number[pref[len - 1]];
        if (number[len] > max_number) {
            max_number = number[len];
            res_len = len;
        }
    }
    cout << s.substr(0, res_len);
    return 0;
}