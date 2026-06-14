#include "../include/draw_board.h"
#include "../include/menu.h"
#include "../include/chess_engine.h"
#include "../include/mouse_input.h"
#include "../include/game_control.h"
#include "../network/network.h"
#include <graphics.h>

// 判断当前是否为联机对局
static bool is_network_game(int menu_choice)
{
    return menu_choice == MENU_HOST || menu_choice == MENU_CLIENT;
}

// 判断当前回合是否轮到本地玩家操作
static bool is_my_turn(int menu_choice, int current_side)
{
    if (menu_choice == MENU_HOST)
        return current_side == _RED; // 主机执红
    if (menu_choice == MENU_CLIENT)
        return current_side == _BLACK; // 客机执黑
    return true;                       // 单机模式总是本地操作
}

// 显示一个简单的等待窗口
static void show_wait_window(const char *msg)
{
    initgraph(400, 200);
    setbkcolor(RGB(220, 180, 130));
    cleardevice();
    settextstyle(24, 0, "楷体");
    setbkmode(TRANSPARENT);
    settextcolor(BLACK);
    int tw = textwidth(msg);
    outtextxy((400 - tw) / 2, 80, msg);
}

// 处理一条网络消息，返回 true 表示消息已处理
static bool process_network_msg(GameState &state, int menu_choice)
{
    NetMessage msg;
    if (!net_recv_msg_nonblock(msg))
        return false;

    switch (msg.type)
    {
    case NET_MOVE:
        if (!is_my_turn(menu_choice, state.current_side))
        {
            Move move;
            net_unpack_move(msg, move);
            apply_move(state, move.from_x, move.from_y, move.to_x, move.to_y);
            check_over(state);
        }
        break;
    case NET_UNDO_REQ:
        state.undo_req_pending = true;
        break;
    case NET_UNDO_ACCEPT:
    {
        int my_color = (menu_choice == MENU_HOST) ? _RED : _BLACK;
        undo_to_turn(state, my_color);
        net_set_waiting_undo(false);
        state.undo_req_pending = false;
        break;
    }
    case NET_UNDO_REJECT:
        net_set_waiting_undo(false);
        state.undo_req_pending = false;
        break;
    case NET_RESIGN:
        state.game_over = true;
        state.winner = (menu_choice == MENU_HOST) ? _RED : _BLACK;
        break;
    case NET_DISCONNECT:
        state.game_over = true;
        state.winner = (menu_choice == MENU_HOST) ? _RED : _BLACK;
        break;
    }
    return true;
}

// 启动网络连接（主机等待客机 / 客机连接主机）。失败返回 false
static bool net_startup(int menu_choice)
{
    if (!net_init())
    {
        printf("[main] 网络初始化失败\n");
        return false;
    }

    if (menu_choice == MENU_HOST)
    {
        char buf[128];
        sprintf(buf, "本机IP：%s\n\n正在等待对方连接...", net_get_local_ip());
        initgraph(400, 220);
        setbkcolor(RGB(220, 180, 130));
        cleardevice();
        settextstyle(20, 0, "楷体");
        setbkmode(TRANSPARENT);
        settextcolor(BLACK);
        outtextxy(40, 60, buf);
        outtextxy(40, 120, "请将此 IP 输入到连接窗口");

        net_host(8888);
        closegraph();
    }
    else
    {
        show_wait_window("正在连接主机...");
        const char *ip = get_connect_ip();
        if (!net_connect(ip, 8888))
        {
            closegraph();
            net_cleanup();
            printf("[main] 连接失败\n");
            return false;
        }
        closegraph();
    }

    if (!net_get_state().connected)
    {
        printf("[main] 未检测到客机连接\n");
        net_cleanup();
        return false;
    }
    return true;
}

// 游戏结算：显示结果，等待按键，清理资源
static void game_over_sequence(GameState &state, bool is_network, bool is_host)
{
    if (is_network)
        net_send_game_over(state.winner);

    draw_board(state, is_network, is_host);
    Sleep(500);
    show_result(state);

    ExMessage em;
    while (true)
    {
        peekmessage(&em, EX_KEY);
        if (em.message == WM_KEYDOWN)
            break;
    }

    if (is_network)
        net_cleanup();
    closegraph();
}

int main()
{
    int menu_choice = show_start_menu();

    // 联机模式：启动网络连接
    if (is_network_game(menu_choice) && !net_startup(menu_choice))
        return 0;

    // 进入游戏
    initgraph(800, 640);
    GameState state;
    init_game(state);
    bool is_network = is_network_game(menu_choice);
    bool is_host = (menu_choice == MENU_HOST);

    while (!state.game_over)
    {
        // OPTIMIZED: UI按钮（悔棋/认输）在任何回合都可点击，不限于本地回合
        if (MouseHit())
        {
            MOUSEMSG m = GetMouseMsg();
            if (m.uMsg == WM_LBUTTONDOWN)
            {
                if (m.x >= UI_BTN_X && m.x <= UI_BTN_X + UI_BUTTON_W)
                {
                    // UI 按钮（悔棋/认输）
                    if (handle_control_click(state, m.x, m.y, is_network, is_host))
                        check_over(state);
                }
                // OPTIMIZED: 桌面走棋仅在本地回合且未等待悔棋响应时
                else if (is_my_turn(menu_choice, state.current_side) && !net_get_state().waiting_undo && !state.undo_req_pending)
                {
                    Move moves;
                    if (handle_mouse_click(state, m.x, m.y, moves))
                    {
                        // 成功走棋->通过网络发送给对方
                        if (is_network_game(menu_choice))
                            net_send_move(moves);
                        check_over(state);
                    }
                }
            }
        }

        // OPTIMIZED: 始终接收网络消息（悔棋请求可能在任何时刻到达）
        if (is_network)
            process_network_msg(state, menu_choice);

        // 检测关闭窗口
        ExMessage em;
        while (peekmessage(&em, EX_WINDOW))
        {
            if (em.message == WM_CLOSE)
            {
                if (is_network)
                {
                    net_send_game_over(-1);
                    net_cleanup();
                }
                closegraph();
                return 0;
            }
        }

        // 渲染
        BeginBatchDraw();
        draw_board(state, is_network, is_host);
        EndBatchDraw();
    }

    // 结算
    game_over_sequence(state, is_network, is_host);
    return 0;
}
