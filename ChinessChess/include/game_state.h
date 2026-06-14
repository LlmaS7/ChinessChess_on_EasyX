#pragma once
#include "chess_def.h"
#include <vector>

struct ChessPiece // id color river
{
    int id;    // 接Piece枚举
    int color; // 接Color枚举
    // 废弃： int x,y
    // 废弃： 是否过河 int river;
};

struct GameState // board[][] 当前玩家 回合数 当前阶段 选中坐标 gameOver winnner
{
    ChessPiece board[ROW][COL]; // 每个格子：GameState--ChessPiece--id/color/xy/river
    int current_side;           // 当前玩家 RED/BLACK
    int turn_count;             // 回合数
    int phase;                  // 当前阶段：FIRST选 SECOND走
    int selected_x, selected_y; // 选中坐标 (-1 未选中)
    bool game_over;
    int winner; // 胜方颜色, -1 未结束

    std::vector<Move> history; // 走法历史（悔棋用）（可选：记录棋谱？）
    bool undo_req_pending;     // 对方请求悔棋，等待本地回应
};
