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
#include <tuple>
#include <deque>
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
    // 敵を見失ってcellsから消しても、観測済みという履歴は消さない。
    set<pair<int, int>> observed_cells;
    set<pair<int, int>> observed_walkable;

    void record_observation(pair<int, int> position, char cell) {
        observed_cells.insert(position);
        if (cell == EMPTY || cell == ITEM || cell == ENEMY) observed_walkable.insert(position);
    }
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
            record_observation(position, i == 4 ? EMPTY : neighbor[i]);
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
            record_observation(position, result[i]);
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
int choose_direction(const Mapping& mapping, bool avoid_enemy = true, DecisionLog* log = nullptr,
                     const set<pair<int, int>>& deferred = {}) {
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
            if (unknown > 0 && !deferred.count(current) && (frontier_direction == -1 ||
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
            // 現在の警戒範囲を通る経路・目的地は一旦保留する。
            // 今回の一歩だけ許可し、その先で引き返す往復を避ける。
            // 敵を再観測したり警戒が失効したら、次ターンに経路を再計算。
            if (avoid_enemy) {
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
            if (current == start && fallback_direction == -1 && !deferred.count(next)) fallback_direction = d;
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

// 近くの塊だけ、壁化を含めた移動状態をBFSする。
// 候補4個・取得3個・10歩・6000状態まで。未知や候補外アイテムは踏まない。
struct CollectionPlan {
    vector<int> route;
    vector<pair<int, int>> items;
    int expanded = 0;
};

CollectionPlan plan_cluster(const Mapping& mapping) {
    using Position = pair<int, int>;
    Position start = {mapping.x, mapping.y};
    map<Position, int> distance;
    queue<Position> reach;
    distance[start] = 0;
    reach.push(start);
    vector<pair<int, Position>> nearby;
    while (!reach.empty()) {
        Position here = reach.front(); reach.pop();
        int steps = distance[here];
        if (mapping.at(here.first, here.second) == ITEM) nearby.push_back({steps, here});
        if (steps == 6) continue;
        for (int d = 0; d < 4; ++d) {
            Position next = {here.first + dx[d], here.second + dy[d]};
            char cell = mapping.at(next.first, next.second);
            if (distance.count(next) || (cell != EMPTY && cell != ITEM) ||
                mapping.enemy_risk(next.first, next.second)) continue;
            distance[next] = steps + 1;
            reach.push(next);
        }
    }
    sort(nearby.begin(), nearby.end());
    if (nearby.size() < 2) return {};
    vector<Position> targets = {nearby.front().second};
    // 最寄りのアイテムと、マンハッタン距離3以内でつながる近所の塊。
    for (bool changed = true; changed && targets.size() < 4;) {
        changed = false;
        for (const auto& candidate : nearby) {
            if (find(targets.begin(), targets.end(), candidate.second) != targets.end()) continue;
            bool close = false;
            for (auto target : targets) {
                if (abs(target.first - candidate.second.first) +
                    abs(target.second - candidate.second.second) <= 3) close = true;
            }
            if (close) { targets.push_back(candidate.second); changed = true; }
            if (targets.size() == 4) break;
        }
    }
    if (targets.size() < 2) return {};
    int goal = min(3, static_cast<int>(targets.size()));
    struct State {
        Position position;
        int mask = 0;
        vector<Position> walls; // 取得のたびに壁化した移動元。ソートして状態を比較。
        vector<int> route;
        vector<Position> picked;
    };
    using Key = tuple<Position, int, vector<Position>>;
    queue<State> que;
    set<Key> visited;
    que.push({start, 0, {}, {}, {}});
    visited.insert({start, 0, {}});
    int expanded = 0;
    while (!que.empty() && expanded < 6000) {
        State state = std::move(que.front()); que.pop(); ++expanded;
        if (static_cast<int>(state.picked.size()) == goal) {
            return {state.route, state.picked, expanded};
        }
        if (state.route.size() == 10) continue;
        for (int d = 0; d < 4; ++d) {
            State next = state;
            next.position = {state.position.first + dx[d], state.position.second + dy[d]};
            if (binary_search(state.walls.begin(), state.walls.end(), next.position) ||
                mapping.enemy_risk(next.position.first, next.position.second)) continue;
            char cell = mapping.at(next.position.first, next.position.second);
            if (cell != EMPTY && cell != ITEM) continue;
            if (cell == ITEM) {
                auto it = find(targets.begin(), targets.end(), next.position);
                if (it == targets.end()) continue; // 候補外を踏んで計算を省略しない。
                int bit = 1 << static_cast<int>(it - targets.begin());
                if (!(state.mask & bit)) {
                    next.mask |= bit;
                    next.picked.push_back(next.position);
                    next.walls.push_back(state.position);
                    sort(next.walls.begin(), next.walls.end());
                    next.walls.erase(unique(next.walls.begin(), next.walls.end()), next.walls.end());
                }
            }
            // 取得後に四方壁・未知だけのマスで終わる計画は採らない。
            bool exit = false;
            for (int e = 0; e < 4; ++e) {
                Position q = {next.position.first + dx[e], next.position.second + dy[e]};
                char c = mapping.at(q.first, q.second);
                if ((c == EMPTY || c == ITEM) &&
                    !binary_search(next.walls.begin(), next.walls.end(), q) &&
                    !mapping.enemy_risk(q.first, q.second)) exit = true;
            }
            if (!exit) continue;
            if (!visited.insert({next.position, next.mask, next.walls}).second) continue;
            next.route.push_back(d);
            que.push(std::move(next));
            // キュー自体も上限を設ける。未完了なら従来BFSへ戻す。
            if (visited.size() >= 6000) return {};
        }
    }
    return {};
}

// 同じ位置・同じ目的地で、新しい観測が得られない探索を一時保留する。
// ITEMや接敵の制限は変えず、EXPLORE/FALLBACKだけを対象にする。
struct ExplorationMemory {
    using Position = pair<int, int>;
    using Signature = tuple<Position, Position, string>;
    deque<Signature> recent;
    map<Position, int> blocked_until;

    string terrain_signature(const Mapping& mapping) const {
        string signature;
        for (const auto& [position, cell] : mapping.cells) {
            // 敵位置の消去だけで「新しい地図」と見なさない。
            if (cell == ENEMY) continue;
            signature += to_string(position.first) + "," + to_string(position.second) + ":" + cell + ";";
        }
        return signature;
    }

    set<Position> blocked(int turn) {
        set<Position> result;
        for (auto it = blocked_until.begin(); it != blocked_until.end();) {
            if (it->second < turn) it = blocked_until.erase(it);
            else { result.insert(it->first); ++it; }
        }
        return result;
    }

    bool repeating(const Mapping& mapping, const DecisionLog& decision, int turn) {
        if (string(decision.reason) != "EXPLORE" && string(decision.reason) != "FALLBACK") return false;
        Position target = {decision.target_x, decision.target_y};
        Signature current = {{mapping.x, mapping.y}, target, terrain_signature(mapping)};
        bool repeat = find(recent.begin(), recent.end(), current) != recent.end();
        recent.push_back(current);
        if (recent.size() > 16) recent.pop_front();
        if (repeat) blocked_until[target] = turn + 8;
        return repeat;
    }
};

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

// 確定している壁3枚と、自分の隣へのputで敵の最後の出口を塞ぐ。
// 記憶上の候補敵ではなく、今見えている斜めの敵だけを対象にする。
int choose_enclosing_put(const Mapping& mapping) {
    for (int ex : {-1, 1}) for (int ey : {-1, 1}) {
        int enemy_x = mapping.x + ex, enemy_y = mapping.y + ey;
        if (mapping.at(enemy_x, enemy_y) != ENEMY) continue;
        for (int d = 0; d < 4; ++d) {
            int px = mapping.x + dx[d], py = mapping.y + dy[d];
            char cell = mapping.at(px, py);
            if (cell != EMPTY && cell != ITEM) continue;
            if (abs(px - enemy_x) + abs(py - enemy_y) != 1) continue;
            bool enclosed = true;
            for (int e = 0; e < 4; ++e) {
                int nx = enemy_x + dx[e], ny = enemy_y + dy[e];
                if (nx == px && ny == py) continue;
                if (mapping.at(nx, ny) != WALL) enclosed = false;
            }
            if (!enclosed) continue;
            // 自分まで同時に四方壁にしない。未知を出口として当てにしない。
            bool own_exit = false;
            for (int e = 0; e < 4; ++e) {
                if (e == d) continue;
                char exit = mapping.at(mapping.x + dx[e], mapping.y + dy[e]);
                if (exit == EMPTY || exit == ITEM) own_exit = true;
            }
            if (own_exit) return d;
        }
    }
    return -1;
}

int main() {
    Mapping mapping;
    int count = 0;
    ExplorationMemory exploration_memory;
    void (*walks[])() = {walk_up, walk_left, walk_right, walk_down};
    string (*searches[])() = {search_up, search_left, search_right, search_down};
    void (*puts[])() = {put_up, put_left, put_right, put_down};
    cout << "solver-build: coverage-log-v1 CHaser猫のちぇいにゃんだにゃ〜ん☆" << endl;
    while (true) {
        string neighbor = get_ready();
        // EOFや終了通知を9マスとして読み込まない。
        if (neighbor.size() != 9 || any_of(neighbor.begin(), neighbor.end(), [](char cell) {
            return cell != EMPTY && cell != ENEMY && cell != WALL && cell != ITEM;
        })) break;
        mapping.observe(neighbor);
        cout << ++count << "ターン目 position=(" << mapping.x << "," << mapping.y
             << ") observed=" << mapping.observed_cells.size()
             << " walkable_seen=" << mapping.observed_walkable.size()
             << " known=" << mapping.cells.size()
             << " neighbor=" << neighbor
             << " enemy_age=" << mapping.enemy_age
             << " candidates=" << mapping.enemy_candidates.size() << " ";

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

        int enclosing_direction = choose_enclosing_put(mapping);
        if (enclosing_direction != -1) {
            int px = mapping.x + dx[enclosing_direction];
            int py = mapping.y + dy[enclosing_direction];
            cout << "decision=ENCLOSE_PUT target=(" << px << "," << py
                 << ") direction=" << enclosing_direction << endl;
            puts[enclosing_direction]();
            mapping.cells[{px, py}] = WALL;
            continue;
        }

        DecisionLog decision;
        auto deferred = exploration_memory.blocked(count);
        int direction = choose_direction(mapping, true, &decision, deferred);
        if (direction != -1 && exploration_memory.repeating(mapping, decision, count)) {
            cout << "defer_target=(" << decision.target_x << "," << decision.target_y
                 << ") reason=REPEATED_EXPLORE turns=8" << endl;
            direction = choose_direction(mapping, true, &decision, exploration_memory.blocked(count));
        }
        if (direction != -1 && string(decision.reason) == "ITEM") {
            CollectionPlan plan = plan_cluster(mapping);
            if (!plan.route.empty()) {
                direction = plan.route.front();
                auto target = plan.items.front();
                int px = mapping.x, py = mapping.y, first_distance = 0;
                for (int d : plan.route) {
                    px += dx[d]; py += dy[d]; ++first_distance;
                    if (make_pair(px, py) == target) break;
                }
                decision = {"CLUSTER", target.first, target.second, first_distance,
                            unknown_around(mapping, target.first, target.second)};
                cout << "cluster_plan: items=" << plan.items.size() << " steps=" << plan.route.size()
                     << " expanded=" << plan.expanded << " route=";
                for (int d : plan.route) cout << d;
                cout << " order=";
                for (auto [x, y] : plan.items) cout << "(" << x << "," << y << ")";
                cout << endl;
            }
        }
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
                // ヘッダのsearch関数が探索結果を1行読み取る。
                string result = searches[search_direction]();
                if (result.size() != 9 || any_of(result.begin(), result.end(), [](char cell) {
                    return cell != EMPTY && cell != ENEMY && cell != WALL && cell != ITEM;
                })) break;
                mapping.observe_search(search_direction, result);
                cout << "search_result=" << result << " observed=" << mapping.observed_cells.size()
                     << " walkable_seen=" << mapping.observed_walkable.size() << endl;
                continue; // 次ターンの周囲情報で攻撃・移動を判断し直す。
            }
        }
        if (direction == -1) {
            // 全方向が壁なら移動できない。通常はゲーム側で終了する。
            cout << "decision=NO_MOVE target=none distance=0" << endl;
            break;
        }
        cout << "decision=" << decision.reason << " target=(" << decision.target_x
             << "," << decision.target_y << ") distance=" << decision.distance
             << " unknown=" << decision.unknown << " direction=" << direction << endl;
        walks[direction]();
        mapping.moved(direction);
    }
    // EOF等で自然終了した場合の補助。対戦側による強制終了でも直近ターンに数値が残る。
    cout << "map_summary: turns=" << count << " observed=" << mapping.observed_cells.size()
         << " walkable_seen=" << mapping.observed_walkable.size() << endl;
    return 0;
}
