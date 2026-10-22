#include"CHaser.hpp"
#include<iostream>
#include<cstdlib>
#include<ctime>
#include<algorithm>
#include<string>
using namespace std;
int main() {
srand(time(nullptr));
int grid = -1; // 記憶
int count = 0; // 自ターンのカウント
cout << "CHaser猫のちぇいにゃんだにゃ〜ん☆" << endl;
cout << "対戦よろしくだにゃん☆" << endl;
while (true) {
count++;
cout << " " << count << "ターン目にゃん" <<endl;
string neighbor = get_ready();
cout << "周囲の確認にゃ: " << neighbor << endl;
// 各方向マスの状態
int up    = neighbor[1] - '0';
int left  = neighbor[3] - '0';
int right = neighbor[5] - '0';
int down  = neighbor[7] - '0';
string naname = "0268";
bool acted = false;

// 各移動方向の粗評価
    // 4アイテム, 3なにもない, 0壁
    int safe[4];
    safe[0] = (up == 3) ? 4 : (up == 0 ? 3 : 0);
    safe[1] = (left == 3) ? 4 : (left == 0 ? 3 : 0);
    safe[2] = (right == 3) ? 4 : (right == 0 ? 3 : 0);
    safe[3] = (down == 3) ? 4 : (down == 0 ? 3 : 0);
    //攻撃
    if (up == 1) {
        cout << "上に敵発見にゃ!putするにゃん!" << endl; put_up(); acted = true;
    } else if (left == 1) {
        cout << "左に敵確認にゃ!いっくぞ〜" << endl; put_left(); acted = true;
    } else if (right == 1) {
        cout << "ぼくの右側に立つにゃんて〜" << endl; put_right(); acted = true;
    } else if (down == 1) {
        cout << "下側に標的にゃ！くらえにゃ〜" << endl; put_down(); acted = true;
    }
    // 細かい評価フェーズ
    // 4(+1)アイテム, 3(+1)なにもない, 3はさまれアイテム, 2前回方向, 1斜め敵, 0壁, 斜めアイテムは加点
    // 前回マスを減点
    if(!acted && !(grid == -1)){
        safe[grid] = 2;
    }
    // 斜めアイテムは加点
    if(!acted){
        for (char x : naname) {
            int idx = x - '0';
            if (neighbor[idx] == '3') {  // アイテム3
            cout << idx <<"におやつの香りがするにゃん…" << endl;
                switch (idx) {
                    case 0: // 左上
                        if (safe[0] > 0) safe[0]++; // 上
                        if (safe[1] > 0) safe[1]++; // 左
                        break;
                    case 2: // 右上
                        if (safe[0] > 0) safe[0]++; // 上
                        if (safe[2] > 0) safe[2]++; // 右
                        break;
                    case 6: // 左下
                        if (safe[1] > 0) safe[1]++; // 左
                        if (safe[3] > 0) safe[3]++; // 下
                        break;
                    case 8: // 右下
                        if (safe[2] > 0) safe[2]++; // 右
                        if (safe[3] > 0) safe[3]++; // 下
                        break;
                }
            }
        }
    }
    // はさまれアイテムは減点
    if(!acted){
        if(up == 3 && neighbor[0] == '2' && neighbor[2] == '2') safe[0] = 3;
        if(left == 3 && neighbor[0] == '2' && neighbor[6] == '2') safe[1] = 3;
        if(right == 3 && neighbor[2] == '2' && neighbor[8] == '2') safe[2] = 3;
        if(down == 3 && neighbor[6] == '2' && neighbor[8] == '2') safe[3] = 3;
    }
    // 斜め敵は減点
    if(!acted){
        for (char x : naname) {
            int idx = x - '0';
            if (neighbor[idx] == '1') {  // 敵1
                cout << "斜めに敵, 警戒するにゃん!" << endl;
                switch (idx) {
                    case 0: // 左上
                        safe[0] = 1; // 上
                        safe[1] = 1; // 左
                        break;
                    case 2: // 右上
                        safe[0] = 1; // 上
                        safe[2] = 1; // 右
                        break;
                    case 6: // 左下
                        safe[1] = 1; // 左
                        safe[3] = 1; // 下
                        break;
                    case 8: // 右下
                        safe[2] = 1; // 右
                        safe[3] = 1; // 下
                        break;
                }
            }
        }
    }
    // 移動
    if (!acted) {
        // ランダム順で探索
        struct info {int di; int score; };
        info safe2[4] = {{0, safe[0]},{1, safe[1]},{2, safe[2]},{3, safe[3]}};
        for (int i = 3; i > 0; i--) {
            int j = rand() % (i + 1);
            swap(safe2[i], safe2[j]);
        }
        // マスのスコア順にソート
        sort(safe2,safe2 + 4, [](auto&a, auto&b){return a.score > b.score; });
        int dd = safe2[0].di;
        switch(dd){ // 移動
            case 0: cout << "上に行くにゃん" << endl; walk_up(); break;
            case 1: cout << "左に進むにゃんよ" << endl; walk_left(); break;
            case 2: cout << "右に移動するにゃん" << endl; walk_right(); break;
            case 3: cout << "下に降りるにゃん" << endl; walk_down(); break;
        }
        switch(dd){ // 記憶
            case 0: grid = 3; break;
            case 1: grid = 2; break;
            case 2: grid = 1; break;
            case 3: grid = 0; break;
        }
    }
}
    return 0;

}
