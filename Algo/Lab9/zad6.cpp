#include <iostream>
#include <vector>
#include <string>

using namespace std;

class Hash {
private:
    struct Info {
        string key;
        int mask_student_number;
        bool used;
    };

    vector<Info> T1, T2;
    int capacity_;

    long long hash1(string key) {
        long long h = 0;
        for (char c : key) {
            h = h * 131 + c;
        }
        if (h < 0) {
            h = -h;
        }
        return h % capacity_;
    }

    long long hash2(string key) {
        long long h = 0;
        for (char c : key) {
            h = h * 137 + c;
        }
        if (h < 0) {
            h = -h;
        }
        return h % capacity_;
    }

public:
    Hash(int capacity) : capacity_(capacity) {
        T1.resize(capacity, {"", 0, false});
        T2.resize(capacity, {"", 0, false});
    }

    void Insert(string key, int mask_student_number) {
        for (int attempt = 0; attempt < 500; attempt++) {
            long long h1 = hash1(key);
            if (!T1[h1].used) {
                T1[h1] = {key, mask_student_number, true};
                return;
            }

            if (T1[h1].key == key) {
                T1[h1].mask_student_number |= mask_student_number;
                return;
            }

            swap(key, T1[h1].key);
            swap(mask_student_number, T1[h1].mask_student_number);

            
            long long h2 = hash2(key);
            if (!T2[h2].used) {
                T2[h2] = {key, mask_student_number, true};
                return;
            }

            if (T2[h2].key == key) {
                T2[h2].mask_student_number |= mask_student_number;
                return;
            }

            swap(key, T2[h2].key);
            swap(mask_student_number, T2[h2].mask_student_number);
        }
    }

    const vector<Info>& GetT1() const { return T1; }
    const vector<Info>& GetT2() const { return T2; }
    friend void Solve(const vector<Hash::Info>& table, vector<int>& points);
};


void Solve(const vector<Hash::Info>& table, vector<int>& points) {
    for (auto& info : table) {
        if (info.used) {
            int mask = info.mask_student_number;
            int count = 0;

            if (mask & 1) {
                count++;
            }
            if (mask & 2) {
                count++;
            }
            if (mask & 4) {
                count++;
            }

            if (count == 1) {
                if (mask & 1) {
                    points[0] += 3;
                }
                if (mask & 2) {
                    points[1] += 3;
                }
                if (mask & 4) {
                    points[2] += 3;
                }
            }
            else if (count == 2) {
                if (mask & 1) {
                    points[0] += 1;
                }
                if (mask & 2) {
                    points[1] += 1;
                }
                if (mask & 4) {
                    points[2] += 1;
                }
            }
        }
    }
}


int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n;
    cin >> n;

    vector<vector<string>> students(3, vector<string>(n));
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < n; j++) {
            cin >> students[i][j];
        }
    }

    int capacity = 1000003;
    Hash table(capacity);

    for (int student = 0; student < 3; student++) {
        int mask_student = 1 << student;
        for (auto& name_file : students[student]) {
            table.Insert(name_file, mask_student);
        }
    }

    vector<int> points = {0, 0, 0};

    Solve(table.GetT1(), points);
    Solve(table.GetT2(), points);

    cout << points[0] << " " << points[1] << " " << points[2];
    return 0;
}