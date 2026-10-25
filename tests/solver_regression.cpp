// solver本体のmainを差し替え、地図と経路選択を直接テストする。
#define main solver_main
#include "../2026_solver.cpp"
#undef main
#include <cassert>
int main() {
 Mapping m;
 m.observe("000000000");m.moved(0);
 m.observe("222000000");m.moved(1);
 m.observe("222300200");
 assert(choose_direction(m)==3);
 m.moved(3);m.observe("300200000");
 DecisionLog log;
 int d=choose_direction(m,true,&log);
 assert(string(log.reason)!="ITEM"); // 4ターン目、未確認アイテムへ引き返さない。
 assert(d==3);
 m.moved(d);m.observe("200000000");
 assert(choose_direction(m,true,&log)!=0); // 探索を続ける。
 for(int dir=0;dir<4;++dir) {
  Mapping trap;
  for(int x=-3;x<=3;++x)for(int y=-3;y<=3;++y)trap.cells[{x,y}]=WALL;
  trap.cells[{0,0}]=EMPTY;
  int x=dx[dir],y=dy[dir];trap.cells[{x,y}]=ITEM;
  assert(choose_direction(trap)==-1);
  trap.cells.erase({2*x,2*y});
  assert(choose_direction(trap)==-1);
  assert(choose_search_direction(trap)==dir);
  trap.cells[{2*x,2*y}]=EMPTY;
  assert(choose_direction(trap)==dir);
 }

 // ボード6の34→35ターン目: 退避後、見失った敵へ左に戻らない。
 Mapping memory;
 memory.x=7;memory.y=-4;
 memory.observe("100300020");
 assert(memory.enemy_age==0);
 memory.moved(2);
 memory.observe("002002202");
 assert(memory.enemy_age==1);
 assert(memory.enemy_risk(7,-4));
 assert(choose_direction(memory)!=1);
 // 観測範囲内で敵がいないマスは候補から消えている。
 for(int y=-5;y<=-3;++y)for(int x=7;x<=9;++x)
     assert(!memory.enemy_candidates.count({x,y}));
 memory.observe("002002202");assert(memory.enemy_age==2);
 memory.observe("002002202");assert(memory.enemy_age==3);
 memory.observe("002002202");
 assert(memory.enemy_age==-1 && memory.enemy_candidates.empty());
 // 再発見で候補を確定位置に絞り、保持期間をリセット。
 memory.observe("100000000");
 assert(memory.enemy_age==0 && memory.enemy_candidates.size()==1);
 // searchの観測結果も絞り込みに使う。
 Mapping scan;scan.enemy_age=1;scan.enemy_candidates={{1,0},{2,0},{0,2}};
 scan.observe_search(2,"000000000");
 assert(scan.enemy_candidates.size()==1 && scan.enemy_candidates.count({0,2}));
 scan.observe_search(3,"010000000");
 assert(scan.enemy_age==0 && scan.enemy_candidates.size()==1 && scan.enemy_candidates.count({0,2}));
 // 既知壁の向こうへは1手で候補が広がらない。
 Mapping walls;walls.enemy_age=0;walls.enemy_candidates={{3,3}};
 walls.cells[{4,3}]=WALL;walls.advance_enemy_memory();
 assert(!walls.enemy_candidates.count({4,3}));
}
