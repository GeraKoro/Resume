#include <algorithm>
#include <iostream>
#include <unordered_set>
#include <vector>

using namespace std;

using u64 = unsigned long long;
using ll = long long;

const ll kMod1 = 1000000007LL;
const ll kMod2 = 1000000003LL;

int min(int a, int b) {
    return (a < b) ? a : b;
}

int max(int a, int b) {
    return (a > b) ? a : b;
}

u64 Random(u64& number) {
    number = number * 234235234121LL + 4523423423LL;
    return number;
}

struct PolyHash {
    vector<ll> pref;
    vector<ll> degree;
    ll mod = 0;
    PolyHash(const vector<int>& a, ll base, ll modul) : mod(modul) {
        int n = static_cast<int>(a.size());
        pref.resize(n + 1, 0);
        degree.resize(n + 1, 1);
        for (int i = 0; i < n; ++i) {
            pref[i + 1] = ((pref[i] * base + a[i] + 1) % mod + mod) % mod;
            degree[i + 1] = (degree[i] * base) % mod;
        }
    }
    ll Get(int l, int r) const {
        ll result = pref[r] - (pref[l] * degree[r - l]) % mod;
        result %= mod;
        if (result < 0) {
            result += mod;
        }
        return result;
    }
};

u64 MakeKey(ll difference_first, ll difference_second) {
    ll normal_first = difference_first % kMod1;
    if (normal_first < 0) {
        normal_first += kMod1;
    }
    ll normal_second = difference_second % kMod2;
    if (normal_second < 0) {
        normal_second += kMod2;
    }
    return (static_cast<u64>(static_cast<unsigned int>(normal_first)) << 32) | static_cast<unsigned int>(normal_second);
}

bool CanThisLength(int len, const vector<int>& A, const vector<int>& B, const PolyHash& A_hash_1, const PolyHash& A_reversed_1,
    const PolyHash& A_hash_2, const PolyHash& A_reversed_2, const PolyHash& B_hash_1, const PolyHash& B_reversed_1,
    const PolyHash& B_hash_2, const PolyHash& B_reversed_2)
{
    if (len == 0) {
        return false;
    }
    unordered_set<u64> seen_code;
    int n = static_cast<int>(A.size());
    int m = static_cast<int>(B.size());
    const int windows_count_A = n - len + 1;
    seen_code.reserve(static_cast<size_t>(windows_count_A) * 2 + 7);

    for (int i = 0; i + len <= n; ++i) {
        ll forward_1 = A_hash_1.Get(i, i + len);
        ll reversed_1 = A_reversed_1.Get(n - i - len, n - i);
        ll forward_2 = A_hash_2.Get(i, i + len);
        ll reversed_2 = A_reversed_2.Get(n - i - len, n - i);
        ll code_a_1 = reversed_1 - forward_1;
        ll code_a_2 = reversed_2 - forward_2;
        seen_code.insert(MakeKey(code_a_1, code_a_2));
    }
    for (int i = 0; i + len <= m; ++i) {
        ll forward_1 = B_hash_1.Get(i, i + len);
        ll reversed_1 = B_reversed_1.Get(m - i - len, m - i);
        ll forward_2 = B_hash_2.Get(i, i + len);
        ll reversed_2 = B_reversed_2.Get(m - i - len, m - i);
        ll code_b_1 = forward_1 - reversed_1;
        ll code_b_2 = forward_2 - reversed_2;
        if (seen_code.count(MakeKey(code_b_1, code_b_2))) {
            return true;
        }
    }
    return false;
}

int FindRes(int parity, int limit, const PolyHash& A_hash_1, const PolyHash& A_reversed_1, const PolyHash& A_hash_2,
    const PolyHash& A_reversed_2, const PolyHash& B_hash_1, const PolyHash& B_reversed_1, const PolyHash& B_hash_2,
    const PolyHash& B_reversed_2, const vector<int>& A, const vector<int>& B)
{
    int left = 0;
    int right = (limit - parity) / 2;
    while (left < right) {
        int mid = (left + right + 1) / 2;
        int len = mid * 2 + parity;
        if (CanThisLength(len, A, B, A_hash_1, A_reversed_1, A_hash_2, A_reversed_2, B_hash_1, B_reversed_1, B_hash_2, B_reversed_2)) {
            left = mid;
        }
        else {
            right = mid - 1;
        }
    }
    int candidate = left * 2 + parity;
    while (candidate > 0 && candidate <= limit && !CanThisLength(candidate, A, B, A_hash_1, A_reversed_1, A_hash_2, A_reversed_2, B_hash_1, B_reversed_1, B_hash_2, B_reversed_2)) {
        candidate -= 2;
    }
    if (candidate < 0) {
        return 0;
    }
    return candidate;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int n = 0;
    int m = 0;
    cin >> n >> m;
    vector<int> A(n);
    vector<int> B(m);
    for (int i = 0; i < n; ++i) {
        cin >> A[i];
    }
    for (int i = 0; i < m; ++i) {
        cin >> B[i];
    }
    vector<int> A_rev = A;
    vector<int> B_rev = B;
    std::reverse(A_rev.begin(), A_rev.end());
    std::reverse(B_rev.begin(), B_rev.end());
    u64 random_state = 123712367123ULL;
    ll base_value = static_cast<ll>(Random(random_state) % 900000000ULL) + 100000000LL;

    PolyHash A_hash_1(A, base_value, kMod1);
    PolyHash A_reversed_1(A_rev, base_value, kMod1);
    PolyHash A_hash_2(A, base_value, kMod2);
    PolyHash A_reversed_2(A_rev, base_value, kMod2);

    PolyHash B_hash_1(B, base_value, kMod1);
    PolyHash B_reversed_1(B_rev, base_value, kMod1);
    PolyHash B_hash_2(B, base_value, kMod2);
    PolyHash B_reversed_2(B_rev, base_value, kMod2);

    int limit = min(n, m);
    int best_even = FindRes(0, limit, A_hash_1, A_reversed_1, A_hash_2, A_reversed_2, B_hash_1, B_reversed_1, B_hash_2, B_reversed_2, A, B);
    int best_odd = FindRes(1, limit, A_hash_1, A_reversed_1, A_hash_2, A_reversed_2, B_hash_1, B_reversed_1, B_hash_2, B_reversed_2, A, B);
    cout << max(best_even, best_odd);
    return 0;
}
