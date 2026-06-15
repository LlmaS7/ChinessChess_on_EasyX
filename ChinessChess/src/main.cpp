#include "../include/draw_board.h"
#include "../include/menu.h"
#include "../include/chess_engine.h"
#include "../include/mouse_input.h"
#include "../include/game_manager.h"
#include "../network/network.h"
#include <graphics.h>

int main()
{
    int menu_choice = show_start_menu();

    // 联机模式：尝试连接，失败则退出
    if ((menu_choice == MENU_HOST || menu_choice == MENU_CLIENT) && !net_startup(menu_choice))
        return 0;

    // 进入游戏
    initgraph(800, 640);
    GameState state;
    init_game(state);
    bool is_network = (menu_choice == MENU_HOST || menu_choice == MENU_CLIENT);
    bool is_host = (menu_choice == MENU_HOST);

    while (!state.game_over)
    {
        if (MouseHit())
        {
            MOUSEMSG m = GetMouseMsg();
            if (m.uMsg == WM_LBUTTONDOWN)
            {
                if (m.x >= UI_BTN_X && m.x <= UI_BTN_X + UI_BUTTON_W)
                {
                    if (handle_control_click(state, m.x, m.y, is_network, is_host))
                        check_over(state);
                }
                else if (is_my_turn(menu_choice, state.current_side) && !net_get_state().waiting_undo && !state.undo_req_pending)
                {
                    Move moves;
                    if (handle_mouse_click(state, m.x, m.y, moves))
                    {
                        if (is_network)
                            net_send_move(moves);
                        check_over(state);
                    }
                }
            }
        }

        if (is_network)
            handle_network_msg(state, menu_choice);

        ExMessage em;
        while (peekmessage(&em, EX_WINDOW))
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

        BeginBatchDraw();
        draw_board(state, is_network, is_host);
        EndBatchDraw();
    }

    game_over_sequence(state, is_network, is_host);
    return 0;
}
