#pragma once
#include "game_state.h"
// 初始化
void init_game(GameState &state);
// 走棋 fX->tX
bool apply_move(GameState &state, int fX, int fY, int tX, int tY);
// 悔棋
bool undo_move(GameState &state);
//  针对点 (x,y) 遍历棋盘上color方棋子（要求与目标不同色），判断是否能攻击到该点，用于将军判定
bool is_attacked(const GameState &state, int x, int y, int attacker_color);
// 判断color方是否 "正在" 被将军
bool is_in_check(const GameState &state, int color);
// 获取 (fx,fy) 处棋子 "所有的" 合法走法
std::vector<Move> get_legal_moves(GameState &state, int fx, int fy);
// 将死判定 返回胜方颜色，未结束为 -1
int check_over(GameState &state);
// 悔棋回退到指定颜色方的回合（用于联机悔棋协议）
void undo_to_turn(GameState &state, int target_color);
