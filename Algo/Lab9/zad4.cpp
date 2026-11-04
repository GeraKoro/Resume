#include <iostream>
#include <optional>
#include <vector>
#include <string>

using namespace std;

int Max(int a, int b) {
    return (a > b) ? a : b;
}




struct Student {
    int isu;
    int points;
};

class Hash {
private:
    int M;
    vector<vector<Student>> table_;

    int hash(int h) {
        return (h % M + M) % M;
    }

public:
    Hash(int N) : M(N * 2 + 1) {
        table_.resize(M);
    }

    void Insert(int isu, int points) {
        int h = hash(isu);
        for (auto& s : table_[h]) {
            if (s.isu == isu) {
                s.points = points; 
                return;
            }
        }
        table_[h].push_back({isu, points});
    }

    int Delete(int isu) {
        int h = hash(isu);
        for (auto it = table_[h].begin(); it != table_[h].end(); ++it) {
            if (it->isu == isu) {
                int p = it->points;
                table_[h].erase(it);
                return p;
            }
        }
        return -1;
    }

    int GetPoints(int isu) {
        int h = hash(isu);
        for (auto& s : table_[h]) {
            if (s.isu == isu) {
                return s.points;
            }
        }
        return -1;
    }

    friend int main();
};


struct Group {
    Hash students;
    int max_points = 0;
    int sum = 0;
    int students_count = 0;
    Group(int M) : students(M) {}
};


int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int M, Q;
    cin >> M >> Q;
    vector<Group> groups;
    int hash_size = 131;
    for (int i = 0; i <= M; i++) {
        groups.emplace_back(hash_size);
    }
    for (int i = 0; i < Q; ++i) {
        char command;
        cin >> command;
        
        if (command == '+') {
            int group, isu, points;
            cin >> group >> isu >> points;
            int old_points = groups[group].students.GetPoints(isu);
            if (old_points != -1) {
                groups[group].sum -= old_points;
                groups[group].sum += points;
            }
            else {
                groups[group].sum += points;
                groups[group].students_count++;
            }
            groups[group].students.Insert(isu, points);
            groups[group].max_points = Max(groups[group].max_points, points);
        }
        else if (command == '-') {
            int group, isu;
            cin >> group >> isu;
            int points = groups[group].students.Delete(isu);
            if (points != -1) {
                groups[group].sum -= points;
                groups[group].students_count--;
                if (points == groups[group].max_points) {
                    groups[group].max_points = 0;
                    for (auto& lst : groups[group].students.table_) {
                        for (auto& s : lst) {
                            groups[group].max_points = Max(groups[group].max_points, s.points);
                        }
                    }
                }
            }
        }
        else if (command == 'm') {
            int group;
            cin >> group;
            cout << groups[group].max_points << '\n';
        }
        else if (command == 'a') {
            int group;
            cin >> group;
            if (groups[group].students_count == 0) {
                cout << 0 << '\n';
            }
            else {
                cout << groups[group].sum / groups[group].students_count << '\n';
            }
        }
    }
    return 0;
}