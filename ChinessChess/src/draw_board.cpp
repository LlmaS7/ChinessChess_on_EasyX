#include "../include/draw_board.h"
#include "../include/chess_engine.h"
#include <graphics.h>

#include <cstdio>

// 改动： 常量与 toX toY 移到chess_def.h里

// 棋子名,仅供draw_board ; 利用 Piece 枚举为索引
static const char *piece_name[] = {
    "車",
    "馬",
    "象",
    "士",
    "将",
    "砲",
    "卒",
    "俥",
    "马",
    "相",
    "仕",
    "帥",
    "炮",
    "兵",
};

void draw_board(GameState &state, bool is_network, bool is_host)
{

    setbkcolor(RGB(241, 208, 147));
    cleardevice();

    // 被将提示 // OPTIMIZED: 只计算一次is_in_check
    bool red_check = is_in_check(state, _RED);
    bool black_check = is_in_check(state, _BLACK);

    if (red_check)
    {
        settextcolor(BLACK);
        settextstyle(40, 0, "楷体");
        const char *msg = "将军";
        int tw = textwidth(msg);
        int th = textheight(msg);
        // 红九宫中心 (4, 8)
        outtextxy(toX(4) - tw / 2, toY(8) - th / 2, msg);
    }
    else if (black_check)
    {
        settextcolor(RED);
        settextstyle(40, 0, "楷体");
        const char *msg = "将军";
        int tw = textwidth(msg);
        int th = textheight(msg);
        // 黑九宫中心 (4, 1)
        outtextxy(toX(4) - tw / 2, toY(1) - th / 2, msg);
    }

    setlinecolor(BLACK);
    // 最外框
    setlinestyle(PS_SOLID, 2);
    rectangle(toX(0) - 5, toY(0) - 5, toX(8) + 5, toY(9) + 5);
    setlinestyle(PS_SOLID, 1);
    // 横线
    for (int i = 0; i < ROW; i++)
        line(toX(0), toY(i), toX(8), toY(i));
    // 竖线 r4r5间断开
    line(toX(0), toY(0), toX(0), toY(9));
    line(toX(8), toY(0), toX(8), toY(9));
    for (int j = 1; j < COL - 1; j++)
    {
        line(toX(j), toY(0), toX(j), toY(4));
        line(toX(j), toY(5), toX(j), toY(9));
    }
    // 九宫格
    line(toX(3), toY(0), toX(5), toY(2));
    line(toX(5), toY(0), toX(3), toY(2));
    line(toX(3), toY(7), toX(5), toY(9));
    line(toX(5), toY(7), toX(3), toY(9));

    // 楚河汉界
    settextstyle(28, 0, "楷体");
    setbkmode(TRANSPARENT);
    settextcolor(BLACK);
    outtextxy(toX(1) + 5, toY(4) + 15, "楚  河");
    outtextxy(toX(5) + 5, toY(4) + 15, "汉  界");

    // 画棋子
    for (int r = 0; r < ROW; r++)
    {
        for (int c = 0; c < COL; c++)
        {
            const ChessPiece &TemP = state.board[r][c];
            if (TemP.id == SPACE) // state.board[r][c].id 为空则不画
                continue;
            // 转像素坐标
            int pX = toX(c), pY = toY(r);
            bool is_selected = (r == state.selected_y && c == state.selected_x);

            // 阴影
            setfillcolor(RGB(180, 165, 130));
            setlinecolor(RGB(180, 165, 130));
            fillcircle(pX + 2, pY + 3, 25);

            // 画棋子本体的外圈 + 内圈
            if (is_selected) // 选中时淡黄高亮 + 红色边框
            {
                setlinecolor(RGB(200, 50, 50));
                setlinestyle(PS_SOLID, 2);
                setfillcolor(RGB(255, 255, 150));
                fillcircle(pX, pY, 25);
                setlinestyle(PS_SOLID, 1);
                setfillcolor(RGB(255, 255, 200));
                fillcircle(pX, pY, 21);
            }
            else
            {
                setlinecolor(BLACK);
                setlinestyle(PS_SOLID, 2);
                // 外圈（深色）
                if (TemP.color == _RED)
                    setfillcolor(RGB(220, 120, 80));
                else
                    setfillcolor(RGB(200, 150, 55));
                fillcircle(pX, pY, 25);
                setlinestyle(PS_SOLID, 1);
                // 内圈（亮色）
                if (TemP.color == _RED)
                    setfillcolor(RGB(234, 149, 101));
                else
                    setfillcolor(RGB(237, 169, 68));
                fillcircle(pX, pY, 21);
            }
            // 棋子文字
            settextstyle(35, 0, "楷体");
            setbkmode(TRANSPARENT);
            settextcolor(TemP.color == _RED ? RED : BLACK);
            const char *name = piece_name[TemP.id];
            int tw = textwidth(name);
            int th = textheight(name);
            outtextxy(pX - tw / 2, pY - th / 2, name);
        }
    }

    // 标出可行的走棋点
    if (state.phase == SELECT_SECOND && state.selected_x != -1) // != -1 防bug(?)
    {
        std::vector<Move> moves = get_legal_moves(state, state.selected_x, state.selected_y);

        setfillcolor(RGB(50, 180, 80));
        setlinecolor(RGB(30, 140, 60));
        setlinestyle(PS_SOLID, 1);
        for (const Move &m : moves) // 遍历所有合法moves
        {
            if (state.board[m.to_y][m.to_x].id != SPACE)
            {
                // 可吃子位置画红色圈
                setlinecolor(RGB(220, 50, 50));
                setlinestyle(PS_SOLID, 2);
                circle(toX(m.to_x), toY(m.to_y), 26);
                // 恢复设置，避免影响后续绿点
                setlinecolor(RGB(30, 140, 60));
                setlinestyle(PS_SOLID, 1);
            }
            else
            {
                fillcircle(toX(m.to_x), toY(m.to_y), 6);
            }
        }
    }

    // 右侧 UI // OPTIMIZED: 根据联机角色显示不同按钮

    setfillcolor(RGB(220, 200, 160));
    setlinecolor(BLACK);
    fillrectangle(UI_PANEL_X, BOARD_TOP, UI_PANEL_X + UI_BUTTON_W + 20, toY(9));

    settextstyle(22, 0, "楷体");
    setbkmode(TRANSPARENT);

    if (is_network)
    {
        // 联机模式：根据角色排列按钮
        int top_btn_y, bot_btn_y;
        const char *top_label, *bot_label;
        COLORREF top_color, bot_color;

        if (!is_host) // BLACK: 认输(上) 悔棋(中) 空白(下)
        {
            top_btn_y = UI_LOSE_B_Y;
            top_label = "认  输";
            top_color = RGB(100, 100, 100);
            bot_btn_y = UI_LOSE_R_Y;
            bot_label = NULL;
            bot_color = RGB(220, 200, 160);
        }
        else // ROLE_HOST (RED): 空白(上) 悔棋(中) 认输(下)
        {
            top_btn_y = UI_LOSE_B_Y;
            top_label = NULL;
            top_color = RGB(220, 200, 160);
            bot_btn_y = UI_LOSE_R_Y;
            bot_label = "认  输";
            bot_color = RGB(200, 100, 80);
        }

        // 上方按钮/空白
        if (top_label)
        {
            setfillcolor(top_color);
            fillrectangle(UI_BTN_X, top_btn_y, UI_BTN_X + UI_BUTTON_W, top_btn_y + UI_BUTTON_H);
            settextcolor(WHITE);
            outtextxy(UI_BTN_X + 30, top_btn_y + 10, top_label);
        }
        // 悔棋（中间）
        setfillcolor(RGB(160, 140, 100));
        fillrectangle(UI_BTN_X, UI_UNDO_Y, UI_BTN_X + UI_BUTTON_W, UI_UNDO_Y + UI_BUTTON_H);
        settextcolor(BLACK);
        outtextxy(UI_BTN_X + 40, UI_UNDO_Y + 10, "悔  棋");
        // 下方按钮/空白
        if (bot_label)
        {
            setfillcolor(bot_color);
            fillrectangle(UI_BTN_X, bot_btn_y, UI_BTN_X + UI_BUTTON_W, bot_btn_y + UI_BUTTON_H);
            settextcolor(WHITE);
            outtextxy(UI_BTN_X + 30, bot_btn_y + 10, bot_label);
        }

        // 悔棋请求弹窗 // OPTIMIZED: 对方请求悔棋时显示同意/拒绝按钮
        if (state.undo_req_pending)
        {
            // 主机在上方 (默认)，客机在下方，间距对称
            int gap = UI_UNDO_Y - (UI_UNDO_REQ_Y + UI_REQ_BTN_H + 30);
            int req_y = is_host ? UI_UNDO_REQ_Y : UI_UNDO_Y + UI_BUTTON_H + gap + 5;

            setfillcolor(RGB(240, 220, 180));
            setlinecolor(BLACK);
            fillrectangle(UI_BTN_X - 5, req_y - 5, UI_BTN_X + UI_BUTTON_W + 5, req_y + UI_REQ_BTN_H + 30);
            settextstyle(18, 0, "楷体");
            setbkmode(TRANSPARENT);
            settextcolor(BLACK);
            outtextxy(UI_BTN_X + 15, req_y, "对方请求悔棋");
            // 同意
            setfillcolor(RGB(100, 180, 100));
            fillrectangle(UI_ACCEPT_X, req_y + 22, UI_ACCEPT_X + UI_REQ_BTN_W, req_y + 22 + UI_REQ_BTN_H);
            settextcolor(WHITE);
            outtextxy(UI_ACCEPT_X + 12, req_y + 25, "同意");
            // 拒绝
            setfillcolor(RGB(200, 100, 100));
            fillrectangle(UI_REJECT_X, req_y + 22, UI_REJECT_X + UI_REQ_BTN_W, req_y + 22 + UI_REQ_BTN_H);
            settextcolor(WHITE);
            outtextxy(UI_REJECT_X + 12, req_y + 25, "拒绝");
        }
    }
    else
    {
        // 单机模式：原始三按钮
        // 黑认输
        setfillcolor(RGB(100, 100, 100));
        fillrectangle(UI_BTN_X, UI_LOSE_B_Y,
                      UI_BTN_X + UI_BUTTON_W, UI_LOSE_B_Y + UI_BUTTON_H);
        settextcolor(WHITE);
        outtextxy(UI_BTN_X + 30, UI_LOSE_B_Y + 10, "黑方认输");
        // 悔棋
        setfillcolor(RGB(160, 140, 100));
        fillrectangle(UI_BTN_X, UI_UNDO_Y,
                      UI_BTN_X + UI_BUTTON_W, UI_UNDO_Y + UI_BUTTON_H);
        settextcolor(BLACK);
        outtextxy(UI_BTN_X + 40, UI_UNDO_Y + 10, "悔  棋");
        // 红认输
        setfillcolor(RGB(200, 120, 100));
        fillrectangle(UI_BTN_X, UI_LOSE_R_Y,
                      UI_BTN_X + UI_BUTTON_W, UI_LOSE_R_Y + UI_BUTTON_H);
        settextcolor(WHITE);
        outtextxy(UI_BTN_X + 30, UI_LOSE_R_Y + 10, "红方认输");
    }
}

// 结算
void show_result(GameState &state)
{
    const char *msg = "平局";
    settextcolor(BLACK);
    settextstyle(36, 0, "楷体");
    setbkmode(TRANSPARENT);

    if (state.winner == _RED)
    {
        settextcolor(RED);
        msg = "红方胜利！";
    }
    else if (state.winner == _BLACK)
    {
        settextcolor(BLACK);
        msg = "黑方胜利！";
    }

    int tw = textwidth(msg);
    int th = textheight(msg);
    int bx = (getwidth() - tw - 200) / 2 - 20;
    int by = (getheight() - th - 20) / 2 - 10;

    // BKG
    setfillcolor(WHITE);
    setlinecolor(WHITE);
    fillrectangle(bx, by, bx + tw + 40, by + th + 30);

    // 文字
    settextcolor(state.winner == _RED ? RED : BLACK);
    outtextxy(bx + 25, by + 15, msg);
}
