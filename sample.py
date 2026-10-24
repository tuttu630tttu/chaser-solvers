import CHaser
import random

# 定数
BLANK = '0'
ENEMY = '1'
BLOCK = '2'
ITEM = '3'

UP = 0
LEFT = 1
RIGHT = 2
DOWN = 3

client = CHaser.CHaserClient()
looks = [client.look_up, client.look_left, client.look_right, client.look_down]
searches = [client.search_up, client.search_left, client.search_right, client.search_down]
walks = [client.walk_up, client.walk_left, client.walk_right, client.walk_down]
puts = [client.put_up, client.put_left, client.put_right, client.put_down]

here = {"y": 20, "x": 20}
"""現在位置"""
neighbor = []
"""周囲のブロック９マス"""
Chaser_map = [['-' for _ in range(41)] for _ in range(41)]
"""全体マップ"""

#Bot2 Code
class Node():
    def __init__(self, parent=None, position=None):
        self.parent = parent
        self.position = position 
        self.g = 0
        self.h = 0
        self.f = 0

    def __eq__(self, other):
        return self.position == other.position

# def search_focus():
#     """探索する方向を決定する,多分使ってない"""
#     c_up=c_down=c_left=c_right=0

#     for i in range(9):
#         if Chaser_map[here["y"]-1-i][here["x"]] == '-': c_up+=1
#     for i in range(9):
#         if Chaser_map[here["y"]+1-+i][here["x"]] == '-': c_down+=1
#     for i in range(9):
#         if Chaser_map[here["y"]][here["x"]+1+i] == '-': c_right+=1
#     for i in range(9):
#         if Chaser_map[here["y"]][here["x"]-1-i] == '-': c_left+=1

#     unexplored_counts = [c_up, c_left, c_right, c_down]
#     return unexplored_counts.index(max(unexplored_counts))

# def search_fill(focus):
#     """指定方向のsearch関数の使用,マップ更新  たぶん使ってない"""
#     global Chaser_map
#     tmp_list = list(searches[focus]())
#     if focus == 0: 
#         for i in range(9):
#             Chaser_map[here["y"]-1-i][here["x"]] = tmp_list[i]
#     elif focus == 1: 
#         for i in range(9):
#             Chaser_map[here["y"]][here["x"]-1-i] = tmp_list[i]
#     elif focus == 2: 
#         for i in range(9):
#             Chaser_map[here["y"]][here["x"]+1+i] = tmp_list[i]
#     elif focus == 3:
#         for i in range(9):
#             Chaser_map[here["y"]+1+i][here["x"]] = tmp_list[i]

def store_neighbor_blocks():
    '''
    隣接するブロックの取得,マップ保存
    '''
    global Chaser_map
    global neighbor
    neighbor = input()  
    reshaped_neighbor = [neighbor[0:3], neighbor[3:6], neighbor[6:9]]
    for i in range(3):
        for j in range(3):
            Chaser_map[here["y"]-1+i][here["x"]-1+j] = reshaped_neighbor[i][j]

def walk_fill(focus)-> bool:
    """
    指定方向へのwalk関数の使用,所在（here）の変更

    :param int focus: 0から3の方向を示す数値
    :return: walkができたか真偽値
    
    """
    global here
    global Chaser_map
    y_move=[-1,0,0,1]
    x_move=[0,-1,1,0]
    target_y = here["y"]+y_move[focus]
    target_x = here["x"]+x_move[focus]
    if Chaser_map[target_y][target_x] != '2':
        walks[focus]()
        here["y"], here["x"] = target_y, target_x
        return True
    return False

def check_enemy():
    """周囲の敵ブロックを確認し、putsする。putしたらTrueを返す"""
    for (dy, dx), direction in [
        ((-1, 0), 0), ((0, 1), 2), ((0, -1), 1), ((1, 0), 3),
        ((-1, -1), 0), ((-1, 1), 2), ((1, -1), 1), ((1, 1), 3)
    ]:
        if Chaser_map[here["y"] + dy][here["x"] + dx] == '1':
            puts[direction]()
            return True
    return False

    
def check_item_death(target)-> bool:
    """アイテムブロックの周囲に敵ブロックが3つ以上あるか確認する"""
    global Chaser_map

    count=0
    if Chaser_map[target["y"]-1][target["x"]] == '2': count += 1
    if Chaser_map[target["y"]][target["x"]+1] == '2': count += 1
    if Chaser_map[target["y"]][target["x"]-1] == '2': count += 1
    if Chaser_map[target["y"]+1][target["x"]] == '2': count += 1

    if count>2:
        Chaser_map[target["y"]][target["x"]] = '2'
        return False
    return True

def compere_walk(target):
    """目的地と現在地を比較して、目的地に向かって歩く"""
    if target["y"] > here["y"]: walk_fill(3)  
    elif target["y"] < here["y"]: walk_fill(0)
    elif target["x"] > here["x"]: walk_fill(2) 
    elif target["x"] < here["x"]: walk_fill(1)  


def closest_block(block):
    """指定ブロックの中で、現在地から最も近いブロックを探す"""
    min_distance = float('inf')
    closest = {"y": None, "x": None}
    for i in range(len(Chaser_map)):
        for j in range(len(Chaser_map[i])):
            if Chaser_map[i][j] == block:
                distance = abs(here["y"] - i) + abs(here["x"] - j)
                if distance < min_distance:
                    min_distance = distance
                    closest = {"y": i, "x": j}
    return closest

def find_unexplored_blank():
    """未探索のブロックの中で、現在地から最も近いブロックを探す"""
    closest_distance = float('inf')
    closest_position = {"y": None, "x": None}
    for i in range(len(Chaser_map)):
        for j in range(len(Chaser_map[i])):
            if Chaser_map[i][j] == '0':
                for dy, dx in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
                    ny, nx = i + dy, j + dx
                    if Chaser_map[ny][nx] == '-':
                        distance = abs(here["y"] - ny) + abs(here["x"] - nx)
                        if distance < closest_distance:
                            closest_distance = distance
                            closest_position = {"y": ny, "x": nx}
    return closest_position

def a_star(start, goal):
    """A*アルゴリズムを使用して、最短経路を探索する"""
    start_node = Node(None, {"y": start["y"], "x": start["x"]})
    end_node = Node(None, {"y": goal["y"], "x": goal["x"]})
    start_node.g = start_node.h = start_node.f = 0
    end_node.g = end_node.h = end_node.f = 0

    open_list = []
    closed_list = [] 
    open_list.append(start_node)
    while len(open_list) > 0:

        current_node = open_list[0]
        current_index = 0
        for index, item in enumerate(open_list):
            if item.f < current_node.f:
                current_node = item
                current_index = index
        open_list.pop(current_index)
        closed_list.append(current_node)

        if current_node == end_node:
            path = []
            current = current_node
            while current is not None:
                path.append(current.position)
                current = current.parent
            return path[::-1] 

        children = []
        for new_position in [{"y": 0, "x": -1}, {"y": 0, "x": 1}, {"y": -1, "x": 0}, {"y": 1, "x": 0}]:
            node_position = {"y": current_node.position["y"] + new_position["y"],
                             "x": current_node.position["x"] + new_position["x"]}

            if Chaser_map[node_position["y"]][node_position["x"]] == '2':
                continue

            new_node = Node(current_node, node_position)
            children.append(new_node)

        for child in children:
            if len([closed_child for closed_child in closed_list if closed_child == child]) > 0:
                continue

            child.g = current_node.g + 1
            child.h = abs(child.position["y"] - end_node.position["y"]) + abs(child.position["x"] - end_node.position["x"])
            child.f = child.g + child.h

            if len([open_node for open_node in open_list if child.position == open_node.position and child.g > open_node.g]) > 0:
                continue

            open_list.append(child)

def closest_way(item):
    """指定ブロックの中で、現在地から最も近いブロックに向かって歩く"""

    while True:
        target = closest_block(item)
        if target["y"] is None or target["x"] is None:
            return False
        if check_item_death(target):
            break

    
    if abs(here["y"]-target["y"]) + abs(here["x"]-target["x"]) <= 1:
        compere_walk(target)
        return True 
    
    path = a_star(here, target)
    next_step = path[1]
    if Chaser_map[next_step["y"]][next_step["x"]] != '2':
        compere_walk(next_step)
    else:
        return False
    return True

def random_work():
    """ランダムに歩く、たぶんここまでいかない"""
    target = find_unexplored_blank()
    if target["y"] is None or target["x"] is None:
        while True:
            if walk_fill(random.randint(0,3)):
                break

    path = a_star(here, target)
    next_step = path[1]
    if Chaser_map[next_step["y"]][next_step["x"]] != '2': 
        compere_walk(next_step)

def main():
    for i in range(500):
        store_neighbor_blocks()
        if not check_enemy(): # check_enemyがFalseを返した場合（putしなかった場合）に実行
            if not closest_way(ITEM):
                random_work()


if __name__ == "__main__":
    main()
