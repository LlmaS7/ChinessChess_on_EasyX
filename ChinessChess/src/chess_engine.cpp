#include "chess_engine.h"

void init_game(GameState &state)
{
    // 清空
    for (int r = 0; r < ROW; r++)
        for (int c = 0; c < COL; c++)
            state.board[r][c] = {SPACE, _NONE};

    // 黑方底线
    int black_back[] = {車, 馬, 象, 士, 将, 士, 象, 馬, 車};
    for (int c = 0; c < COL; c++)
        state.board[0][c] = {black_back[c], _BLACK};
    // 黑炮
    state.board[2][1] = {砲, _BLACK};
    state.board[2][7] = {砲, _BLACK};
    // 黑卒
    int black_pawns[] = {0, 2, 4, 6, 8};
    for (int i = 0; i < 5; i++)
        state.board[3][black_pawns[i]] = {卒, _BLACK};

    // 红方底线
    int red_back[] = {俥, 马, 相, 仕, 帥, 仕, 相, 马, 俥};
    for (int c = 0; c < COL; c++)
        state.board[9][c] = {red_back[c], _RED};
    // 红炮
    state.board[7][1] = {炮, _RED};
    state.board[7][7] = {炮, _RED};
    // 红兵
    int red_pawns[] = {0, 2, 4, 6, 8};
    for (int i = 0; i < 5; i++)
        state.board[6][red_pawns[i]] = {兵, _RED};

    // 初始化状态
    state.current_side = _RED;
    state.turn_count = 1;
    state.phase = SELECT_FIRST;
    state.selected_x = -1;
    state.selected_y = -1;
    state.game_over = false;
    state.winner = -1;
    state.history.clear();
    state.undo_req_pending = false;
}

bool apply_move(GameState &state, int fX, int fY, int tX, int tY)
{

    ChessPiece &piece = state.board[fY][fX];
    if (piece.id == SPACE)
        return false; // 选空处
    if (piece.color != state.current_side)
        return false; // 选不是自己的子
    if (tX < 0 || tX >= COL || tY < 0 || tY >= ROW)
        return false; // 出界; 注意COL=9, x from 0 to 8
    if (fX == tX && fY == tY)
        return false; // 原地走

    ChessPiece &target = state.board[tY][tX];
    if (target.color == piece.color)
        return false; // 落点出不是自己的子

    // 走棋合法性判断（switch）
    switch (piece.id)
    {
    case 車:
    case 俥:
    {
        if (fX != tX && fY != tY)
            return false; // 必须走直线
        if (fX == tX)
        { // 检测是否被挡
            int step = (tY > fY) ? 1 : -1;
            for (int r = fY + step; r != tY; r += step)
                if (state.board[r][fX].id != SPACE)
                    return false;
        }
        else
        {
            int step = (tX > fX) ? 1 : -1;
            for (int c = fX + step; c != tX; c += step)
                if (state.board[fY][c].id != SPACE)
                    return false;
        }
        break; // 合法则跳出，执行走棋，return true
    }
    case 馬:
    case 马:
    {
        // 走日， 别马脚
        int dx = tX - fX, dy = tY - fY;
        bool judge = false;
        if (dx == 1 && dy == -2 && state.board[fY - 1][fX].id == SPACE)
            judge = true;
        if (dx == -1 && dy == -2 && state.board[fY - 1][fX].id == SPACE)
            judge = true;
        if (dx == 2 && dy == -1 && state.board[fY][fX + 1].id == SPACE)
            judge = true;
        if (dx == 2 && dy == 1 && state.board[fY][fX + 1].id == SPACE)
            judge = true;
        if (dx == 1 && dy == 2 && state.board[fY + 1][fX].id == SPACE)
            judge = true;
        if (dx == -1 && dy == 2 && state.board[fY + 1][fX].id == SPACE)
            judge = true;
        if (dx == -2 && dy == -1 && state.board[fY][fX - 1].id == SPACE)
            judge = true;
        if (dx == -2 && dy == 1 && state.board[fY][fX - 1].id == SPACE)
            judge = true;
        if (!judge)
            return false;
        break;
    }
    case 砲:
    case 炮:
    {
        // 直线判定
        if (fX != tX && fY != tY)
            return false;
        // 数中间有多少棋子
        int block = 0;
        if (fX == tX)
        {
            int step = (tY > fY) ? 1 : -1;
            for (int r = fY + step; r != tY; r += step)
                if (state.board[r][fX].id != SPACE)
                    block++;
        }
        else
        {
            int step = (tX > fX) ? 1 : -1;
            for (int c = fX + step; c != tX; c += step)
                if (state.board[fY][c].id != SPACE)
                    block++;
        }

        if (target.id == SPACE) // 目标点为空
        {
            if (block != 0) // 有间隔的棋子
                return false;
        }
        else // 目标点有子
        {
            if (block != 1) // 不满足间隔一个
                return false;
        }
        break;
    }
    case 象:
    case 相:
    {
        // 走田子
        int dx = tX - fX, dy = tY - fY;
        if (dx != 2 && dx != -2)
            return false;
        if (dy != 2 && dy != -2)
            return false;

        // 检测象眼
        if (state.board[fY + dy / 2][fX + dx / 2].id != SPACE)
            return false;

        // 不过河 黑 0-4，红 5-9
        if (piece.color == _BLACK && tY > 4)
            return false;
        if (piece.color == _RED && tY < 5)
            return false;

        break;
    }
    case 士:
    case 仕:
    {
        // 斜走 一步
        int dx = tX - fX, dy = tY - fY;
        if (dx != 1 && dx != -1)
            return false;
        if (dy != 1 && dy != -1)
            return false;

        // 不出九宫 col 3-5, black 0-2, red 7-9
        if (tX < 3 || tX > 5)
            return false;
        if (piece.color == _BLACK && (tY < 0 || tY > 2))
            return false;
        if (piece.color == _RED && (tY < 7 || tY > 9))
            return false;

        break;
    }
    case 将:
    case 帥:
    {
        // 直走 一步: dx==0 y==+-1 or dy==0 x==+-1
        int dx = tX - fX, dy = tY - fY;
        if ((dx == 0 && dy == 0) || (dx != 0 && dy != 0))
            return false;
        if (dx < -1 || dx > 1 || dy < -1 || dy > 1)
            return false;

        // 不出九宫 同士
        if (tX < 3 || tX > 5)
            return false;
        if (piece.color == _BLACK && (tY < 0 || tY > 2))
            return false;
        if (piece.color == _RED && (tY < 7 || tY > 9))
            return false;

        // 检查对将
        // 暂存原位置
        ChessPiece saved_src = state.board[fY][fX];
        ChessPiece saved_dst = state.board[tY][tX];
        // 模拟移动
        state.board[tY][tX] = saved_src;
        state.board[fY][fX] = {SPACE, _NONE};

        // 找到双方将的位置
        int bKingX = -1, bKingY = -1, rKingX = -1, rKingY = -1;
        for (int r = 0; r < ROW; r++)
            for (int c = 0; c < COL; c++)
            {
                if (state.board[r][c].id == 将)
                {
                    bKingX = c;
                    bKingY = r;
                }
                if (state.board[r][c].id == 帥)
                {
                    rKingX = c;
                    rKingY = r;
                }
            }

        bool face_to_face = false;
        if (bKingX >= 0 && rKingX >= 0 && bKingX == rKingX) // 同列
        {
            // 检查之间是否全空
            face_to_face = true;
            for (int r = bKingY + 1; r < rKingY; r++)
                if (state.board[r][bKingX].id != SPACE)
                {
                    face_to_face = false;
                    break;
                }
        }
        // 恢复原位置
        state.board[fY][fX] = saved_src;
        state.board[tY][tX] = saved_dst;

        if (face_to_face)
            return false;

        break;
    }
    case 卒:
    case 兵:
    {
        // 只能走一步，前进或横着走
        int dx = tX - fX, dy = tY - fY;
        if (dx == 0 && dy == 0)
            return false;
        if (dx != 0 && dy != 0)
            return false;
        if (dx < -1 || dx > 1 || dy < -1 || dy > 1)
            return false;

        // 不能后退
        if (piece.color == _BLACK && dy < 0)
            return false;
        if (piece.color == _RED && dy > 0)
            return false;

        // 未过河不能横走
        bool crosseRiver = (piece.color == _BLACK && fY >= 5) || (piece.color == _RED && fY <= 4);
        if (!crosseRiver && dx != 0)
            return false;

        break;
    }
    default:
        return false;
    }

    // 需保证走后不能自己被将, 模拟走棋进行检查
    ChessPiece saved_piece = piece;
    ChessPiece saved_target = target;
    target = piece;
    piece = {SPACE, _NONE};
    bool in_check = is_in_check(state, saved_piece.color);
    piece = saved_piece; // 注意！！ 先还原再return，否则函数退出时没有执行还原
    target = saved_target;
    if (in_check)
        return false; // ！！！！如果会被将，则该走法不合法，否定

    // 执行走棋
    Move move;
    move.from_x = fX;
    move.from_y = fY;
    move.to_x = tX;
    move.to_y = tY;
    move.moved_id = piece.id;
    move.moved_color = piece.color;
    move.captured_id = target.id;
    move.captured_color = target.color;

    // 移动棋子
    target = piece;
    piece = {SPACE, _NONE};
    // 本次move记入history
    state.history.push_back(move);

    state.current_side = (state.current_side == _RED) ? _BLACK : _RED;
    state.turn_count++;

    return true;
}

bool undo_move(GameState &state)
{
    if (state.history.empty())
        return false;

    Move last = state.history.back();
    state.history.pop_back();

    // 恢复起点棋子
    state.board[last.from_y][last.from_x] = {last.moved_id, last.moved_color};
    // 恢复终点棋子
    state.board[last.to_y][last.to_x] = {last.captured_id, last.captured_color};

    // 更换下棋方， 回退回合数
    state.current_side = (state.current_side == _RED) ? _BLACK : _RED;
    state.turn_count--;

    return true;
}

bool is_attacked(const GameState &state, int tx, int ty, int attacker_color)
{ // 检测点（tx,ty）
    // 遍历所有color方棋子, 输入时要求保证color方棋子与目标点不同色
    for (int fy = 0; fy < ROW; fy++)
        for (int fx = 0; fx < COL; fx++)
        {
            const ChessPiece &p = state.board[fy][fx];
            if (p.id == SPACE || p.color != attacker_color)
                continue; // 空处或己方：跳过, 筛选出color方棋子

            int dx = tx - fx, dy = ty - fy;

            switch (p.id)
            {
            case 車:
            case 俥:
            {
                if (fx != tx && fy != ty)
                    break; // 不成立直接跳出，return false
                bool blocked = false;
                // 检测是否被挡
                if (fx == tx)
                {
                    int step = (ty > fy) ? 1 : -1;
                    for (int r = fy + step; r != ty; r += step)
                        if (state.board[r][fx].id != SPACE)
                        {
                            blocked = true;
                            break;
                        }
                }
                else
                {
                    int step = (tx > fx) ? 1 : -1;
                    for (int c = fx + step; c != tx; c += step)
                        if (state.board[fy][c].id != SPACE)
                        {
                            blocked = true;
                            break;
                        }
                }
                if (!blocked)
                    return true; // 成立 return true
                break;
            }
            case 馬:
            case 马:
            {
                bool ok = false;
                if (dx == 1 && dy == -2 && state.board[fy - 1][fx].id == SPACE)
                    ok = true;
                if (dx == -1 && dy == -2 && state.board[fy - 1][fx].id == SPACE)
                    ok = true;
                if (dx == 2 && dy == -1 && state.board[fy][fx + 1].id == SPACE)
                    ok = true;
                if (dx == 2 && dy == 1 && state.board[fy][fx + 1].id == SPACE)
                    ok = true;
                if (dx == 1 && dy == 2 && state.board[fy + 1][fx].id == SPACE)
                    ok = true;
                if (dx == -1 && dy == 2 && state.board[fy + 1][fx].id == SPACE)
                    ok = true;
                if (dx == -2 && dy == -1 && state.board[fy][fx - 1].id == SPACE)
                    ok = true;
                if (dx == -2 && dy == 1 && state.board[fy][fx - 1].id == SPACE)
                    ok = true;
                if (ok)
                    return true;
                break;
            }
            case 砲:
            case 炮:
            {
                if (fx != tx && fy != ty)
                    break;
                int block = 0;
                // 算间隔棋子数
                if (fx == tx)
                {
                    int step = (ty > fy) ? 1 : -1;
                    for (int r = fy + step; r != ty; r += step)
                        if (state.board[r][fx].id != SPACE)
                            block++;
                }
                else
                {
                    int step = (tx > fx) ? 1 : -1;
                    for (int c = fx + step; c != tx; c += step)
                        if (state.board[fy][c].id != SPACE)
                            block++;
                }
                // 已保证attacker与目标不同色 只要间隔一个，且目标有子则成立
                if (block == 1 && state.board[ty][tx].id != SPACE)
                    return true;
                break;
            }
            case 象:
            case 相:
            {
                if (dx != 2 && dx != -2)
                    break;
                if (dy != 2 && dy != -2)
                    break;
                if (state.board[fy + dy / 2][fx + dx / 2].id != SPACE) // 被堵
                    break;
                if (p.color == _BLACK && ty > 4)
                    break;
                if (p.color == _RED && ty < 5)
                    break;
                return true;
            }
            case 士:
            case 仕:
            {
                if (dx != 1 && dx != -1)
                    break;
                if (dy != 1 && dy != -1)
                    break;
                if (tx < 3 || tx > 5)
                    break;
                if (p.color == _BLACK && (ty < 0 || ty > 2))
                    break;
                if (p.color == _RED && (ty < 7 || ty > 9))
                    break;
                return true;
            }
            case 将:
            case 帥:
            {
                if (dx == 0 && dy == 0)
                    break;
                if (dx != 0 && dy != 0)
                    break;
                if (dx < -1 || dx > 1 || dy < -1 || dy > 1)
                    break;
                if (tx < 3 || tx > 5)
                    break;
                if (p.color == _BLACK && (ty < 0 || ty > 2))
                    break;
                if (p.color == _RED && (ty < 7 || ty > 9))
                    break;
                return true;
            }
            case 卒:
            case 兵:
            {
                if (dx == 0 && dy == 0)
                    break;
                if (dx != 0 && dy != 0)
                    break;
                if (dx < -1 || dx > 1 || dy < -1 || dy > 1)
                    break;
                if (p.color == _BLACK && dy < 0)
                    break;
                if (p.color == _RED && dy > 0)
                    break;
                bool crosseRiver = (p.color == _BLACK && fy >= 5) || (p.color == _RED && fy <= 4);
                if (!crosseRiver && dx != 0)
                    break;
                return true;
            }
            }
        }
    return false;
}

bool is_in_check(const GameState &state, int color)
{
    // 找到己方将/帅的位置
    int king_id = (color == _BLACK) ? 将 : 帥;
    for (int r = 0; r < ROW; r++)
        for (int c = 0; c < COL; c++)
            if (state.board[r][c].id == king_id)
            {
                int oppColor = (color == _RED) ? _BLACK : _RED;
                // 普通子能否直接吃
                if (is_attacked(state, c, r, oppColor))
                    return true;
                // 飞将检查
                int otherKing = (color == _RED) ? 将 : 帥;
                for (int rr = 0; rr < ROW; rr++)
                    for (int cc = 0; cc < COL; cc++)
                        if (state.board[rr][cc].id == otherKing)
                        {
                            if (cc == c)
                            {
                                bool blocked = false;
                                int top = (r < rr) ? r : rr;
                                int bot = (r > rr) ? r : rr;
                                for (int mid = top + 1; mid < bot; mid++)
                                    if (state.board[mid][c].id != SPACE)
                                    {
                                        blocked = true;
                                        break;
                                    }
                                if (!blocked)
                                    return true;
                            }
                        }
                return false;
            }
    return false; // 找不到将/帅 （不该发生）
}

std::vector<Move> get_legal_moves(GameState &state, int fx, int fy)
{
    std::vector<Move> moves;                 // 要获得所有的合法走法，所以使用vector
    ChessPiece &piece = state.board[fy][fx]; // 将 fx,fy 点的状态赋给piece
    if (piece.id == SPACE)
        return moves;

    // 检测能否走到 tx,ty
    for (int ty = 0; ty < ROW; ty++)
        for (int tx = 0; tx < COL; tx++)
        {
            if (!apply_move(state, fx, fy, tx, ty))
                continue; // 走不到该点
            // ！！apply_move时换边

            // aply_move内已经进行is_in_check，这里不能再进行了
            // if (is_in_check(state, move_color))
            // {
            //     undo_move(state);
            //     continue; // 走后被将
            // }

            undo_move(state); // 回应apply_move的模拟走棋
            // 构造用于 return 的 Move
            Move m;
            m.from_x = fx;
            m.from_y = fy;
            m.to_x = tx;
            m.to_y = ty;
            m.moved_id = piece.id;
            m.moved_color = piece.color;
            m.captured_id = state.board[ty][tx].id;
            m.captured_color = state.board[ty][tx].color;
            moves.push_back(m);
        }
    return moves;
}

int check_over(GameState &state)
{
    for (int fy = 0; fy < ROW; fy++)
        for (int fx = 0; fx < COL; fx++)
        {
            const ChessPiece &p = state.board[fy][fx];
            if (p.id == SPACE || p.color != state.current_side)
                continue;

            // get_legal_moves内部会调用apply_move, 其中已经否定了会被将的走法
            std::vector<Move> moves = get_legal_moves(state, fx, fy);
            if (!moves.empty())
                return -1; // 还有走法，游戏继续
        }
    // 无合法走法： 当前方输，对方胜
    state.game_over = true;
    state.winner = (state.current_side == _RED) ? _BLACK : _RED;
    return state.winner;
}

void undo_to_turn(GameState &state, int target_color)
{
    // 当前回合方已经是 target_color 时退2步，否则退1步
    if (state.current_side != target_color)
        undo_move(state);
    else
    {
        undo_move(state);
        undo_move(state);
    }
}
