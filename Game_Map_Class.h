#ifndef GAME_MAP_CLASS_H
#define GAME_MAP_CLASS_H

#include <vector>
#include <string>
#include "conioex_New.h"

struct EnemySpawnInfo {
    int type;
    float x;
    float y;
};

static constexpr int MAX_ROW = 5;
static constexpr int MAX_COL = 5;


extern int g_mapX;
extern int g_mapY;
void InitMaps();
void CleanupMaps();

// ------------------- 解决 “Character” 未识别 -------------------
// 如果你只在本头文件用到 Character* 指针，可以用前置声明：
class Character;
// 如果你还要访问 Character 的方法或成员，就要改成：
// #include "Game_Character_Class.h"

// 每个Tile的信息
struct Tile {
    int tileID;       // 贴图/类型ID
    bool isCollidable;// 是否碰撞
    Bmp* texture;     // 可不使用
};

struct Vector2
{
    float x;
    float y;
};
class GameMap {
private:
    // 碰撞层(地形层)
    std::vector<std::vector<Tile>> collisionLayer;
    // 背景层
    std::vector<std::vector<Tile>> backgroundLayer;
    std::vector<std::vector<int>> decorationLayer;

    // 贴图管理
    std::vector<Bmp*> tileSet;

    // Tile 基本信息
    int tileSize;  // 文件中 80×80

    int enemyCount = 0;
    std::vector <Vector2> enemy01Postion = {};

public:

    std::vector<EnemySpawnInfo> enemySpawnInfos;
    // 构造 & 析构
    GameMap(int width, int height);
    ~GameMap();

    // 加载贴图资源
    void LoadTileImages();

    // 从文件中读取地图
    bool LoadFromFile(const char* filename);

    // 绘制
    void Draw(int layer) const;

    // 简易碰撞判断(针对 collisionLayer)
    bool CheckCollision(float x, float y, float w, float h) const;

    // 兼容旧逻辑：扫描下面的碰撞Tile
    float GetGroundY(float x, float y, float h) const;

    // ===== 地图边界切换相关 =====
    void SetBoundaryMaps(const std::string& left,
        const std::string& right,
        const std::string& up,
        const std::string& down);

    // 每帧可调用，若角色出边界，尝试切图或Clamp
    // 注意这里用到 Character*
    void CheckBoundaryAndSwitch(Character* c);

    // 记录边界要切换到的地图
    std::string nextMapLeft;
    std::string nextMapRight;
    std::string nextMapUp;
    std::string nextMapDown;
    //生成敌人
    void SpawnEnemies();
};
extern GameMap* g_maps[MAX_ROW][MAX_COL];

#endif // GAME_MAP_CLASS_H