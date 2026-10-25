#include "CHaser.hpp"
#include <iostream>

int main() {
    while (true) {
        std::string neighbor = get_ready();
        bool acted = false;
        // 各方向マスの状態
        int up    = neighbor[1] - '0';
        int left  = neighbor[3] - '0';
        int right = neighbor[5] - '0';
        int down  = neighbor[7] - '0';
        if (up == 1) {
            std::cout << "上に敵発見にゃ!putするにゃん!" << std::endl; put_up(); acted = true;
        } else if (left == 1) {
            std::cout << "左に敵確認にゃ!いっくぞ〜" << std::endl; put_left(); acted = true;
        } else if (right == 1) {
            std::cout << "ぼくの右側に立つにゃんて〜" << std::endl; put_right(); acted = true;
        } else if (down == 1) {
            std::cout << "下側に標的にゃ！くらえにゃ〜" << std::endl; put_down(); acted = true;
        }
        if(!acted){
            search_down();
        }
    }
    return 0;
}
