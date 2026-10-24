#include"CHaser.hpp"
#include<iostream>
#include<cstdlib>
#include<ctime>
#include<algorithm>
#include<string>
using namespace std;
// 記憶した地図をBFSで探索し、アイテム・未探索領域へ向かう。
#include <map>
#include <utility>
#include <queue>
constexpr char UNKNOWN = '?';
constexpr char EMPTY   = '0';
constexpr char ENEMY   = '1';
constexpr char WALL    = '2';
constexpr char ITEM    = '3';
static const int dx[] = {0, -1, 1, 0};
static const int dy[] = {-1, 0, 0, 1};

struct Mapping {
    // 出発点を(0, 0)とする相対座標。xは右、yは下が正。
    int x = 0;
    int y = 0;
    // 未観測の座標は登録しない。盤面の大きさは仮定しない。
    std::map<std::pair<int, int>, char> cells;

    char at(int px, int py) const {
        auto it = cells.find({px, py});
        return it == cells.end() ? UNKNOWN : it->second;
    }

    void observe(const std::string& neighbor) {
        // 敵は移動するため、前ターンの位置を残さない。
        // 敵の足元の地形は観測できていないので未観測に戻す。
        for (auto it = cells.begin(); it != cells.end();) {
            if (it->second == ENEMY) it = cells.erase(it);
            else ++it;
        }
        for (int i = 0; i < 9; ++i) {
            cells[{x + i % 3 - 1, y + i / 3 - 1}] = neighbor[i];
        }
        cells[{x, y}] = EMPTY;
    }

    void observe_search(int direction, const string& result) {
        // searchは指定方向の距離1〜9の順。現在地は変わらない。
        for (int i = 0; i < 9; ++i) {
            cells[{x + dx[direction] * (i + 1),
                   y + dy[direction] * (i + 1)}] = result[i];
        }
    }

    void moved(int direction) {
        int nx = x + dx[direction], ny = y + dy[direction];
        // アイテム取得時は「移動元」が壁になる。移動先は取得後の床。
        if (at(nx, ny) == ITEM) cells[{x, y}] = WALL;
        x = nx;
        y = ny;
        cells[{x, y}] = EMPTY;
    }
};

// 到達済みかどうかはdist.count()で判定する。
// 各マスへ向かう「最初の方向」も記録し、経路の逆追跡を省く。
int choose_direction(const Mapping& mapping, bool avoid_enemy = true) {
    using Position = pair<int, int>;
    queue<Position> que;
    map<Position, int> dist;
    map<Position, int> first_direction;
    Position start = {mapping.x, mapping.y};
    dist[start] = 0;
    que.push(start); // 現在地は壁扱いでも出発できる。
    int frontier_direction = -1;
    int best_unknown = 0;
    int best_distance = 0;
    int fallback_direction = -1;

    while (!que.empty()) {
        auto [x, y] = que.front();
        que.pop();
        Position current = {x, y};
        // BFSは近い順。最初に見つけたアイテムを目指す。
        if (current != start && mapping.at(x, y) == ITEM) {
            return first_direction[current];
        }
        // アイテムが見つからない場合に備え、探索先も比較する。
        // 評価 = 到着時に見える未知マス数 / (移動距離 + 1)。
        // 現在地は既に観測済みなので候補にしない。
        if (current != start) {
            int unknown = 0;
            for (int oy = -1; oy <= 1; ++oy) {
                for (int ox = -1; ox <= 1; ++ox) {
                    if (mapping.at(x + ox, y + oy) == UNKNOWN) ++unknown;
                }
            }
            int distance = dist[current];
            // 分数は交差積で比較。同点なら近い候補を優先する。
            if (unknown > 0 && (frontier_direction == -1 ||
                1LL * unknown * (best_distance + 1) > 1LL * best_unknown * (distance + 1) ||
                (1LL * unknown * (best_distance + 1) == 1LL * best_unknown * (distance + 1) &&
                 distance < best_distance))) {
                frontier_direction = first_direction[current];
                best_unknown = unknown;
                best_distance = distance;
            }
        }
        for (int d = 0; d < 4; ++d) {
            int nx = x + dx[d], ny = y + dy[d];
            Position next = {nx, ny};
            char cell = mapping.at(nx, ny);
            // 斜め敵は減点：今回の一歩で敵の隣へ出る方向を後回しにする。
            // 遠い未来の経路には、現在の敵位置を固定して適用しない。
            if (avoid_enemy && current == start) {
                bool danger = false;
                for (int e = 0; e < 4; ++e) {
                    if (mapping.at(nx + dx[e], ny + dy[e]) == ENEMY) danger = true;
                }
                if (danger) continue;
            }
            // 未知のマスへ直接は進まず、観測済みの床・アイテムだけ探索。
            if (dist.count(next)) continue;
            if (cell != EMPTY && cell != ITEM) continue;
            dist[next] = dist[current] + 1;
            first_direction[next] = current == start ? d : first_direction[current];
            if (current == start && fallback_direction == -1) fallback_direction = d;
            que.push(next);
        }
    }
    if (frontier_direction != -1) return frontier_direction;
    return fallback_direction; // 既知のアイテムも未探索領域もない場合。
}

// 待機中にも地図を広げるため、直線9マスの未知が最も多い方向を選ぶ。
int choose_search_direction(const Mapping& mapping) {
    int best = 0, best_unknown = -1;
    for (int d = 0; d < 4; ++d) {
        int unknown = 0;
        for (int i = 1; i <= 9; ++i) {
            if (mapping.at(mapping.x + dx[d] * i, mapping.y + dy[d] * i) == UNKNOWN) {
                ++unknown;
            }
        }
        if (unknown > best_unknown) {
            best = d;
            best_unknown = unknown;
        }
    }
    return best;
}

int main() {
    Mapping mapping;
    int count = 0;
    void (*walks[])() = {walk_up, walk_left, walk_right, walk_down};
    string (*searches[])() = {search_up, search_left, search_right, search_down};
    void (*puts[])() = {put_up, put_left, put_right, put_down};
    cout << "CHaser猫のちぇいにゃんだにゃ〜ん☆" << endl;
    cout << "対戦よろしくだにゃん☆" << endl;
    while (true) {
        string neighbor = get_ready();
        // EOFや終了通知を9マスとして読み込まない。
        if (neighbor.size() != 9 || any_of(neighbor.begin(), neighbor.end(), [](char cell) {
            return cell != EMPTY && cell != ENEMY && cell != WALL && cell != ITEM;
        })) break;
        mapping.observe(neighbor);
        cout << ++count << "ターン目 position=(" << mapping.x << "," << mapping.y
             << "), known=" << mapping.cells.size() << endl;

        bool acted = false;
        for (int d = 0; d < 4; ++d) {
            if (mapping.at(mapping.x + dx[d], mapping.y + dy[d]) == ENEMY) {
                puts[d]();
                mapping.cells[{mapping.x + dx[d], mapping.y + dy[d]}] = WALL;
                acted = true;
                break;
            }
        }
        if (acted) continue;

        int direction = choose_direction(mapping);
        if (direction == -1) {
            // 斜め敵に隣接する移動しかないなら、動かずsearchで1ターン使う。
            if (choose_direction(mapping, false) != -1) {
                int search_direction = choose_search_direction(mapping);
                cout << "危険な移動を避けてsearch: direction=" << search_direction << endl;
                // ヘッダのsearch関数が探索結果を1行読み取る。
                string result = searches[search_direction]();
                if (result.size() != 9 || any_of(result.begin(), result.end(), [](char cell) {
                    return cell != EMPTY && cell != ENEMY && cell != WALL && cell != ITEM;
                })) break;
                mapping.observe_search(search_direction, result);
                continue; // 次ターンの周囲情報で攻撃・移動を判断し直す。
            }
        }
        if (direction == -1) {
            // 全方向が壁なら移動できない。通常はゲーム側で終了する。
            cout << "移動できるマスがないにゃん" << endl;
            break;
        }
        cout << "BFS: direction=" << direction << endl;
        walks[direction]();
        mapping.moved(direction);
    }
    return 0;
}
