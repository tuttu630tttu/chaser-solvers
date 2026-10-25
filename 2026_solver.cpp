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
#include <set>
#include <vector>
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
    static constexpr int ENEMY_MEMORY_TURNS = 3;
    int enemy_age = -1; // -1は警戒なし、0は今観測した敵。
    set<pair<int, int>> enemy_candidates;
    // 未観測の座標は登録しない。盤面の大きさは仮定しない。
    std::map<std::pair<int, int>, char> cells;

    char at(int px, int py) const {
        auto it = cells.find({px, py});
        return it == cells.end() ? UNKNOWN : it->second;
    }

    void advance_enemy_memory() {
        if (enemy_age < 0) return;
        if (++enemy_age > ENEMY_MEMORY_TURNS) {
            enemy_candidates.clear();
            enemy_age = -1;
            return;
        }
        auto next = enemy_candidates; // 相手がput/search等で移動しない場合。
        for (auto [ex, ey] : enemy_candidates) {
            for (int d = 0; d < 4; ++d) {
                int nx = ex + dx[d], ny = ey + dy[d];
                if (at(nx, ny) != WALL) next.insert({nx, ny});
            }
        }
        enemy_candidates = std::move(next);
    }

    void refine_enemy_memory(const vector<pair<pair<int, int>, char>>& observed) {
        set<pair<int, int>> visible;
        for (const auto& [position, cell] : observed) {
            if (cell == ENEMY) visible.insert(position);
        }
        if (!visible.empty()) {
            enemy_candidates = std::move(visible);
            enemy_age = 0;
        } else {
            // 実際に観測した非敵マスだけ除外。古い地図からは除外しない。
            for (const auto& [position, cell] : observed) {
                if (cell != ENEMY) enemy_candidates.erase(position);
            }
        }
    }

    bool enemy_risk(int px, int py) const {
        for (auto [ex, ey] : enemy_candidates) {
            // 敵がいるかもしれないマス自体にも踏み込まない。
            if (abs(px - ex) + abs(py - ey) <= 1) return true;
        }
        return false;
    }

    void observe(const std::string& neighbor) {
        advance_enemy_memory();
        vector<pair<pair<int, int>, char>> observed;
        // 敵は移動するため、前ターンの位置を残さない。
        // 敵の足元の地形は観測できていないので未観測に戻す。
        for (auto it = cells.begin(); it != cells.end();) {
            if (it->second == ENEMY) it = cells.erase(it);
            else ++it;
        }
        for (int i = 0; i < 9; ++i) {
            pair<int, int> position = {x + i % 3 - 1, y + i / 3 - 1};
            cells[position] = neighbor[i];
            observed.push_back({position, i == 4 ? EMPTY : neighbor[i]});
        }
        cells[{x, y}] = EMPTY;
        refine_enemy_memory(observed);
    }

    void observe_search(int direction, const string& result) {
        // searchは指定方向の距離1〜9の順。現在地は変わらない。
        vector<pair<pair<int, int>, char>> observed;
        for (int i = 0; i < 9; ++i) {
            pair<int, int> position = {x + dx[direction] * (i + 1),
                                       y + dy[direction] * (i + 1)};
            cells[position] = result[i];
            observed.push_back({position, result[i]});
        }
        refine_enemy_memory(observed);
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

// デバッグ用。行動選択には使わず、決定した目的地を記録する。
struct DecisionLog {
    const char* reason = "NO_MOVE";
    int target_x = 0, target_y = 0;
    int distance = 0;
    int unknown = 0;
};

int unknown_around(const Mapping& mapping, int x, int y) {
    int count = 0;
    for (int oy = -1; oy <= 1; ++oy) {
        for (int ox = -1; ox <= 1; ++ox) {
            if (mapping.at(x + ox, y + oy) == UNKNOWN) ++count;
        }
    }
    return count;
}

// アイテム取得で移動元が壁になった後、移動先から出られるか。
// 2=既知の出口あり、1=未知の出口候補のみ、0=出口なし。
int item_exit_status(const Mapping& mapping, int from_x, int from_y, int x, int y) {
    bool unknown = false;
    for (int d = 0; d < 4; ++d) {
        int nx = x + dx[d], ny = y + dy[d];
        if (nx == from_x && ny == from_y) continue; // 取得と同時に壁になる。
        char cell = mapping.at(nx, ny);
        if (cell == EMPTY || cell == ITEM) return 2;
        if (cell == UNKNOWN) unknown = true;
    }
    return unknown ? 1 : 0;
}

// 到達済みかどうかはdist.count()で判定する。
// 各マスへ向かう「最初の方向」も記録し、経路の逆追跡を省く。
int choose_direction(const Mapping& mapping, bool avoid_enemy = true, DecisionLog* log = nullptr) {
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
    Position frontier_target = start;
    auto chosen = [&](int direction, const char* reason, Position target, int distance) {
        if (log) *log = {reason, target.first, target.second, distance,
                        unknown_around(mapping, target.first, target.second)};
        return direction;
    };

    while (!que.empty()) {
        auto [x, y] = que.front();
        que.pop();
        Position current = {x, y};
        // BFSは近い順。最初に見つけたアイテムを目指す。
        if (current != start && mapping.at(x, y) == ITEM) {
            return chosen(first_direction[current], "ITEM", current, dist[current]);
        }
        // アイテムが見つからない場合に備え、探索先も比較する。
        // 評価 = 到着時に見える未知マス数 / (移動距離 + 1)。
        // 現在地は既に観測済みなので候補にしない。
        if (current != start) {
            int unknown = unknown_around(mapping, x, y);
            int distance = dist[current];
            // 分数は交差積で比較。同点なら近い候補を優先する。
            if (unknown > 0 && (frontier_direction == -1 ||
                1LL * unknown * (best_distance + 1) > 1LL * best_unknown * (distance + 1) ||
                (1LL * unknown * (best_distance + 1) == 1LL * best_unknown * (distance + 1) &&
                 distance < best_distance))) {
                frontier_direction = first_direction[current];
                frontier_target = current;
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
                bool danger = mapping.enemy_risk(nx, ny);
                for (int e = 0; e < 4; ++e) {
                    if (mapping.at(nx + dx[e], ny + dy[e]) == ENEMY) danger = true;
                }
                if (danger) continue;
            }
            // 未知のマスへ直接は進まず、観測済みの床・アイテムだけ探索。
            if (dist.count(next)) continue;
            if (cell != EMPTY && cell != ITEM) continue;
            if (cell == ITEM) {
                int exit_status = item_exit_status(mapping, x, y, nx, ny);
                // 遠い目的地も今回の一歩も同じ基準にする。
                // 未確認のアイテムへ接近→直前で撤退、の往復を防ぐ。
                if (exit_status != 2) continue;
            }
            dist[next] = dist[current] + 1;
            first_direction[next] = current == start ? d : first_direction[current];
            if (current == start && fallback_direction == -1) fallback_direction = d;
            que.push(next);
        }
    }
    if (frontier_direction != -1) {
        return chosen(frontier_direction, "EXPLORE", frontier_target, best_distance);
    }
    if (fallback_direction != -1) {
        return chosen(fallback_direction, "FALLBACK",
                      {mapping.x + dx[fallback_direction], mapping.y + dy[fallback_direction]}, 1);
    }
    return chosen(-1, "NO_MOVE", start, 0);

}

// 待機中にも地図を広げるため、直線9マスの未知が最も多い方向を選ぶ。
int choose_search_direction(const Mapping& mapping) {
    // 隣接アイテムの出口が未知なら、その奥を先に確認する。
    // 隣接先の左右は周囲9マスで見えており、正面2マス先はsearchで確認できる。
    for (int d = 0; d < 4; ++d) {
        int nx = mapping.x + dx[d], ny = mapping.y + dy[d];
        if (mapping.at(nx, ny) == ITEM &&
            item_exit_status(mapping, mapping.x, mapping.y, nx, ny) == 1) return d;
    }
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
    cout << "solver-build: enemy-memory-v1" << endl;
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
        cout << "neighbor=" << neighbor << endl;
        cout << "enemy_memory: age=" << mapping.enemy_age
             << " candidates=" << mapping.enemy_candidates.size() << endl;

        bool acted = false;
        for (int d = 0; d < 4; ++d) {
            if (mapping.at(mapping.x + dx[d], mapping.y + dy[d]) == ENEMY) {
                cout << "decision=PUT target=(" << mapping.x + dx[d] << ","
                     << mapping.y + dy[d] << ") distance=1 direction=" << d << endl;
                puts[d]();
                mapping.cells[{mapping.x + dx[d], mapping.y + dy[d]}] = WALL;
                acted = true;
                break;
            }
        }
        if (acted) continue;

        DecisionLog decision;
        int direction = choose_direction(mapping, true, &decision);
        if (direction == -1) {
            // 危険な接近、または出口未確認・袋小路のアイテムしかないなら待機。
            // 拒否したアイテムを、フォールバックでも踏まない。
            bool has_walkable = false;
            for (int d = 0; d < 4; ++d) {
                char cell = mapping.at(mapping.x + dx[d], mapping.y + dy[d]);
                if (cell == EMPTY || cell == ITEM) has_walkable = true;
            }
            if (has_walkable) {
                int search_direction = choose_search_direction(mapping);
                int unknown = 0;
                for (int i = 1; i <= 9; ++i) {
                    if (mapping.at(mapping.x + dx[search_direction] * i,
                                   mapping.y + dy[search_direction] * i) == UNKNOWN) ++unknown;
                }
                cout << "decision=SEARCH_WAIT target=none distance=0 direction="
                     << search_direction << " unknown_line=" << unknown << endl;
                cout << "危険な移動・閉じ込めを避けてsearch: direction=" << search_direction << endl;
                // ヘッダのsearch関数が探索結果を1行読み取る。
                string result = searches[search_direction]();
                cout << "search_result=" << result << endl;
                if (result.size() != 9 || any_of(result.begin(), result.end(), [](char cell) {
                    return cell != EMPTY && cell != ENEMY && cell != WALL && cell != ITEM;
                })) break;
                mapping.observe_search(search_direction, result);
                continue; // 次ターンの周囲情報で攻撃・移動を判断し直す。
            }
        }
        if (direction == -1) {
            // 全方向が壁なら移動できない。通常はゲーム側で終了する。
            cout << "decision=NO_MOVE target=none distance=0" << endl;
            cout << "移動できるマスがないにゃん" << endl;
            break;
        }
        cout << "decision=" << decision.reason << " target=(" << decision.target_x
             << "," << decision.target_y << ") distance=" << decision.distance
             << " unknown=" << decision.unknown << " direction=" << direction << endl;
        cout << "BFS: direction=" << direction << endl;
        walks[direction]();
        mapping.moved(direction);
    }
    return 0;
}
