// solver本体のmainを差し替え、地図と経路選択を直接テストする。
#define main solver_main
#include "../2026_solver.cpp"
#undef main
#include <cassert>
#include <fstream>
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

 // 近いアイテムへの経路が警戒範囲を通るなら、遠くても安全な方を選ぶ。
 Mapping route;
 for(int x=-7;x<=7;++x)for(int y=-7;y<=7;++y)route.cells[{x,y}]=WALL;
 for(int x=0;x<=5;++x)route.cells[{x,0}]=EMPTY;
 route.cells[{4,0}]=ITEM;
 for(int y=-1;y>=-6;--y)route.cells[{0,y}]=EMPTY;
 route.cells[{0,-5}]=ITEM;
 route.cells[{3,1}]=ENEMY;route.enemy_candidates={{3,1}};route.enemy_age=0;
 DecisionLog safe;
 assert(choose_direction(route,true,&safe)==0);
 assert(string(safe.reason)=="ITEM" && safe.target_x==0 && safe.target_y==-5);
 assert(choose_direction(route,false)==2); // 警戒なしでは近い方。
 for(int ex : {-1,1})for(int ey : {-1,1}) {
  Mapping enc;
  for(int x=-3;x<=3;++x)for(int y=-3;y<=3;++y)enc.cells[{x,y}]=EMPTY;
  enc.cells[{ex,ey}]=ENEMY;
  for(int d=0;d<4;++d) {
   int x=ex+dx[d],y=ey+dy[d];
   if(x==0 && y==ey)continue;
   enc.cells[{x,y}]=WALL;
  }
  assert(choose_enclosing_put(enc)==(ey==-1 ? 0 : 3));
  enc.cells.erase({2*ex,ey}); // 未知の壁を決めつけない。
  assert(choose_enclosing_put(enc)==-1);
  enc.cells[{2*ex,ey}]=WALL;
  for(int d=0;d<4;++d) if(!(dx[d]==0 && dy[d]==ey))enc.cells[{dx[d],dy[d]}]=WALL;
  assert(choose_enclosing_put(enc)==-1); // 自分も囲まれるputを拒否。
 }

 // 実戦32Tの観測済み地図。斜め下を先に取ると、3個を4歩で回収。
 Mapping cluster;
 ifstream fixture("tests/fixtures/cluster_turn32.txt");
 assert(fixture);
 fixture >> cluster.x >> cluster.y;
 int fx,fy;char fc;
 while(fixture >> fx >> fy >> fc)cluster.cells[{fx,fy}]=fc;
 CollectionPlan plan=plan_cluster(cluster);
 assert(plan.items.size()==3 && plan.route.size()==4 && plan.route.front()==3);
 assert(plan.expanded<=6000);
 Mapping simulation=cluster;
 int picked=0;
 for(int d:plan.route) {
  int nx=simulation.x+dx[d],ny=simulation.y+dy[d];
  char c=simulation.at(nx,ny);
  assert(c==EMPTY || c==ITEM);
  if(c==ITEM)++picked;
  simulation.moved(d);
 }
 assert(picked==3 && simulation.x==-6 && simulation.y==-15);
 // 取得済みアイテムを2度数えず、動的な壁に突っ込まない。
 assert(simulation.at(-5,-14)==WALL && simulation.at(-6,-14)==WALL);
 Mapping isolated;isolated.observe("000003000");
 assert(plan_cluster(isolated).route.empty());
 // 未確認の出口しかないアイテムの塊で、無理に計画を採用しない。
 Mapping unsafe;
 unsafe.cells={{{0,0},EMPTY},{{1,0},ITEM},{{1,1},ITEM}};
 assert(plan_cluster(unsafe).route.empty());
 // 地形も位置も目的地も同じ探索を再選択したときだけ一時保留。
 ExplorationMemory history;
 DecisionLog explore={"EXPLORE",3,2,5,4};
 Mapping seen;seen.cells[{0,0}]=EMPTY;
 assert(!history.repeating(seen,explore,1));
 assert(history.repeating(seen,explore,3));
 assert(history.blocked(4).count({3,2}));
 assert(history.blocked(12).empty());
 seen.cells[{1,0}]=EMPTY;
 assert(!history.repeating(seen,explore,13));
 DecisionLog item={"ITEM",3,2,5,4};
 assert(!history.repeating(seen,item,14));
 // 保留した探索先は通行禁止ではなく、目的地候補からだけ除外。
 Mapping deferred_map;
 deferred_map.cells={{{0,0},EMPTY},{{1,0},EMPTY}};
 DecisionLog deferred_log;
 set<pair<int,int>> deferred={{1,0}};
 assert(choose_direction(deferred_map,true,&deferred_log,deferred)==-1);

 // 毎ターン計画を作り直しても、同じ既知地図では4歩で3個を取れる。
 Mapping replan=cluster;
 int collected=0;
 for(int step=0;step<4;++step) {
  DecisionLog choice;
  int direction=choose_direction(replan,true,&choice);
  auto next_plan=plan_cluster(replan);
  if(!next_plan.route.empty()) direction=next_plan.route.front();
  assert(direction>=0);
  if(replan.at(replan.x+dx[direction],replan.y+dy[direction])==ITEM)++collected;
  replan.moved(direction);
 }
 assert(collected==3);

 // 観測履歴は敵の消去や同じ範囲の再観測で減らず、重複を数えない。
 Mapping coverage;
 coverage.observe("100000002");
 assert(coverage.observed_cells.size()==9 && coverage.observed_walkable.size()==8);
 coverage.observe("000000002");
 assert(coverage.observed_cells.size()==9 && coverage.observed_walkable.size()==8);
 coverage.observe_search(2,"000000000");
 assert(coverage.observed_cells.size()==17 && coverage.observed_walkable.size()==16);
 coverage.observe_search(2,"222222222");
 assert(coverage.observed_cells.size()==17 && coverage.observed_walkable.size()==16);
}
