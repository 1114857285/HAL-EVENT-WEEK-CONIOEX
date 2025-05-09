#include "conioex_New.h"
#include "Game_Character_Class.h"
#include "Game_Map_Class.h"
// 全局变量
bool g_isRunning = true;
Bmp* background = LoadBmp("pic/Background.bmp", false);
GameMap* gameMap = nullptr;
// 使用智能指针管理角色
std::shared_ptr<Player> player = nullptr;
std::vector<std::shared_ptr<Enemy>> enemy01s; // 使用智能指针
std::array<std::shared_ptr<Leaf>, 100> leaves;
UI* playerUI = nullptr;
MPUI* playerMPUI = nullptr;
BGMPlayer bgm("mysterious-melody-loop-197040.mp3");
BGMPlayer bgm2("1642908824-1-16.mp3");
BGMPlayer buttonSE("game-start-6104.mp3");

Bmp* wing = LoadBmp("pic/Wing_s.bmp");
Bmp* key = LoadBmp("pic/key_s.bmp");
Bmp* normalKey = LoadBmp("pic/NormalKey_s.bmp");
Bmp* magicBall = LoadBmp("pic/player_bullet.bmp");

int g_mapX = 0;
int g_mapY = 0;

void GamePauseMenu();

void GameStart();

void GameEnd();
void DrawCollisionBox(const Character& character, int colorIndex) {
    if (!character.collisionEnabled) return;
    int x = static_cast<int>(character.GetHitboxX());
    int y = static_cast<int>(character.GetHitboxY());
    int width = static_cast<int>(character.GetWidth());
    int height = static_cast<int>(character.GetHeight());
    DrawRect(x, y, x + width, y + height, colorIndex, false);
}

// 绘制攻击碰撞体（调试用）
void DrawAttackCollider(const Character& character, int colorIndex) {
    if (!character.GetAttackCollider().active) return;

    const auto& collider = character.GetAttackCollider();
    int x = static_cast<int>(collider.x);
    int y = static_cast<int>(collider.y);
    int width = static_cast<int>(collider.width);
    int height = static_cast<int>(collider.height);
    DrawRect(x, y, x + width, y + height, colorIndex, false);
}

void GameLoop() {
    const float deltaTime = 1.0f / 60.0f;
    std::vector<Character*> toDelete;
    //RedPotion* red = new RedPotion(500, 600);
    //BluePotion* blue = new BluePotion(700, 600);
    //Lighting* lighting = new Lighting(600,-80);
    // Update 所有角色
    while (g_isRunning) {
        UpdateKeyStates();
        ClearScreen();
        if (player->hasKey)
        {
            bgm.Stop();
            bgm2.Play(true);
        }
        for (int i = 0; i < BGMPlayer::bgms.size(); i++)
        {
            BGMPlayer::bgms[i]->Update(deltaTime);
        }
        // 先画背景
        DrawBmp(0, 0, background);

        // 画地图背景层
        if (g_maps[g_mapX][g_mapY]) {
            g_maps[g_mapX][g_mapY]->Draw(0);
        }
        // 画碰撞层地图
        if (g_maps[g_mapX][g_mapY]) {
            g_maps[g_mapX][g_mapY]->Draw(1);
        }


        //update
        for (int i = 0; i < Character::allCharacters.size();i++)
        {
            if (Character::allCharacters[i])
            {
                if (!Character::allCharacters[i]->IsMarkedForDeletion())
                {
                    Character::allCharacters[i]->Update(deltaTime);
                    Character::GetCurrentMap()->CheckBoundaryAndSwitch(Character::allCharacters[i]);
                }

            }
        }
        std::sort(Character::allCharacters.begin(), Character::allCharacters.end(),
            [](Character* a, Character* b) {
                return a->layer < b->layer;
            });
        // Draw 所有角色
        for (Character* c : Character::allCharacters) {
            if (!c->IsMarkedForDeletion()) {
                c->Draw();
                #if 0
                DrawCollisionBox(*c, GREEN);
                DrawAttackCollider(*c, RED);
                #endif
            }
        }
        // 收集待删除角色
        for (Character* character : Character::allCharacters) {
            if (character->IsMarkedForDeletion()) {
                toDelete.push_back(character);
            }
        }

        //统一移除和删除
        for (Character* character : toDelete) {
            // 从全局列表移除
            Character::UnregisterCharacter(character);


            // 释放内存
            delete character;
            //printf("[GameLoop] Deleted character %p\n", character);
        }
        toDelete.clear();
#if 1
        for (std::shared_ptr<Leaf> leaf : leaves)
        {
            leaf->Update(deltaTime);
            leaf->Draw();//画在最前面
        }
#endif   
        for (int i = 0; i < UI::UIs.size(); i++)
        {
            UI::UIs[i]->Update(deltaTime);
        }
        for (int i = 0; i < UI::UIs.size(); i++)
        {
            UI::UIs[i]->Draw();
        }
       // playerUI->Update(deltaTime);
        //playerUI->Draw();
        if (player->GetCharacterState() == DEAD)
        {
            if(!player->drawWhat)
            DrawBmp(0, 0, player->dead, true);
            else DrawBmp(0, 0, player->standUp,true);
        }
        if (player->canUseDash)
            DrawBmp(120, 130, wing, true);
        if(player->hasNormalKey)
            DrawBmp(160, 130, normalKey, true);
        if(player->canShoot)
            DrawBmp(200, 130, magicBall, true);
        if(player->hasKey)
            DrawBmp(240, 130, key, true);
        // 帧同步
        FrameSync();
        PrintFrameBuffer();
        FlipScreen();

        // ESC退出
        if (GetKeyUp(PK_ESC)) {
            GamePauseMenu();
        }

        if (player->gameClear)
            return;
    }
}
int main() {
    // 初始化控制台窗口
    InitConioEx(1280, 720, 1, 1, true);

    GameStart();
#define P_C_W  45  //主角碰撞体宽度
#define P_C_H  70  //主角碰撞体高度
    player = std::make_shared<Player>(100, 500, P_C_W, P_C_H, 44, 57);
    player->InitializeAnimations();    
    //BulletEmitter* emitter = new BulletEmitter(640, 50, player,12.0f, 10, 1, 3, 0.3);
    //emitter->InitializeAnimations();
    InitMaps(); // 里面把 g_maps[0][0], [0][1], ... 赋值
    Character::SetCurrentMap(g_maps[0][0]);
    g_maps[0][0]->SpawnEnemies();
    g_mapX = 0; g_mapY = 0;

#if 1
    for (size_t i = 0; i < leaves.size();i++)
    {
        leaves[i] = std::make_shared<Leaf>((rand() % 2000) - 720 - 200, rand() % 1500 - 750);
        leaves[i]->InitializeAnimations();
    }
#endif

    //bgm
    bgm.Play(true);
    bgm.SetVolume(20);

     playerUI = new UI(10, 10);
     playerMPUI = new MPUI(10, 70);
     //Projectile* projectile = new Projectile(1200,600,10,10,0,0,10,-2,0);
    // 5) 启动游戏主循环
    GameLoop();

    GameEnd();
    // 退出前释放
    DeleteBmp(&background);
    delete gameMap;
    EndConioEx();
    return 0;
}

void GamePauseMenu()
{
    // 预加载菜单用的BMP，确保只加载一次
    Bmp* bmp1 = LoadBmp("pic/pause_ui_1.bmp");
    Bmp* bmp2 = LoadBmp("pic/pause_ui_2.bmp");
    Bmp* bmp3 = LoadBmp("pic/Tutorial_1.bmp");
    Bmp* bmp4 = LoadBmp("pic/Tutorial_2.bmp");
    const float deltaTime2 = 1.0f / 60.0f;
    ClearKeyStates();
    int chose = 5000;

    // 在进入菜单时构造 players 数组，确保动画初始化只调用一次
    std::array<MenuePlayer, 6> players = {
        MenuePlayer(193, 164, RUNNING),
        MenuePlayer(193, 255, JUMPING),
        MenuePlayer(200, 343, ATTACKING),
        MenuePlayer(1050, 164, ATTACKING2),
        MenuePlayer(1045, 255, IDLE),
        MenuePlayer(1045, 343, RUNNING)
    };

    // 如果可能，请在 MenuePlayer 的构造中或外部确保 InitializeAnimations() 已调用一次
    // 例如：for (auto& p : players) p.InitializeAnimations();

    // 主菜单循环
    while (true)
    {
        UpdateKeyStates();
        ClearScreen();
        DrawBmp(0, 0, background, true);
        if (GetKeyDown(PK_UP) || GetKeyDown(PK_W))
            chose++;
        if (GetKeyDown(PK_DOWN) || GetKeyDown(PK_S))
            chose--;

        // 绘制教程图片（背景元素）
        DrawBmp(50, 200, bmp3, true);
        DrawBmp(900, 200, bmp4, true);

        // 根据选择绘制对应的UI图像，并响应 Enter 键
        switch (chose % 2)
        {
        case 0:
            DrawBmp(420, 25, bmp1, true);
            if (GetKeyDown(PK_ENTER))
            {
                // 选择 0 时，退出菜单，恢复游戏
                buttonSE.Play(false);
                break;
            }
            break;
        case 1:
            DrawBmp(420, 25, bmp2, true);
            if (GetKeyDown(PK_ENTER))
            {
                // 选择 1 时，退出菜单并结束游戏
                EndConioEx();
                return;
            }
            break;
        }

        // 更新并绘制所有菜单角色（不要在每帧重新初始化动画）
        for (int i = 0; i < players.size(); i++)
        {
            players[i].Update(deltaTime2);
        }
        for (int i = 0; i < players.size(); i++)
        {
            players[i].Draw();
        }

        FrameSync();
        PrintFrameBuffer();
        FlipScreen();

        // 这里用 GetKeyUp 检测 ESC 键释放，避免快速触发导致菜单闪退
        if (GetKeyUp(PK_ESC))
            break;
    }

    // 退出菜单后释放菜单BMP资源
    DeleteBmp(&bmp1);
    DeleteBmp(&bmp2);
    DeleteBmp(&bmp3);
    DeleteBmp(&bmp4);
}

void GameStart()
{
    ClearKeyStates();
    int chose = -1;
    Bmp* bmp1 = LoadBmp("pic/Start_menu_1.bmp");
    Bmp* bmp2 = LoadBmp("pic/Start_menu_2.bmp");
    Bmp* bmp0 = LoadBmp("pic/GameStart.bmp");
    while (1)
    {
        UpdateKeyStates();
        ClearScreen();
        // 先画背景
        DrawBmp(0, 0, bmp0,true);
        if (GetKeyUp(PK_ENTER) && chose < 0)
            chose = 5000;
        if (GetKeyDown(PK_ESC) && chose > 0)
            chose = -1;
        if (GetKeyDown(PK_UP) || GetKeyDown(PK_W))
            chose++;
        if (GetKeyDown(PK_DOWN) || GetKeyDown(PK_S))
        {
            chose--;
        }
        switch (chose % 2)
        {
        case 0:
            DrawBmp(420, 25, bmp1, true);
            if (GetKeyDown(PK_ENTER))
            {
                buttonSE.Play(false);
                return;
            }

            break;
        case 1:
            DrawBmp(420, 25, bmp2, true);
            if (GetKeyDown(PK_ENTER))
                EndConioEx();
            break;
        default:
            break;
        }


        // 帧同步
        FrameSync();
        PrintFrameBuffer();
        FlipScreen();
    }
}

void GameEnd()
{
    ClearKeyStates();
    int chose = -1;
    Bmp* bmp2 = LoadBmp("pic/End_menu.bmp");
    Bmp* bmp0 = LoadBmp("pic/GameClear.bmp");
    while (1)
    {
        UpdateKeyStates();
        ClearScreen();
        // 先画背景
        DrawBmp(0, 0, bmp0, true);
        if (GetKeyUp(PK_ENTER) && chose < 0)
            chose = 5000;
        if (GetKeyDown(PK_ESC) && chose > 0)
            chose = -1;
        switch (chose % 2)
        {
        case 0:
            DrawBmp(420, 25, bmp2, true);
            if (GetKeyDown(PK_ENTER))
                EndConioEx();
            break;
        default:
            break;
        }


        // 帧同步
        FrameSync();
        PrintFrameBuffer();
        FlipScreen();
    }
}