
#define DECLSPEC_EXPORT __declspec(dllexport)
#define WINAPI __stdcall

#include <ctime>
#include "tetris_core.h"
#include "land_point_search_simple.h"
#include "land_point_search_path.h"
#include "land_point_search_cautious.h"
#include "ai_ax.h"
#include "ai_zzz.h"
#include "rule_st.h"
#include "rule_qq.h"
#include "rule_srs.h"
#include "random.h"

m_tetris::TetrisEngine<rule_st::TetrisRuleSet, ai_ax_1::AI, land_point_search_path::Search, 3> tetris_ai;

extern "C" void attach_init()
{
    ege::mtsrand(unsigned int(time(nullptr)));
}

//返回AI名字，会显示在界面上
extern "C" DECLSPEC_EXPORT char const *WINAPI Name()
{
    static std::string name = tetris_ai.ai_name();
    return name.c_str();
}

//数据转换
void build_map(char board[], int w, int h, m_tetris::TetrisMap &map)
{
    memset(&map, 0, sizeof map);
    map.width = w;
    map.height = h;
    for(int y = 0, add = 0; y < h; ++y, add += w)
    {
        for(int x = 0; x < w; ++x)
        {
            if(board[x + add] == '1')
            {
                map.top[x] = map.roof = y + 1;
                map.row[y] |= 1 << x;
                ++map.count;
            }
        }
    }
}

/*
 * board是一个boardW*boardH长度用01组成的字符串，原点于左下角，先行后列。
 * 例如8*3的场地实际形状：
 * 00000000
 * 00011001
 * 01111111
 * 则参数board的内容为："011111110001100100000000"。
 *
 * Piece参数使用字符 OISZLJT 及空格表示方块形状。
 * nextPiece为' '时表示无预览。
 * curR是方向朝向，1是初始方向，2是逆时针90度，3是180度，4是顺时针90度。
 * curX,curY的坐标，是以当前块4*4矩阵的上数第二行，右数第二列为基准，
 *     左下角为x=1,y=1；右下角为x=boardW,y=1；左上角为x=1,y=boardH
 *     具体方块形状参阅上一级目录下的pieces_orientations.jpg
 *
 * bestX,bestRotation 用于返回最优位置，与curX,curR的规则相同。
 *
 * 注意：方块操作次序规定为先旋转，再平移，最后放下。
 *       若中间有阻挡而AI程序没有判断则会导致错误摆放。
 *       该函数在出现新方块的时候被调用，一个方块调用一次。
 */
extern "C" DECLSPEC_EXPORT int WINAPI AI(int boardW, int boardH, char board[], char curPiece, int curX, int curY, int curR, char nextPiece, int *bestX, int *bestRotation)
{
    if(!tetris_ai.prepare(boardW, boardH))
    {
        return 0;
    }
    m_tetris::TetrisMap map;
    m_tetris::TetrisBlockStatus status =
    {
        curPiece, curX - 1, curY - 1, curR - 1
    };
    size_t next_length = 0;
    unsigned char next[] = {nextPiece, ' ', ' '};
    build_map(board, boardW, boardH, map);
    /////////////////////////////////////////////////
    //这里计算空闲方块数,局势比较差开启vp计算
    int free_block = 0;
    for(int x = 0; x < map.width; ++x)
    {
        free_block += map.height - map.top[x];
    }
    if(free_block < map.width * 6)
    {
        next_length = 2;
    }
    else if(nextPiece != ' ' || free_block < map.width * 10)
    {
        next_length = 1;
    }
    /////////////////////////////////////////////////
    auto result = tetris_ai.run(map, tetris_ai.get(status), next, next_length);

    if(result != nullptr)
    {
        *bestX = result->status.x + 1;
        *bestRotation = result->status.r + 1;
    }
    return 0;
}

/*
 * path 用于接收操作过程并返回，操作字符集：
 *      'l': 左移一格
 *      'r': 右移一格
 *      'd': 下移一格
 *      'L': 左移到头
 *      'R': 右移到头
 *      'D': 下移到底（但不粘上，可继续移动）
 *      'z': 逆时针旋转
 *      'c': 顺时针旋转
 * 字符串末尾要加'\0'，表示落地操作（或硬降落）
 *
 * 本函数支持任意路径操作，若不需要此函数只想使用上面一个的话，则删掉本函数即可
 */
extern "C" DECLSPEC_EXPORT int WINAPI AIPath(int boardW, int boardH, char board[], char curPiece, int curX, int curY, int curR, char nextPiece, char path[])
{
    if(!tetris_ai.prepare(boardW, boardH))
    {
        return 0;
    }
    m_tetris::TetrisMap map;
    m_tetris::TetrisBlockStatus status =
    {
        curPiece, curX - 1, curY - 1, curR - 1
    };
    size_t next_length = 0;
    unsigned char next[] = {nextPiece, ' ', ' '};
    build_map(board, boardW, boardH, map);
    /////////////////////////////////////////////////
    //这里计算空闲方块数,局势比较差开启vp计算
    int free_block = 0;
    for(int x = 0; x < map.width; ++x)
    {
        free_block += map.height - map.top[x];
    }
    if(free_block < map.width * 6)
    {
        next_length = 2;
    }
    else if(nextPiece != ' ' || free_block < map.width * 10)
    {
        next_length = 1;
    }
    /////////////////////////////////////////////////
    m_tetris::TetrisNode const *node = tetris_ai.get(status);
    auto result = tetris_ai.run(map, node, next, next_length);
    if(result != nullptr)
    {
        std::vector<char> ai_path = tetris_ai.path(node, result, map);
        memcpy(path, ai_path.data(), ai_path.size());
        path[ai_path.size()] = '\0';
    }
    return 0;
}


m_tetris::TetrisEngine<rule_srs::TetrisRuleSet, ai_zzz::qq::Attack, land_point_search_cautious::Search, 8> srs_ai;

extern "C" DECLSPEC_EXPORT int AIDllVersion()
{
    return 2;
}

extern "C" DECLSPEC_EXPORT char *AIName(int level)
{
    static char name[200];
    strcpy_s(name, srs_ai.ai_name().c_str());
    return name;
}

/*
all 'char' type is using the characters in ' ITLJZSO'

field data like this:
00........   -> 0x3
00.0......   -> 0xb
00000.....   -> 0x1f

b2b: the count of special attack, the first one set b2b=1, but no extra attack. Have extra attacks when b2b>=2
combo: first clear set combo=1, so the comboTable in toj rule is [0, 0, 0, 1, 1, 2, 2, 3, ...]
next: array size is 'maxDepth'
x, y, spin: the active piece's x/y/orientation,
x/y is the up-left corner's position of the active piece.
see tetris_gem.cpp for the bitmaps.
curCanHold: indicates whether you can use hold on current move.
might be caused by re-think after a hold move.
canhold: false if hold is completely disabled.
comboTable: -1 is the end of the table.
*/
extern "C" DECLSPEC_EXPORT char *TetrisAI(int overfield[], int field[], int field_w, int field_h, int b2b, int combo, char next[], char hold, bool curCanHold, char active, int x, int y, int spin, bool canhold, bool can180spin, int upcomeAtt, int comboTable[], int maxDepth, int level, int player)
{
    static char result_buffer[8][1024];
    char *result = result_buffer[player];
    result[0] = '\0';

    if(field_w != 10 || field_h != 22 || !srs_ai.prepare(10, 40))
    {
        return result;
    }
    m_tetris::TetrisMap map =
    {
        {}, {}, 10, 40
    };
    for(size_t d = 0, s = 22; d < 23; ++d, --s)
    {
        map.row[d] = field[s];
    }
    for(size_t d = 23, s = 0; s < 8; ++d, ++s)
    {
        map.row[d] = overfield[s];
    }
    for(int my = 0; my < map.height; ++my)
    {
        for(int mx = 0; mx < map.width; ++mx)
        {
            if(map.full(mx, my))
            {
                map.top[mx] = map.roof = my + 1;
                map.row[my] |= 1 << mx;
                ++map.count;
            }
        }
    }
    m_tetris::TetrisBlockStatus status =
    {
        active, x, 22 - y, (4 - spin) % 4
    };
    m_tetris::TetrisNode const *node = srs_ai.get(status);
    auto target = srs_ai.run(map, node, reinterpret_cast<unsigned char *>(next), maxDepth);
    if(target != nullptr)
    {
        std::vector<char> ai_path = srs_ai.path(node, target, map);
        memcpy(result, ai_path.data(), ai_path.size());
        result[ai_path.size()] = 'V';
        result[ai_path.size() + 1] = '\0';
    }
    return result;
}


#ifndef WINVER
#define WINVER 0x0500
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <ctime>

//这是一个加载dll测试数据的控制台,优先调用AIPath,找不到则调用AI
int wmain(unsigned int argc, wchar_t *argv[], wchar_t *eve[])
{
    attach_init();
    if(argc < 2)
    {
        return 0;
    }
    HMODULE hDll = LoadLibrary(argv[1]);
    if(hDll == nullptr)
    {
        return 0;
    }
    void *name = nullptr;
    name = GetProcAddress(hDll, "_Name@0");
    if(name == nullptr)
    {
        name = GetProcAddress(hDll, "Name@0");
    }
    if(name == nullptr)
    {
        name = GetProcAddress(hDll, "Name");
    }
    void *ai[2] = {};
    ai[0] = GetProcAddress(hDll, "_AIPath@36");
    if(ai[0] == NULL)
    {
        ai[0] = GetProcAddress(hDll, "AIPath@36");
    }
    if(ai[0] == NULL)
    {
        ai[0] = GetProcAddress(hDll, "AIPath");
    }
    ai[1] = GetProcAddress(hDll, "_AI@40");
    if(ai[1] == NULL)
    {
        ai[1] = GetProcAddress(hDll, "AI@40");
    }
    if(ai[1] == NULL)
    {
        ai[1] = GetProcAddress(hDll, "AI");
    }

    if(name == nullptr)
    {
        return 0;
    }
    int version = -1;
    for(int i = 0; i < sizeof ai / sizeof ai[0]; ++i)
    {
        if(ai[i] != nullptr)
        {
            version = i;
            break;
        }
    }
    if(version == -1)
    {
        return 0;
    }
    SetWindowTextA(GetConsoleWindow(), ((char const *(*)())name)());
    int w = 10, h = 20;
    m_tetris::TetrisMap map =
    {
        {}, {}, w, h
    };
    char *param_map = new char[w * h];
    char *path = new char[1024];
    tetris_ai.prepare(w, h);
    clock_t log_start = clock();
    clock_t log_time = log_start;
    clock_t log_new_time;

    clock_t log_interval = 10000;
    long long log_rows = 0, log_piece = 0;

    long long total_lines = 0;
    long long this_lines = 0;
    long long max_line = 0;
    long long game_count = 0;

    while(true)
    {
        m_tetris::TetrisNode const *node = tetris_ai.context()->generate();
        log_new_time = clock();
        if(log_new_time - log_time > log_interval)
        {
            printf("{\"time\":%.2lf,\"current\":%lld,\"rows_ps\":%lld,\"piece_ps\":%lld}\n", (log_new_time - log_start) / 1000., this_lines, log_rows * 1000 / log_interval, log_piece * 1000 / log_interval);
            log_time += log_interval;
            log_rows = 0;
            log_piece = 0;
        }
        if(!node->check(map))
        {
            total_lines += this_lines;
            if(this_lines > max_line)
            {
                max_line = this_lines;
            }
            ++game_count;
            printf("{\"avg\":%.2lf,\"max\":%lld,\"count\":%lld,\"current\":%lld}\n", game_count == 0 ? 0. : double(total_lines) / game_count, max_line, game_count, this_lines);
            this_lines = 0;
            map.count = 0;
            map.roof = 0;
            memset(map.top, 0, sizeof map.top);
            memset(map.row, 0, sizeof map.row);
        }
        for(int y = 0; y < h; ++y)
        {
            int row = y * w;
            for(int x = 0; x < w; ++x)
            {
                param_map[x + row] = map.full(x, y) ? '1' : '0';
            }
        }
        if(version == 0)
        {
            memset(path, 0, 1024);
            typedef int(__stdcall *ai_run_t)(int boardW, int boardH, char board[], char curPiece, int curX, int curY, int curR, char nextPiece, char path[]);
            ((ai_run_t)ai[version])(w, h, param_map, node->status.t, node->status.x + 1, node->status.y + 1, node->status.r, ' ', path);
            char *move = path, *move_end = path + 1024;
            //printf("%c->%s\n", node->status.t, path);
            while(move != move_end && *move != '\0')
            {
                switch(*move++)
                {
                case 'l':
                    if(node->move_left && (node->row >= map.roof || node->move_left->check(map)))
                    {
                        node = node->move_left;
                    }
                    break;
                case 'r':
                    if(node->move_right && (node->row >= map.roof || node->move_right->check(map)))
                    {
                        node = node->move_right;
                    }
                    break;
                case 'd':
                    if(node->move_down && (node->row > map.roof || node->move_down->check(map)))
                    {
                        node = node->move_down;
                    }
                    break;
                case 'L':
                    while(node->move_left && (node->row >= map.roof || node->move_left->check(map)))
                    {
                        node = node->move_left;
                    }
                    break;
                case 'R':
                    while(node->move_right && (node->row >= map.roof || node->move_right->check(map)))
                    {
                        node = node->move_right;
                    }
                    break;
                case 'D':
                    node = node->drop(map);
                    break;
                case 'z':
                    if(node->rotate_counterclockwise && node->rotate_counterclockwise->check(map))
                    {
                        node = node->rotate_counterclockwise;
                    }
                    break;
                case 'c':
                    if(node->rotate_clockwise && node->rotate_clockwise->check(map))
                    {
                        node = node->rotate_clockwise;
                    }
                    break;
                case 'x':
                    if(node->rotate_opposite && node->rotate_opposite->check(map))
                    {
                        node = node->rotate_opposite;
                    }
                    break;
                default:
                    move = move_end;
                    break;
                }
            }
        }
        else
        {
            typedef int(__stdcall *ai_run_t)(int boardW, int boardH, char board[], char curPiece, int curX, int curY, int curR, char nextPiece, int *bestX, int *bestRotation);
            int best_x = node->status.x + 1, best_r = node->status.r;
            ((ai_run_t)ai[version])(w, h, param_map, node->status.t, best_x, node->status.y + 1, best_r, ' ', &best_x, &best_r);
            --best_x;
            int r = node->status.r;
            while(best_r > r && node->rotate_counterclockwise && node->rotate_counterclockwise->check(map))
            {
                ++r;
                node = node->rotate_counterclockwise;
            }
            while(best_r < r && node->rotate_clockwise && node->rotate_clockwise->check(map))
            {
                --r;
                node = node->rotate_clockwise;
            }
            while(best_x > node->status.x && node->move_right && (node->row >= map.roof || node->move_right->check(map)))
            {
                node = node->move_right;
            }
            while(best_x < node->status.x && node->move_left && (node->row >= map.roof || node->move_left->check(map)))
            {
                node = node->move_left;
            }
        }
        node = node->drop(map);
        int clear = node->attach(map);
        this_lines += clear;
        log_rows += clear;
        ++log_piece;
    }
}