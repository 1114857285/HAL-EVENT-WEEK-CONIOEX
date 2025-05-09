#include "Game_Map_Class.h"
#include "Game_Character_Class.h"  // ��Ϊ��Character���� static GameMap* currentMap
#include <fstream>
#include <sstream>
#include <algorithm>

//����
static void LoadAnotherMap(Character* c, const char* nextMapFile);
std::vector<EnemySpawnInfo> enemySpawnInfos;

GameMap* g_maps[MAX_ROW][MAX_COL] = { nullptr };

// ���죺���ø�����߳�ʼ������
GameMap::GameMap(int width, int height)
    : tileSize(32) // Ĭ���ȸ���32,����LoadFromFile�Ḳ��
{
    // ��ʼ������(��ײ/����/װ��)��С
    collisionLayer.resize(height, std::vector<Tile>(width, { 0, false, nullptr }));
    backgroundLayer.resize(height, std::vector<Tile>(width, { 0,false,nullptr }));
    decorationLayer.resize(height, std::vector<int>(width, 0));
}

// ��������ȷ�ͷ� tileSet ��� Bmp*
GameMap::~GameMap() {
    for (auto& bmp : tileSet) {
        if (bmp) {
            DeleteBmp(&bmp);
            bmp = nullptr;
        }
    }
    tileSet.clear();
}

// ����Tileͼ����Դ
void GameMap::LoadTileImages() {
    // ����֧�����50�� tileID
    tileSet.resize(100, nullptr);

    // ʾ������ID=1��һ��ͼ
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

    // ��Ҳ��������� tileID=5,6,12 ��ͼ��
    // tileSet[5] = LoadBmp("maps/Tile_5.png");
    // tileSet[6] = LoadBmp("maps/Tile_6.png");
    // tileSet[12] = LoadBmp("maps/Tile_12.png");
    // ...
}

// ���ļ����ص�ͼ
bool GameMap::LoadFromFile(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    int mapWidth, mapHeight;
    file >> mapWidth >> mapHeight;
    int cellWidth, cellHeight;
    file >> cellWidth >> cellHeight;


    // �� tileSize ����Ϊ�ļ�������
    tileSize = cellWidth;

    // ���µ��������С
    collisionLayer.resize(mapHeight, std::vector<Tile>(mapWidth, { 0,false,nullptr }));
    backgroundLayer.resize(mapHeight, std::vector<Tile>(mapWidth, { 0,false,nullptr }));
    decorationLayer.resize(mapHeight, std::vector<int>(mapWidth, 0));

    // ����ֻ��ʾ������ײ�㡱����Ϣ(ÿ��mapWidth��TileID)
    // ����㻹��Ҫ������/װ�β㣬�Ϳ����ٶ� mapHeight �и� backgroundLayer
    std::string line1;
    std::getline(file, line1); // ��������
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
    std::getline(file, line); // ��������
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
    // ��ȡ����λ����Ϣ
    int enemyCount;
    file >> enemyCount;  // ��ȡ��������
    enemySpawnInfos.clear();
    for (int i = 0; i < enemyCount; i++) {
        EnemySpawnInfo info;
        // �������ζ�ȡ��type, x, y
        file >> info.type >> info.x >> info.y;
        enemySpawnInfos.push_back(info);
    }

    file.close();
    return true;
    file.close();
    return true;
}

// ���Ƶ�ͼ(����ʾ��ײ��)
void GameMap::Draw(int layer) const {

    switch (layer)
    {
    case 0:
        // 1) ���Ʊ��������ͼ(�� tileID !=0)
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
        // 1) ������ײ�����ͼ(�� tileID !=0)
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




    // 3) ��Ҳ���������ﻭ������ / װ�β� 
//    (�����Ȼ�backgroundLayer��Ȼ���ٻ�collisionLayer���ٻ�decorationLayer)
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
    // 2) �����Ҫ������ײ��
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

// ������ײ���
bool GameMap::CheckCollision(float x, float y, float w, float h) const {
    // ת��Tile����
    int left = (int)(x / tileSize);
    int right = (int)((x + w) / tileSize);
    int top = (int)(y / tileSize);
    int bottom = (int)((y + h) / tileSize);

    // ��Խ��
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

    // ����Lock������ײ���
    for (Character* c : Character::allCharacters)
    {
        Lock* lock = dynamic_cast<Lock*>(c);
        if (lock && !lock->isOpen)
        {
            // ����������ײ��
            float lockLeft = lock->GetX()+5;
            float lockTop = lock->GetY()+5;
            float lockRight = lockLeft + lock->GetWidth()-10;
            float lockBottom = lockTop + lock->GetHeight()-10;

            // ���������������ײ
            bool overlapX = (x < lockRight) && (x + w > lockLeft);
            bool overlapY = (y < lockBottom) && (y + h > lockTop);

            if (overlapX && overlapY)
                return true;
        }
    }
    return false;
}

// ���ݾ��߼�������ɨ�裬�ҵ����ζ���
float GameMap::GetGroundY(float x, float y, float h) const {
    int checkBottom = (int)((y + h) / tileSize);
    int checkX = (int)(x / tileSize);

    // �ӵ�ǰrow��ʼ��������ײ
    for (int iy = checkBottom; iy < (int)collisionLayer.size(); ++iy) {
        if (collisionLayer[iy][checkX].isCollidable) {
            return (float)(iy * tileSize - h);
        }
    }
    return y; // û�ҵ��ͷ���ԭֵ
}

// =============== ���������ñ߽���л��ĵ�ͼ�ļ��� ================= //
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

// =============== ����������ɫԽ�磬�������ŵ�ͼ���л�������Clamp ===============
void GameMap::CheckBoundaryAndSwitch(Character* c)
{
    if (!c||!c->IsPlayer()) return;


    // ���ŵ�ͼ=1280��720
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

    // ��ûԽ�磬ֱ�ӷ���
    if (!outLeft && !outRight && !outUp && !outDown) return;

    // clamp����(��û����һ�ŵ�ͼ���ס)
    auto clampToScreen = [&](float& bx, float& by) {
        if (bx < 0)        bx = 0;
        if (bx + w > SCREEN_WIDTH)   bx = SCREEN_WIDTH - w;
        if (by < topMargin)        by = topMargin;
        if (by + h > SCREEN_HEIGHT)  by = SCREEN_HEIGHT - h;
        };

    float offsetX = hx - c->GetX();
    float offsetY = hy - c->GetY();

    if (outLeft) {
        // ��ͼ���� (g_mapX-1,g_mapY) 
        if (g_mapX - 1 >= 0 && g_maps[g_mapX - 1][g_mapY]) {
            // �л�
            // ��ȫɾ�����е���
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
            // ��ɫ�������µ�ͼ�ұ�
            // newMap=> c->SetPosition(1280 - w - 10,y?)
            float spawnX = SCREEN_WIDTH - w - 50.f;
            float spawnY = c->GetY(); // ����y����,�ɼ�����
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
            // ��ȫɾ�����е���
            auto& characters = Character::allCharacters;
            for (auto it = characters.begin(); it != characters.end();) {
                if (!(*it)->IsPlayer()) {
                    (*it)->markForDeletion = true;
                    it = characters.erase(it);
                }
                else
                    ++it;
            }
            // �л�
            g_mapX++;
            Character::SetCurrentMap(g_maps[g_mapX][g_mapY]);
            // ��ɫ�������µ�ͼ��� x=10
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
            // ��ȫɾ�����е���
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
            // ��ɫ���µ�ͼ�ײ�
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
            // ��ȫɾ�����е���
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
            // ��ɫ���µ�ͼ����
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

// ============== ��������: �л�����һ����ͼ�����ý�ɫλ�� ================= //
static void LoadAnotherMap(Character* c, const char* nextMapFile)
{
    // 1) ��ȡ��ǰ��ͼָ��
    GameMap* currentMap = Character::GetCurrentMap();
    if (currentMap) {
        // ɾ���ɵ�ͼ
        delete currentMap;
        currentMap = nullptr;
    }

    // 2) �½�һ�� GameMap
    //    ������ʱд��16��9����ɸĳ��ļ��ж����Ĵ�С
    GameMap* newMap = new GameMap(16, 9);
    newMap->LoadTileImages();

    // 3) ���ļ������µ�ͼ����
    newMap->LoadFromFile(nextMapFile);

    // 4) ����Ϊ��ɫ��ǰ��ͼ
    Character::SetCurrentMap(newMap);

    // 5) ����㻹�� c->SetGameMap(newMap) Ҳ�����������
    //    c->SetGameMap(newMap);

    // 6) ����ɫһ��Ĭ��λ��(�������⣬ʾ������(100,200))
    //    ��ɸ��ݾ�������(�������߽��ͳ����ڵ�ͼ�ұ�)��������
    c->SetPosition(100.0f, 200.0f);
}

void InitMaps()
{
    // ��ʾ: ֻ��ʼ�� [0][0], [0][1], ������nullptr
    g_maps[0][0] = new GameMap(16, 9); // 16��80=1280,9��80=720
    g_maps[0][0]->LoadTileImages();
    g_maps[0][0]->LoadFromFile("pic/maps/level[0][0].txt");

    g_maps[0][1] = new GameMap(16, 9); // 16��80=1280,9��80=720
    g_maps[0][1]->LoadTileImages();
    g_maps[0][1]->LoadFromFile("pic/maps/level[0][1].txt");

    g_maps[0][2] = new GameMap(16, 9); // 16��80=1280,9��80=720
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
    // ��ʼ���ø���ͼָ��

    // ��ʼ��ͼ
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
            // ע�ᵽȫ�ֽ�ɫ�б����û���ڹ��캯�����Զ�ע�ᣩ
            Character::RegisterCharacter(newEnemy);
            break;
        }

        case 2:
        {
            auto* newEnemy = new Lighting(info.x, info.y);
            newEnemy->InitializeAnimations();
            // ע�ᵽȫ�ֽ�ɫ�б����û���ڹ��캯�����Զ�ע�ᣩ
            Character::RegisterCharacter(newEnemy);
            break;
        }

        case 3:
        {
            auto* newEnemy = new Boss(info.x, info.y, 80, 140, 100, 85, 115);
            newEnemy->InitializeAnimations();
            // ע�ᵽȫ�ֽ�ɫ�б����û���ڹ��캯�����Զ�ע�ᣩ
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