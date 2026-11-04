#include <iostream>
#include <vector>
using namespace std;


struct Point {
    int x, y;
    bool used = false;
};


class Hash {
private:
    int M;
    vector<Point> table_;

    int hash(int x, int y) {
        int h = x * 131 + y;
        h = (h % M + M) % M;
        return h;
    }

public:

    Hash(int N) : M(N * 2 + 1) {
        table_.resize(M);
    }

    void Insert(int x, int y) {
        int h = hash(x, y);
        while (table_[h].used) {
            if (table_[h].x == x && table_[h].y == y) {
                return;
            }
            h = (h + 1) % M;
        }
        table_[h] = {x, y, true};
    }

    bool Contains(int x, int y) {
        int h = hash(x, y);
        int start = h;
        while (table_[h].used) {
            if (table_[h].x == x && table_[h].y == y) {
                return true;
            }
            h = (h + 1) % M;
            if (start == h) {
                break;
            }
        }
        return false;
    }

    bool Empty() {
        for (auto& p : table_) {
            if (p.used) {
                return false;
            }
        }
        return true;
    }
    
    Point GetPoint() {
        for (auto& p : table_) {
            if (p.used) {
                return p;
            }
        }
        return {0, 0, false};
    }

    void DeleteLineX(int x) {
        for (auto& p : table_) {
            if (p.used && p.x == x) {
                p.used = false;
            }
        }
    }


    void DeleteLineY(int y) {
        for (auto& p : table_) {
            if (p.used && p.y == y) {
                p.used = false;
            }
        }
    }

    bool Solution2(Hash h) {
        if (h.Empty()) {
            return true;
        }

        Point p1 = h.GetPoint();

        for (int i = 0; i < 2; i++) {
            Hash temp1 = h;
            if (i == 0) {
                temp1.DeleteLineX(p1.x);
            }
            else {
                temp1.DeleteLineY(p1.y);
            }
            if (temp1.Empty()) {
                return true;
            }

            Point p2 = temp1.GetPoint();

            for (int j = 0; j < 2; j++) {
                Hash temp2 = temp1;
                if (j == 0) {
                    temp2.DeleteLineX(p2.x);
                }
                else {
                    temp2.DeleteLineY(p2.y);
                }
                if (temp2.Empty()) {
                    return true;
                }
            }
        }
        return false;
    }

    bool Solution3(Hash h) {
        if (h.Empty()) {
            return true;
        }

        Point p1 = h.GetPoint();

        Hash tempX = h;
        tempX.DeleteLineX(p1.x);
        if (Solution2(tempX)) {
            return true;
        }


        Hash tempY = h;
        tempY.DeleteLineY(p1.y);
        if (Solution2(tempY)) {
            return true;
        }

        return false;
    }
};



int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    
    int N;
    std::cin >> N;
    
    Hash h(N);

    for (int i = 0; i < N; i++) {
        int x, y;
        std::cin >> x >> y;
        h.Insert(x, y);
    }

    if (h.Solution3(h)) {
        std::cout << "YES";
    }
    else {
        std::cout << "NO";
    }

    return 0;
}