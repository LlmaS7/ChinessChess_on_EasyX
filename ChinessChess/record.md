## 杂项
- 项目完成时，删除这个项目中产生的与这个项目有关的临时文件，memory等。如
> c:\Users\25489\AppData\Roaming\Code\User\workspaceStorage\31afc0345b8ab84dfcf3cf14fbb8132a\GitHub.  copilot-chat\memory-tool\memories\repo\cleanup.md
> C:\Users\25489\AppData\Roaming\Code\User\globalStorage\github.copilot-chat\memory-tool\memories\chiness-chess-role.md

## 棋盘
>col: 0   1   2   3   4   5   6   7   8
>row0:車  馬  象  士   将  士   象  馬  車
>row1: 
>row2: 　　砲　　　    　　　　      砲
>row3:卒　    卒　     卒　     卒　    卒
>row4:
>row5:
>row6:兵　    兵　     兵    　 兵　    兵
>row7: 　　炮　　　　　　　　　       炮
>row8: 　　　　　　　　　　　　　　　　　　　　
>row9:俥  马  相  仕   帥  仕   相  马  俥    
## 问题
- 过程中出现的小问题：

    - 冗余代码：先前规划阶段没规划好，定义结构体等时多余定义过多的无用状态
    > 如： game_state.h 中 定义的 struct ChinessPiece ， 预先定义了坐标int x，y （后发现无用，可由GameState负责记录坐标）， 定义了状态是否过河 river（后发现无用，有关卒是否过河，直接检测坐标即可）

    - 规划问题：“对将”检查怎么实现，在哪实现的问题
    - 常量与工具函数的定义位置：画棋盘用的常量和工具函数最开始放在draw_board.cpp里，后由于mouse_input也要用，移到chess_def里
    - 重定义： EasyX 的 graphics.h 中已定义了 RED 与BLACK ， 而代码中 enum Color: RED and BLACK ， 导致无法正产运行。 修改方案： 将enum Color中改为 {_NONE=-1, _RED, _BLACK} 再逐个修改
    - 程序规划： 不显示走位提示： （get_legal_moves 中的问题：） get_legal_moves() 里调用的apply_moves ，其中已经 is_in_check 了。在 get_legal_moves 里面再进行 is_in_check 就会出问题。去掉get_legal_moves里面的is_in_check就没问题了


## 过程

1. chess_def:
    - *Piece, Color, Phase, Move*
    
2. game_state: 
    - *Chesspiece, GameState*

3. draw_board.h, draw_board.cpp:
    - *draw_board*: 棋盘+棋子+底部栏
    - *show_result*: 结算文字
    - *show_start_menu*: 开始界面

4. chess_enging.h, chess_enging.cpp:
    - *init_game*: void, 初始化每个点（棋子位置与空处），初始化游戏各项状态

    - *apply_move*: bool, 合法性判断（过程否定非法走棋，合法则跳出，执行走棋，return true）
        0. 否定非法选/走
        1. 车： 直线，被挡
        2. 马： 走日，被别马脚
        3. 炮： 直线，间隔棋子个数，落点处棋子状况
        4. 象： 走田，象眼，不过河
        5. 士： 斜走，不出九宫
        6. 将： 直走，一格，不出九宫，不能对将（模拟移动检查）
        7. 卒： 只走一步，不后退，未过河不横走
        8. 检查，保证走棋后不能自己被将军
        9. 执行走棋，move记入history

    - *undo_move*: bool, 恢复始末点棋子，变更游戏状态

    - *is_attacked*: bool, 针对点 (x,y) 遍历棋盘上color方棋子（要求与目标不同色），判断是否能攻击到该点（注意：“对将”的检查放在了 将 的走法里） （实现方法类似apply_move）

    - *is_in_check*: bool, 判断color方是否 "正在" 被将军： 找到帅/将后，is_attacked检查普通子能否吃，再进行飞将检查

    - *get_legal_moves*: vector Move，获取 (fx,fy) 处棋子 "所有的" 合法走法： 遍历所有棋子之下，再遍历所有点检查是否能走到，记下所有合法moves追加到vector后并返回

    - *check_over*: int, 将死判定，返回胜方颜色，未结束为 -1: 当一方所有子都没有合法走法时，判负（**有关多个函数的联动：使用的get_legal_moves内部会调用apply_move, 其中已经否定了会被将的走法。所以当一方被将死时，其他子也都动不了。故直接判负**）

5. mouse_input.h, mouse_input.cpp:
    - *handle_mouse_click*: bool, 处理鼠标点击。鼠标像素坐标转逻辑坐标。根据状态：FIRST（选棋）：选己方；SECOND（走棋）：{取消选中，退回FIRST}/{切换选中}/{验证可行性，非法退回FIRST，合法执行并退回FIRST}
6. ui_panel.h, ui_panel.cpp: 
    - *handle_ui_click*: bool, 若点击ui部位：点击黑方认输/悔棋/红方认输
