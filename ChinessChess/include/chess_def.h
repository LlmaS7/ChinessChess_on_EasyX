#pragma once

#define COL 9
#define ROW 10
enum Piece // 棋子 黑0-6 红7-13 空-1
{
    SPACE = -1,
    車,
    馬,
    象,
    士,
    将,
    砲,
    卒,
    俥,
    马,
    相,
    仕,
    帥,
    炮,
    兵
};
enum Color
{
    _NONE = -1,
    _RED,
    _BLACK
};
enum Phase // FIRST等待选棋 SECOND等待走棋
{
    SELECT_FIRST, // 等待选棋
    SELECT_SECOND // 等待走棋
};
struct Move // 起 终 移动棋子类型/颜色 被吃棋子类型/颜色
{
    int from_x, from_y; // 起点
    int to_x, to_y;     // 终点
    int moved_id;       // 移动棋子类型
    int moved_color;    // 移动棋子颜色
    int captured_id;    // 被吃棋子类型
    int captured_color; // 被吃棋子颜色
};

// 以下用于draw_board与mouse_input

const int BOARD_LEFT = 60;
const int BOARD_TOP = 40;
const int CELL_SIZE = 60; // 格子大小
// 逻辑坐标->像素坐标
inline int toX(int col) { return BOARD_LEFT + col * CELL_SIZE; }
inline int toY(int row) { return BOARD_TOP + row * CELL_SIZE; }


// UI 面板
const int UI_PANEL_X  = 600;
const int UI_BUTTON_W = 140;
const int UI_BUTTON_H = 40;
const int UI_BTN_X    = 610;

const int UI_LOSE_B_Y = 110;  // 黑认输
const int UI_UNDO_Y     = 290;  // 悔棋
const int UI_LOSE_R_Y = 470;  // 红认输

// 悔棋请求弹窗按钮
const int UI_UNDO_REQ_Y = 200;  // 悔棋请求弹窗Y
const int UI_ACCEPT_X   = 615;  // 同意按钮X
const int UI_REJECT_X   = 665;  // 拒绝按钮X
const int UI_REQ_BTN_W  = 60;   // 弹窗按钮宽
const int UI_REQ_BTN_H  = 28;   // 弹窗按钮高
