#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

struct BigUInt {
    static const int kBase = 1000000000;
    vector<int> dig;

    BigUInt() : dig(1, 0) {}

    static BigUInt One() {
        BigUInt result;
        result.dig.assign(1, 1);
        return result;
    }

    void MulSmall(int k) {
        uint64_t carry = 0;
        for (size_t i = 0; i < dig.size(); ++i) {
            carry += static_cast<uint64_t>(dig[i]) * static_cast<uint64_t>(k);
            dig[i] = static_cast<int>(carry % kBase);
            carry /= kBase;
        }
        while (carry > 0) {
            dig.push_back(static_cast<int>(carry % kBase));
            carry /= kBase;
        }
        Trim();
    }

    void Add(const BigUInt& other) {
        uint64_t carry = 0;
        size_t os = other.dig.size();
        if (dig.size() < os) {
            dig.resize(os, 0);
        }
        size_t i = 0;
        for (; i < os; ++i) {
            carry += static_cast<uint64_t>(dig[i]) + static_cast<uint64_t>(other.dig[i]);
            dig[i] = static_cast<int>(carry % kBase);
            carry /= kBase;
        }
        while (carry > 0 && i < dig.size()) {
            carry += static_cast<uint64_t>(dig[i]);
            dig[i] = static_cast<int>(carry % kBase);
            carry /= kBase;
            ++i;
        }
        while (carry > 0) {
            dig.push_back(static_cast<int>(carry % kBase));
            carry /= kBase;
        }
        Trim();
    }

    void Trim() {
        while (dig.size() > 1 && dig.back() == 0) {
            dig.pop_back();
        }
    }

    void Print() const {
        printf("%d", dig.back());
        for (int i = static_cast<int>(dig.size()) - 2; i >= 0; --i) {
            printf("%09d", dig[i]);
        }
    }
};

vector<int> PrefixFunction(const string& s) {
    int n = static_cast<int>(s.size());
    vector<int> pi(n, 0);
    for (int i = 1; i < n; ++i) {
        int j = pi[i - 1];
        while (j > 0 && s[i] != s[j]) {
            j = pi[j - 1];
        }
        if (s[i] == s[j]) {
            ++j;
        }
        pi[i] = j;
    }
    return pi;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n_alphabet = 0;
    cin >> n_alphabet;
    string s;
    cin >> s;

    int m = static_cast<int>(s.size());
    vector<int> pi = PrefixFunction(s);

    vector<char> is_border(m + 1, 0);
    int len = m;
    while (len > 0) {
        is_border[len] = 1;
        len = pi[len - 1];
    }

    BigUInt sum;
    BigUInt cur = BigUInt::One();

    for (int i = 1; i <= m; ++i) {
        cur.MulSmall(n_alphabet);
        if (is_border[i]) {
            sum.Add(cur);
        }
    }

    sum.Print();
    printf("\n");

    return 0;
}
