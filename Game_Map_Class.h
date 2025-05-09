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

// ------------------- ��� ��Character�� δʶ�� -------------------
// �����ֻ�ڱ�ͷ�ļ��õ� Character* ָ�룬������ǰ��������
class Character;
// ����㻹Ҫ���� Character �ķ������Ա����Ҫ�ĳɣ�
// #include "Game_Character_Class.h"

// ÿ��Tile����Ϣ
struct Tile {
    int tileID;       // ��ͼ/����ID
    bool isCollidable;// �Ƿ���ײ
    Bmp* texture;     // �ɲ�ʹ��
};

struct Vector2
{
    float x;
    float y;
};
class GameMap {
private:
    // ��ײ��(���β�)
    std::vector<std::vector<Tile>> collisionLayer;
    // ������
    std::vector<std::vector<Tile>> backgroundLayer;
    std::vector<std::vector<int>> decorationLayer;

    // ��ͼ����
    std::vector<Bmp*> tileSet;

    // Tile ������Ϣ
    int tileSize;  // �ļ��� 80��80

    int enemyCount = 0;
    std::vector <Vector2> enemy01Postion = {};

public:

    std::vector<EnemySpawnInfo> enemySpawnInfos;
    // ���� & ����
    GameMap(int width, int height);
    ~GameMap();

    // ������ͼ��Դ
    void LoadTileImages();

    // ���ļ��ж�ȡ��ͼ
    bool LoadFromFile(const char* filename);

    // ����
    void Draw(int layer) const;

    // ������ײ�ж�(��� collisionLayer)
    bool CheckCollision(float x, float y, float w, float h) const;

    // ���ݾ��߼���ɨ���������ײTile
    float GetGroundY(float x, float y, float h) const;

    // ===== ��ͼ�߽��л���� =====
    void SetBoundaryMaps(const std::string& left,
        const std::string& right,
        const std::string& up,
        const std::string& down);

    // ÿ֡�ɵ��ã�����ɫ���߽磬������ͼ��Clamp
    // ע�������õ� Character*
    void CheckBoundaryAndSwitch(Character* c);

    // ��¼�߽�Ҫ�л����ĵ�ͼ
    std::string nextMapLeft;
    std::string nextMapRight;
    std::string nextMapUp;
    std::string nextMapDown;
    //���ɵ���
    void SpawnEnemies();
};
extern GameMap* g_maps[MAX_ROW][MAX_COL];

#endif // GAME_MAP_CLASS_H