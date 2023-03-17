#include <iostream>
#include <vector>

#define __maxNodes 10

using std::cout;
using std::endl;
using std::vector;

struct Edge
{
    int from;
    int to;
    int weight;

    Edge(int f, int t, int w):from(f), to(t), weight(w) {}
};

vector<int> G[__maxNodes]; /* G[i] 存储顶点 i 出发的边的编号 */
vector<Edge> edges;
typedef vector<int>::iterator iterator_t;
int num_nodes;
int num_left;
int num_right;
int num_edges;
int matching[__maxNodes]; /* 存储求解结果 */
int check[__maxNodes];

bool dfs(int u)
{
    cout << "  follow ups of " << u << ":" << endl;
    for (iterator_t i = G[u].begin(); i != G[u].end(); ++i) { // 对 u 的每个邻接点
        int v = edges[*i].to;
        cout << v << endl;
    cout << "check[v] " << check[v] << endl;
        if (!check[v]) {     // 要求不在交替路中
            check[v] = true; // 放入交替路
      cout << "matching[v] " << matching[v] << endl;
            if (matching[v] == -1 || dfs(matching[v])) {
                // 如果是未盖点，说明交替路为增广路，则交换路径，并返回成功
                matching[v] = u;
                // matching[u] = v; //  for undirected only
                return true;
            }
        }
    }
    return false; // 不存在增广路，返回失败
}

int hungarian()
{
    int ans = 0;
    memset(matching, -1, sizeof(matching));
    for (int u=0; u < num_left; ++u) {
      cout << "checking " << u << endl;
        if (matching[u] == -1) {
      cout << "  matching[u]:" << matching[u] << endl;
            memset(check, 0, sizeof(check));
            if (dfs(u)) {
      cout << "match!" << endl;
                ++ans;
            }
        }
    }
    return ans;
}

int main() {

  num_nodes = 5;
  num_left = 5;
  num_right = 5;
  num_edges = 3;

  edges.push_back(Edge(1, 2, 0));
  edges.push_back(Edge(1, 3, 0));
  edges.push_back(Edge(2, 3, 0));
  // edges.push_back(Edge(1, 2, 0));
  // edges.push_back(Edge(2, 4, 0));

  G[1].push_back(0);
  G[1].push_back(1);
  G[2].push_back(2);
  // G[1].push_back(3);
  // G[2].push_back(4);

  cout << hungarian() << endl;


  return 0;
}