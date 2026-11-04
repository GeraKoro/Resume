#include <iostream>
#include <string>
#include <vector>

using namespace std;

const int kAlfCount = 26;

struct Node {
    int len;
    int link;
    int next[kAlfCount];
    Node(int l = 0, int lnk = 0) : len(l), link(lnk) {
        fill(next, next + kAlfCount, -1);
    }
};

int main() {
    string str;
    cin >> str;
    int n = str.size();
    vector<Node> tree;
    tree.push_back(Node(-1, 0));
    tree.push_back(Node(0, 0));
    int longest_suf = 1;
    int unique_count = 0;
    for (int i = 0; i < n; ++i) {
        int c = str[i] - 'a';
        int current = longest_suf;
        while (true) {
            int current_len = tree[current].len;
            if (i - current_len - 1 >= 0 && str[i - current_len - 1] - 'a' == c) {
                break;
            }
            current = tree[current].link;
        }
        if (tree[current].next[c] != -1) {
            longest_suf = tree[current].next[c];
        }
        else {
            int new_node = tree.size();
            tree.push_back(Node(tree[current].len + 2, 0));
            tree[current].next[c] = new_node;
            longest_suf = new_node;
            unique_count++;
            if (tree[new_node].len == 1) {
                tree[new_node].link = 1;
            }
            else {
                int temp = tree[current].link;
                while (true) {
                    int temp_len = tree[temp].len;
                    if (i - temp_len - 1 >= 0 && str[i - temp_len - 1] - 'a' == c)
                        break;
                    temp = tree[temp].link;
                }
                tree[new_node].link = tree[temp].next[c];
            }
        }
        cout << unique_count << '\n';
    }
    return 0;
}