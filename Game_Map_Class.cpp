#include "Game_Map_Class.h"
#include "Game_Character_Class.h"  // 因为在Character里有 static GameMap* currentMap
#include <fstream>
#include <sstream>
#include <algorithm>

//声明
static void LoadAnotherMap(Character* c, const char* nextMapFile);
std::vector<EnemySpawnInfo> enemySpawnInfos;

GameMap* g_maps[MAX_ROW][MAX_COL] = { nullptr };

// 构造：先用给定宽高初始化各层
GameMap::GameMap(int width, int height)
    : tileSize(32) // 默认先给个32,后面LoadFromFile会覆盖
{
    // 初始化三层(碰撞/背景/装饰)大小
    collisionLayer.resize(height, std::vector<Tile>(width, { 0, false, nullptr }));
    backgroundLayer.resize(height, std::vector<Tile>(width, { 0,false,nullptr }));
    decorationLayer.resize(height, std::vector<int>(width, 0));
}

// 析构：正确释放 tileSet 里的 Bmp*
GameMap::~GameMap() {
    for (auto& bmp : tileSet) {
        if (bmp) {
            DeleteBmp(&bmp);
            bmp = nullptr;
        }
    }
    tileSet.clear();
}

// 加载Tile图像资源
void GameMap::LoadTileImages() {
    // 假设支持最多50种 tileID
    tileSet.resize(100, nullptr);

    // 示例：给ID=1绑定一张图
    tileSet[1] = LoadBmp("pic/maps/Tile_01.bmp");
    tileSet[2] = LoadBmp("pic/maps/Tile_02.bmp");
    tileSet[3] = LoadBmp("pic/maps/Tile_03.bmp");
    tileSet[6] = LoadBmp("pic/maps/Tile_06.bmp");
    tileSet[7] = LoadBmp("pic/maps/Tile_07.bmp");
    tileSet[8] = LoadBmp("pic/maps/Tile_08.bmp");
    tileSet[13] = LoadBmp("pic/maps/Tile_13.bmp");
    tileSet[14] = LoadBmp("pic/maps/Tile_14.bmp");
    tileSet[15] = LoadBmp("pic/maps/Tile_15.bmp");
    tileSet[11] = LoadBmp("pic/maps/Tile_middle_lane_rock1_1.bmp");
    tileSet[12] = LoadBmp("pic/maps/Tile_middle_lane_rock1_2.bmp");
    tileSet[31] = LoadBmp("pic/maps/Tile_Tree_1.bmp");
    tileSet[32] = LoadBmp("pic/maps/Tile_Tree_2.bmp");
    tileSet[33] = LoadBmp("pic/maps/Tile_Tree_3.bmp");
    tileSet[34] = LoadBmp("pic/maps/Tile_bench_1.bmp");
    tileSet[41] = LoadBmp("pic/maps/Tile_41.bmp");
    tileSet[36] = LoadBmp("pic/maps/Tile_Box_1.bmp");
    tileSet[37] = LoadBmp("pic/maps/Tile_grass_1.bmp");
    tileSet[51] = LoadBmp("pic/maps/Fence.bmp");
    tileSet[52] = LoadBmp("pic/maps/Fence_1.bmp");//Fence_1.bmp

    // 你也可在这里给 tileID=5,6,12 赋图：
    // tileSet[5] = LoadBmp("maps/Tile_5.png");
    // tileSet[6] = LoadBmp("maps/Tile_6.png");
    // tileSet[12] = LoadBmp("maps/Tile_12.png");
    // ...
}

// 从文件加载地图
bool GameMap::LoadFromFile(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    int mapWidth, mapHeight;
    file >> mapWidth >> mapHeight;
    int cellWidth, cellHeight;
    file >> cellWidth >> cellHeight;


    // 将 tileSize 设置为文件读到的
    tileSize = cellWidth;

    // 重新调整各层大小
    collisionLayer.resize(mapHeight, std::vector<Tile>(mapWidth, { 0,false,nullptr }));
    backgroundLayer.resize(mapHeight, std::vector<Tile>(mapWidth, { 0,false,nullptr }));
    decorationLayer.resize(mapHeight, std::vector<int>(mapWidth, 0));

    // 下面只演示读“碰撞层”的信息(每行mapWidth个TileID)
    // 如果你还需要读背景/装饰层，就可以再读 mapHeight 行给 backgroundLayer
    std::string line1;
    std::getline(file, line1); // 跳过换行
    for (int y = 0; y < mapHeight; y++) {
        if (!std::getline(file, line1)) break;
        std::stringstream ss(line1);
        for (int x = 0; x < mapWidth; x++) {
            int value;
            ss >> value;
            backgroundLayer[y][x].tileID = value;
            backgroundLayer[y][x].isCollidable = false;
            backgroundLayer[y][x].texture = nullptr;
        }
    }
    std::string line;
    std::getline(file, line); // 跳过换行
    for (int y = 0; y < mapHeight; y++) {
        if (!std::getline(file, line)) break;
        std::stringstream ss(line);
        for (int x = 0; x < mapWidth; x++) {
            int value;
            ss >> value;
            collisionLayer[y][x].tileID = value;
            collisionLayer[y][x].isCollidable = (value != 0);
            collisionLayer[y][x].texture = nullptr;
        }
    }   
    // 读取敌人位置信息
    int enemyCount;
    file >> enemyCount;  // 读取敌人数量
    enemySpawnInfos.clear();
    for (int i = 0; i < enemyCount; i++) {
        EnemySpawnInfo info;
        // 必须依次读取：type, x, y
        file >> info.type >> info.x >> info.y;
        enemySpawnInfos.push_back(info);
    }

    file.close();
    return true;
    file.close();
    return true;
}

// 绘制地图(仅演示碰撞层)
void GameMap::Draw(int layer) const {

    switch (layer)
    {
    case 0:
        // 1) 绘制背景层的贴图(若 tileID !=0)
        for (int y = 0; y < (int)backgroundLayer.size(); ++y) {
            for (int x = 0; x < (int)backgroundLayer[y].size(); ++x) {
                int tileID = backgroundLayer[y][x].tileID;
                if (tileID != 0 && tileID < (int)tileSet.size()) {
                    Bmp* bmp = tileSet[tileID];
                    if (bmp) {
                        DrawBmp(x * tileSize, y * tileSize, bmp, true);
                    }
                }
            }
        }
        break;
    case 1:
        // 1) 绘制碰撞层的贴图(若 tileID !=0)
        for (int y = 0; y < (int)collisionLayer.size(); ++y) {
            for (int x = 0; x < (int)collisionLayer[y].size(); ++x) {
                int tileID = collisionLayer[y][x].tileID;
                if (tileID != 0 && tileID < (int)tileSet.size()) {
                    Bmp* bmp = tileSet[tileID];
                    if (bmp) {
                        DrawBmp(x * tileSize, y * tileSize, bmp, true);
                    }
                }
            }
        }
        break;
    default:
        break;
    }




    // 3) 你也可以在这里画背景层 / 装饰层 
//    (例如先画backgroundLayer，然后再画collisionLayer，再画decorationLayer)
//    
// for (int y=0; y<backgroundLayer.size(); ++y) {
//     for (int x=0; x<backgroundLayer[y].size(); ++x) {
//         int bgID = backgroundLayer[y][x];
//         if (bgID != 0 && bgID < (int)tileSet.size()) {
//             Bmp* bmp = tileSet[bgID];
//             if (bmp) {
//                 DrawBmp(x*tileSize, y*tileSize, bmp);
//             }
//         }
//     }
// }
// ...
#if 0
    // 2) 如果需要调试碰撞体
    for (int y = 0; y < (int)collisionLayer.size(); ++y) {
        for (int x = 0; x < (int)collisionLayer[y].size(); ++x) {
            if (collisionLayer[y][x].isCollidable) {
                DrawRect(
                    x * tileSize, y * tileSize,
                    (x + 1) * tileSize, (y + 1) * tileSize,
                    BLUE, false
                );
            }
        }
    }
#endif


}

// 简易碰撞检测
bool GameMap::CheckCollision(float x, float y, float w, float h) const {
    // 转成Tile坐标
    int left = (int)(x / tileSize);
    int right = (int)((x + w) / tileSize);
    int top = (int)(y / tileSize);
    int bottom = (int)((y + h) / tileSize);

    // 防越界
    left = max(left, 0);
    right = min(right, (int)collisionLayer[0].size() - 1);
    top = max(top, 0);
    bottom = min(bottom, (int)collisionLayer.size() - 1);

    for (int ty = top; ty <= bottom; ++ty) {
        for (int tx = left; tx <= right; ++tx) {
            if (collisionLayer[ty][tx].isCollidable) {
                return true;
            }
        }
    }


    //lock

    // 新增Lock对象碰撞检测
    for (Character* c : Character::allCharacters)
    {
        Lock* lock = dynamic_cast<Lock*>(c);
        if (lock && !lock->isOpen)
        {
            // 计算锁的碰撞框
            float lockLeft = lock->GetX()+5;
            float lockTop = lock->GetY()+5;
            float lockRight = lockLeft + lock->GetWidth()-10;
            float lockBottom = lockTop + lock->GetHeight()-10;

            // 检测区域与锁的碰撞
            bool overlapX = (x < lockRight) && (x + w > lockLeft);
            bool overlapY = (y < lockBottom) && (y + h > lockTop);

            if (overlapX && overlapY)
                return true;
        }
    }
    return false;
}

// 兼容旧逻辑：向下扫描，找到地形顶部
float GameMap::GetGroundY(float x, float y, float h) const {
    int checkBottom = (int)((y + h) / tileSize);
    int checkX = (int)(x / tileSize);

    // 从当前row开始往下找碰撞
    for (int iy = checkBottom; iy < (int)collisionLayer.size(); ++iy) {
        if (collisionLayer[iy][checkX].isCollidable) {
            return (float)(iy * tileSize - h);
        }
    }
    return y; // 没找到就返回原值
}

// =============== 新增：设置边界可切换的地图文件名 ================= //
void GameMap::SetBoundaryMaps(const std::string& left,
    const std::string& right,
    const std::string& up,
    const std::string& down)
{
    nextMapLeft = left;
    nextMapRight = right;
    nextMapUp = up;
    nextMapDown = down;

}

// =============== 新增：检测角色越界，若有下张地图就切换，否则Clamp ===============
void GameMap::CheckBoundaryAndSwitch(Character* c)
{
    if (!c||!c->IsPlayer()) return;


    // 单张地图=1280×720
    constexpr float SCREEN_WIDTH = 1280.f;
    constexpr float SCREEN_HEIGHT = 720.f;
    constexpr float topMargin = -100.0f;

    float hx = c->GetHitboxX();
    float hy = c->GetHitboxY();
    float w = c->GetWidth();
    float h = c->GetHeight();

    bool outLeft = (hx < 0);
    bool outRight = (hx + w > SCREEN_WIDTH);
    bool outUp = (hy < topMargin);        
    bool outDown = (hy + h > SCREEN_HEIGHT);

    // 若没越界，直接返回
    if (!outLeft && !outRight && !outUp && !outDown) return;

    // clamp函数(若没有下一张地图则夹住)
    auto clampToScreen = [&](float& bx, float& by) {
        if (bx < 0)        bx = 0;
        if (bx + w > SCREEN_WIDTH)   bx = SCREEN_WIDTH - w;
        if (by < topMargin)        by = topMargin;
        if (by + h > SCREEN_HEIGHT)  by = SCREEN_HEIGHT - h;
        };

    float offsetX = hx - c->GetX();
    float offsetY = hy - c->GetY();

    if (outLeft) {
        // 地图坐标 (g_mapX-1,g_mapY) 
        if (g_mapX - 1 >= 0 && g_maps[g_mapX - 1][g_mapY]) {
            // 切换
            // 安全删除所有敌人
            auto& characters = Character::allCharacters;
            for (auto it = characters.begin(); it != characters.end();) {
                if (!(*it)->IsPlayer()) {
                    (*it)->markForDeletion = true;
                    it = characters.erase(it);
                }
                else
                ++it;
            }
            g_mapX--;
            Character::SetCurrentMap(g_maps[g_mapX][g_mapY]);
            // 角色出现在新地图右边
            // newMap=> c->SetPosition(1280 - w - 10,y?)
            float spawnX = SCREEN_WIDTH - w - 50.f;
            float spawnY = c->GetY(); // 保持y不变,可加余量
            g_maps[g_mapX][g_mapY]->SpawnEnemies();
            c->SetPosition(spawnX, spawnY);
        }
        else {
            // clamp
            float newHx = hx;
            float newHy = hy;
            clampToScreen(newHx, newHy);
            c->SetPosition(newHx - offsetX, newHy - offsetY);
        }
        return;
    }
    else if (outRight) {
        if (g_mapX + 1 < MAX_COL && g_maps[g_mapX + 1][g_mapY]) {
            // 安全删除所有敌人
            auto& characters = Character::allCharacters;
            for (auto it = characters.begin(); it != characters.end();) {
                if (!(*it)->IsPlayer()) {
                    (*it)->markForDeletion = true;
                    it = characters.erase(it);
                }
                else
                    ++it;
            }
            // 切换
            g_mapX++;
            Character::SetCurrentMap(g_maps[g_mapX][g_mapY]);
            // 角色出现在新地图左边 x=10
            float spawnX = -30.f;
            float spawnY = c->GetY();
            g_maps[g_mapX][g_mapY]->SpawnEnemies();
            c->SetPosition(spawnX, spawnY);
        }
        else {
            // clamp
            float newHx = hx;
            float newHy = hy;
            clampToScreen(newHx, newHy);
            c->SetPosition(newHx - offsetX, newHy - offsetY);
        }
        return;
    }
    else if (outUp) {
        if (g_mapY + 1 >= 0 && g_maps[g_mapX][g_mapY + 1]) {
            // 安全删除所有敌人
            auto& characters = Character::allCharacters;
            for (auto it = characters.begin(); it != characters.end();) {
                if (!(*it)->IsPlayer()) {
                    (*it)->markForDeletion = true;
                    it = characters.erase(it);

                }
                else
                    ++it;
            }
            g_mapY++;
            Character::SetCurrentMap(g_maps[g_mapX][g_mapY]);
            // 角色在新地图底部
            float spawnX = c->GetX();
            float spawnY = SCREEN_HEIGHT - h -80;
            g_maps[g_mapX][g_mapY]->SpawnEnemies();
            c->SetPosition(spawnX, spawnY);
        }
        else {
            float newHx = hx;
            float newHy = hy;
            clampToScreen(newHx, newHy);
            c->SetPosition(newHx - offsetX, newHy - offsetY);
        }
        return;
    }
    else if (outDown) {
        if (g_mapY - 1 < MAX_ROW && g_maps[g_mapX][g_mapY - 1]) {
            // 安全删除所有敌人
            auto& characters = Character::allCharacters;
            for (auto it = characters.begin(); it != characters.end();) {
                if (!(*it)->IsPlayer()) {
                    (*it)->markForDeletion = true;
                    it = characters.erase(it);
                }
                else
                    ++it;
            }
            g_mapY--;
            Character::SetCurrentMap(g_maps[g_mapX][g_mapY]);
            // 角色在新地图顶部
            float spawnX = c->GetX();
            float spawnY = -57;
            g_maps[g_mapX][g_mapY]->SpawnEnemies();
            c->SetPosition(spawnX, spawnY);
        }
        else {
            float newHx = hx;
            float newHy = hy;
            clampToScreen(newHx, newHy);
            c->SetPosition(newHx - offsetX, newHy - offsetY);
        }
        return;
    }
}

// ============== 辅助函数: 切换到另一个地图并设置角色位置 ================= //
static void LoadAnotherMap(Character* c, const char* nextMapFile)
{
    // 1) 获取当前地图指针
    GameMap* currentMap = Character::GetCurrentMap();
    if (currentMap) {
        // 删除旧地图
        delete currentMap;
        currentMap = nullptr;
    }

    // 2) 新建一个 GameMap
    //    这里暂时写死16×9，你可改成文件中读到的大小
    GameMap* newMap = new GameMap(16, 9);
    newMap->LoadTileImages();

    // 3) 从文件加载新地图数据
    newMap->LoadFromFile(nextMapFile);

    // 4) 设置为角色当前地图
    Character::SetCurrentMap(newMap);

    // 5) 如果你还有 c->SetGameMap(newMap) 也可在这里调用
    //    c->SetGameMap(newMap);

    // 6) 给角色一个默认位置(这里随意，示例设在(100,200))
    //    你可根据具体需求(例如从左边进就出现在地图右边)做更复杂
    c->SetPosition(100.0f, 200.0f);
}

void InitMaps()
{
    // 演示: 只初始化 [0][0], [0][1], 其余是nullptr
    g_maps[0][0] = new GameMap(16, 9); // 16×80=1280,9×80=720
    g_maps[0][0]->LoadTileImages();
    g_maps[0][0]->LoadFromFile("pic/maps/level[0][0].txt");

    g_maps[0][1] = new GameMap(16, 9); // 16×80=1280,9×80=720
    g_maps[0][1]->LoadTileImages();
    g_maps[0][1]->LoadFromFile("pic/maps/level[0][1].txt");

    g_maps[0][2] = new GameMap(16, 9); // 16×80=1280,9×80=720
    g_maps[0][2]->LoadTileImages();
    g_maps[0][2]->LoadFromFile("pic/maps/level[0][2].txt");

    g_maps[1][0] = new GameMap(16, 9);
    g_maps[1][0]->LoadTileImages();
    g_maps[1][0]->LoadFromFile("pic/maps/level[1][0].txt");

    g_maps[2][1] = new GameMap(16, 9);
    g_maps[2][1]->LoadTileImages();
    g_maps[2][1]->LoadFromFile("pic/maps/level[2][1].txt");

    g_maps[1][1] = new GameMap(16, 9);
    g_maps[1][1]->LoadTileImages();
    g_maps[1][1]->LoadFromFile("pic/maps/level[1][1].txt"); 


    g_maps[1][2] = new GameMap(16, 9);
    g_maps[1][2]->LoadTileImages();
    g_maps[1][2]->LoadFromFile("pic/maps/level[1][2].txt");

    g_maps[2][1] = new GameMap(16, 9);
    g_maps[2][1]->LoadTileImages();
    g_maps[2][1]->LoadFromFile("pic/maps/level[2][1].txt");



    // g_maps[1][0] ...
    // ...
    // 初始化好各地图指针

    // 起始地图
    g_mapX = 0;
    g_mapY = 0;
    Character::SetCurrentMap(g_maps[g_mapX][g_mapY]);
}

void CleanupMaps()
{
    for (int r = 0; r < MAX_ROW; r++) {
        for (int c = 0; c < MAX_COL; c++) {
            if (g_maps[r][c]) {
                delete g_maps[r][c];
                g_maps[r][c] = nullptr;
            }
        }
    }
}



auto CreateEnemy(int type, float x, float y) {

}
void GameMap::SpawnEnemies() {
    for (const auto& info : enemySpawnInfos) {
        auto newEnemy = nullptr;
        switch (info.type) {
        case 1:
        {
            auto* newEnemy = new Enemy(info.x, info.y, 55, 50, 100, 5, 5);
            newEnemy->InitializeAnimations();
            // 注册到全局角色列表（如果没有在构造函数里自动注册）
            Character::RegisterCharacter(newEnemy);
            break;
        }

        case 2:
        {
            auto* newEnemy = new Lighting(info.x, info.y);
            newEnemy->InitializeAnimations();
            // 注册到全局角色列表（如果没有在构造函数里自动注册）
            Character::RegisterCharacter(newEnemy);
            break;
        }

        case 3:
        {
            auto* newEnemy = new Boss(info.x, info.y, 80, 140, 100, 85, 115);
            newEnemy->InitializeAnimations();
            // 注册到全局角色列表（如果没有在构造函数里自动注册）
            Character::RegisterCharacter(newEnemy);
            break;
        }
        case 4:
        {
            auto* newEnemy= new TreasureChest(info.x, info.y,70,50,2);//593
            break;
        }
        case 5:
        {
            auto* newEnemy = new RedPotion(info.x, info.y);
            break;
        }
        case 6:
        {
            auto* newEnemy = new BluePotion(info.x, info.y);
            break;
        }
        case 7:
        {
            auto* newEnemy = new Fountain(info.x, info.y);//560
            newEnemy->InitializeAnimations();
            newEnemy->RegisterCharacter(newEnemy);
            break;
        }
        case 8:
        {
            auto* newEnemy = new Lock(info.x, info.y);
            break;
        }
        case 9:
        {
            auto* newEnemy = new FallingChest(info.x, info.y,70,50);//593
            break;
        }
        case 10:
        {
            auto* newEnemy = new TreasureChest(info.x, info.y, 70, 50, 3);//593
            break;
        }
        case 11:
        {
            auto* newEnemy = new TreasureChest(info.x, info.y, 70, 50, 4);//593
            break;
        }        
        case 12:
        {
            auto* newEnemy = new NormalLock(info.x, info.y);//593
            break;
        }
        default:
            break;
        }


    }
}