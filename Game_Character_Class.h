#ifndef GAME_CHARACTER_H
#define GAME_CHARACTER_H

#include <vector>
#include <array>
#include <string>
#include <mutex>
#include <memory>
#include <cmath>
#include "conioex_New.h"
#include <unordered_set>
#include "Game_Map_Class.h"
#include <algorithm>


//Game_Character_Class.h
// 角色动作状态
enum CharacterState {
    IDLE, RUNNING, JUMPING, FALLING, ATTACKING,ATTACKING2,ATTACKING3,HURT,DEAD,FREEZE, CHASE, RETREAT
};

// 动画结构体
struct Animation {
    std::vector<std::shared_ptr<Bmp>> frames;
    float frameTime;
    Animation() : frameTime(0.2f) {}
};

// 简单的 BGM 播放器类
class BGMPlayer {
private:
    int* soundHandle;    // 由 MciOpenSound 返回的声音句柄
    bool isLoop;         // 是否循环播放
    float delayTimer;    // 延迟播放计时器，单位秒
    bool delayed;        // 是否正在延迟播放
    bool OnPlay;         //是否正在播放

public:
    static std::vector<BGMPlayer*> bgms;
    void RegisterBGM(BGMPlayer* bgm)
    {
        if (std::find(bgms.begin(), bgms.end(), bgm) == bgms.end())
        {
            bgms.push_back(bgm);
        }
    }
    void UnRegisterBGM(BGMPlayer* bgm)
    {
        auto it = std::remove(bgms.begin(), bgms.end(), bgm);
        if (it != bgms.end()) {
            bgms.erase(it, bgms.end());
        }
    }

    BGMPlayer(const char* path)
        : soundHandle(nullptr), isLoop(false), delayTimer(0.0f), delayed(false)
    {
        // 添加前置路径，例如 "sound/"，然后再连接传入的 path
        std::string fullPath = "sound/";
        fullPath += path;
        soundHandle = MciOpenSound(fullPath.c_str());
        RegisterBGM(this);
    }

    // 析构函数：停止播放并关闭句柄
    ~BGMPlayer() {
        Stop();
        if (soundHandle) {
            MciCloseSound(soundHandle);
            soundHandle = nullptr;
        }
        UnRegisterBGM(this);
    }

    // 播放声音：参数 loop 表示是否循环播放，delaySeconds 指定延迟播放的时间（单位秒）
    void Play(bool loop = false, float delaySeconds = 0.0f) {
        if (!soundHandle||OnPlay) return;
        isLoop = loop;
        if (delaySeconds > 0.0f) {
            delayTimer = delaySeconds;
            delayed = true;
        }
        else {
            // 播放之前先回到开始位置
            MciPlaySound(soundHandle, loop ? 1 : 0);
        }
        OnPlay = true;
    }

    // 停止播放
    void Stop() {
        if (!soundHandle||!OnPlay) return;
        MciStopSound(soundHandle);
        delayed = false;
        delayTimer = 0.0f;
        OnPlay = false;
    }

    // 每帧更新：用于处理延迟播放以及循环播放的更新
    void Update(float deltaTime) {
        if (!soundHandle) return;
        // 处理延迟播放逻辑
        if (delayed) {
            delayTimer -= deltaTime;
            if (delayTimer <= 0.0f) {
                delayed = false;
                MciPlaySound(soundHandle, isLoop ? 1 : 0);
            }
        }
        // 如果是循环播放，则检查播放状态，更新以确保循环
        if (isLoop) {
            MciUpdateSound(soundHandle);
        }
        if (!isLoop && !delayed && !MciCheckSound(soundHandle)) {
            OnPlay = false;
        }

    }

    // 设置播放音量（0~100）
    void SetVolume(int percent) {
        if (!soundHandle) return;
        MciSetVolume(soundHandle, percent);
    }

    // 检查是否正在播放（返回 true 表示正在播放）
    bool IsPlaying() const {
        if (!soundHandle) return false;
        return MciCheckSound(soundHandle) != 0;
    }
};

// 攻击碰撞体结构体
struct AttackCollider {
    float x, y;          // 碰撞体位置
    float width, height; // 碰撞体大小
    bool active = false; // 是否激活
};

// 通用角色父类
class Character {
protected:
    float x, y;           // 位置
    float vx, vy;         // 速度
    float width, height;  // 碰撞盒大小
    float hitboxOffsetX;  // 碰撞盒X轴偏移
    float hitboxOffsetY;  // 碰撞盒Y轴偏移
    bool isOnGround;      // 是否在地面上
    bool facingRight = true; // 是否面向右侧
    CharacterState state; // 当前状态
    Animation animations[12]; // 每个状态的动画
    std::atomic<int> currentFrame;     // 当前帧索引
    float frameCounter;      // 帧计时器
    float gravity = 0.5f; // 重力加速度
    std::mutex animationMutex;
    AttackCollider attackCollider; // 攻击碰撞体
    Character* target = nullptr;   // 攻击目标

    int health = 100;             // 生命值
    int mp = 100;             // 生命值
    bool canTakeDamage = true;    // 伤害接收冷却
    float damageCooldown = 0.5f;  // 伤害冷却时间


    bool isAttackActive = false;       // 当前攻击碰撞体是否激活
    std::unordered_set<Character*> hitTargets; // 当前攻击帧已命中的目标
    bool hasTriggeredThisFrame = false; // 标记本帧是否已触发伤害（防止在一次攻击帧数中多次触发受击功能）
    int previousAnimationFrame = -1;  // 记录上一帧的动画索引

    CharacterState previousState = IDLE; // 记录上一帧状态

    // 击退相关属性
    float knockbackForceX = 5.0f;    // 水平击退力
    float knockbackForceY = 1.0f;    // 垂直击退力
    float hurtDuration = 0.2;       // 受击状态持续时间
    float hurtTimer = 0.0f;          // 受击状态计时器
    float invincibilityTimer = 0.0f;
    float invincibilityDuration = 0.4f;
    float freezeTimer = 0.0f;          // 受击状态计时器
    float freezeDuration = 2.0f;          // freeze状态计时器
    bool isKnockback = false;      // 击退状态标记
    bool canKnockback = true;      // 击退状态标记
    float knockbackDuration = 0.2f; // 击退持续时间
    float canKnockbackDuration = 0.2f; // 击退持续时间
    float knockbackTimer = 0.0f;
    float canKnockbackTimer = 0.0f;
    float cureTimer = 0.0f;
    float cureDuration = 1.0f;

    // === 冲刺系统 ===
    float dashSpeed = 12.0f;    // 25像素/帧 
    float dashDuration = 0.2f;  // 秒持续时间
    float dashCooldown = 0.8f;  // 冷却时间
    float dashTimer = 0.0f;         // 冲刺计时器
    float dashCooldownTimer = 0.0f; // 冷却计时器
    bool isDashing = false;         // 冲刺状态
    bool canDash = true;            // 是否可冲刺
    int maxAirDashes = 1;           // 最大空中冲刺次数
    int remainingAirDashes = 1;     // 剩余空中冲刺次数
    bool wasOnGround = isOnGround;
    bool canGroundDash = true;
    struct DashTrail {
        float x, y;
        float alpha;
    };
    std::vector<DashTrail> dashTrails;
    float trailInterval = 0.0f;

    static GameMap* currentMap; // 新增静态地图指针

public:
    bool hasKey = 0;
    bool hasNormalKey = 0;
    bool canUseDash = 0;
    bool canShoot = 0;
    bool gameClear = 0;
    int layer = 1;
    bool hasDead = false;
    bool markForDeletion = false; // 是否应该被释放内存(角色死亡或其他原因不再被需要)
    enum Type { PLAYER, ENEMY, NEUTRAL };
    Type characterType = NEUTRAL;
    explicit Character(Type type) : characterType(type) {}
    Type GetCharacterType() const { return characterType; }
    bool collisionEnabled = true; // 新增碰撞开关
    void SetCollisionEnabled(bool enabled) {
        collisionEnabled = enabled;
    }
    static void SetGameMap(GameMap* map) { currentMap = map; }
    static GameMap* GetCurrentMap() { return currentMap; }
    static void SetCurrentMap(GameMap* map) { currentMap = map; }
    void ResetAirSystem() { // 新增重置方法
        remainingAirDashes = maxAirDashes;
        canDash = true;
        canGroundDash = true;
        dashCooldownTimer = 0.0f;
    }
    virtual void UpdateSE()
    {
    };


    //全局角色列表（角色生成时自动登记到列表中，销毁时自动从删除列表）
    static std::vector<Character*> allCharacters;

    ///////////////////////////角色类构造函数和析构函数///////////////////////////
    Character(float startX, float startY, float w, float h, float offsetX = 0, float offsetY = 0, Type type=NEUTRAL);
    virtual ~Character()
    {
        // 自动从全局列表移除
        auto it = std::find(allCharacters.begin(), allCharacters.end(), this);
        if (it != allCharacters.end()) {
            allCharacters.erase(it);
        }
    }


    ////////////////////////////动画加载函数声明//////////////////////////////////////////
    bool LoadAnimation(CharacterState action, const std::vector<std::string>& framePaths, float frameTime);

    /////////////////////////角色更新和角色绘画函数声明/////////////////////////////
    virtual void Update(float deltaTime);
    virtual void Draw();

    /////////////////////////角色物理性质定义/////////////////////////////
    int GetHealth() const { return health; }
    int GetMP() const { return mp; }
    float GetWidth() const { return width; }
    float GetHeight() const { return height; }
    float GetX() const { return x; }
    float GetY() const { return y; }
    float GetHitboxX() const { return x + hitboxOffsetX; }
    float GetHitboxY() const { return y + hitboxOffsetY; }
    float GetVx() const { return vx; }
    float GetVy() const { return vy; }
    float GetHitboxCenterX() const {
        return x + hitboxOffsetX + width * 0.5f;
    }

    float GetHitboxCenterY() const {
        return y + hitboxOffsetY + height * 0.5f;
    }
    float GetHitboxTopY() const {
        return y + hitboxOffsetY;
    }
    void SetPosition(float newX,float newY) 
    {
        if (newX != -1)
        {
            x = newX;
        }
        if (newY != -1)
        {
            y = newY;
        }
    }
    CharacterState GetCharacterState() const { return state; }
    /////////////////////////角色受击以及死亡/////////////////////////////
    virtual void TakeDamage(int damage, float attackerX);

    void TakeHealth(int cure) { health += cure; if (health > 100) health = 100; }
    void TakeMP(int cureMp) { mp += cureMp; if (mp > 100)mp = 100; }
    void Cost(int cost) { mp -= cost; if (mp <= 0)mp = 0; };
    void Cure() 
    { 
        if (mp - 20 < 0||health>=100)
            return;
        Cost(20);
        TakeHealth(20);
    }
    virtual void Die();
    
    ////////////////////角色从列表登记和删除函数/////////////////////////
    bool IsMarkedForDeletion() const { return markForDeletion; } //表明需要被删除时的参数 函数
    static void RegisterCharacter(Character* character);
    static void UnregisterCharacter(Character* character);

    ///////////////////////////角色攻击时开启攻击碰撞体以及设置攻击目标和获取攻击目标///////////////////
    const AttackCollider& GetAttackCollider() const { return attackCollider; }
    void SetTarget(Character* newTarget) { target = newTarget; }
    Character* GetTarget() const { return target; }
    void ClearHitTargets() { hitTargets.clear(); } // 清空命中记录

   
    ////////////////////// 通用碰撞检测方法（优化版）/////////////////////
  // 角色基类中
    virtual bool IsEnemy() const { return false; }
    virtual bool IsPlayer() const { return false; }
    virtual bool IsBoss() const { return false; }
    virtual bool IsBullet() const { return false; }
    virtual bool IsFallingChest() const { return false; }
    virtual bool IsLighting() const { return false; }
    // 保持碰撞检测的纯粹性，只负责返回碰撞对象
    std::vector<Character*> CheckCollisionWithOthers() {
        if (!collisionEnabled) return {};
        std::vector<Character*> collidedCharacters;

        // 使用更精确的碰撞盒变量名
        const float thisLeft = GetHitboxX();
        const float thisTop = GetHitboxY();
        const float thisRight = thisLeft + GetWidth();
        const float thisBottom = thisTop + GetHeight();

        for (Character* other : allCharacters) {
            if (other == this||!other->collisionEnabled) continue;

            // 优化敌人互斥逻辑：通过虚函数实现多态判断
            //if (this->IsEnemy() && other->IsEnemy()) continue;

            // 复用碰撞盒计算逻辑
            const float otherLeft = other->GetHitboxX();
            const float otherTop = other->GetHitboxY();
            const float otherRight = otherLeft + other->GetWidth();
            const float otherBottom = otherTop + other->GetHeight();

            // 更清晰的AABB碰撞条件
            bool isColliding = (thisLeft < otherRight) &&
                (thisRight > otherLeft) &&
                (thisTop < otherBottom) &&
                (thisBottom > otherTop);

            if (isColliding) {
                collidedCharacters.push_back(other);
            }
        }
        return collidedCharacters;
    }

    //dash
    virtual void StartDash(float inputDir) {}
    void UpdateDash(float deltaTime);
    void ResetAirDashes() { remainingAirDashes = maxAirDashes; }

////////////////击退函数///////////////////////
void ApplyKnockback(float attackerX) {
    // 如果还在击退冷却里，就不再触发新的击退
    if (!canKnockback||this->IsBoss()) {
        return;
    }

    // 真正开始击退
    float direction = (GetHitboxCenterX() < attackerX) ? -1.0f : 1.0f;
    vx = direction * knockbackForceX;  // 水平速度
    vy = -knockbackForceY;            // 给一个向上分量
    isOnGround = false;               // 离地

    // 进入击退状态
    isKnockback = true;
    knockbackTimer = knockbackDuration;

    // 关闭再次击退的可能性
    canKnockback = false;
    canKnockbackTimer = canKnockbackDuration;
}
/////////////////当攻击碰撞体激活时调用（每次攻击帧开始时）/////////////////////
    void OnAttackFrameStart() {
        hitTargets.clear();  // 清空历史命中记录
        isAttackActive = true;
    }

////////////当攻击碰撞体关闭时调用（每次攻击帧结束时）///////////
    void OnAttackFrameEnd() {
        isAttackActive = false;
    }

/////////// 攻击完全结束时调用（整个连击结束时）////////////////////
    void OnAttackComboEnd() {
        hitTargets.clear();
        isAttackActive = false;
    }


protected:
    void UpdateAnimation(float deltaTime);
    void ActivateAttackCollider(float offsetX, float offsetY, float width, float height);
    void DeactivateAttackCollider();
    void CheckAttackHit();
};

//////////////////主角类///////////////////////
class Player : public Character {
private:
    const float jumpForce = -14.0f;
    const float moveSpeed = 4.0f;
    BGMPlayer* runningSE = nullptr;
    BGMPlayer* attackingSE = nullptr;
    BGMPlayer* jumpSE = nullptr;
    BGMPlayer* magicSE = nullptr;
    float deadTimer = 0;
    float deadDuration = 2.2;

public:
    bool drawWhat = 0;
    Bmp* dead = nullptr;
    Bmp* standUp = nullptr;
    Player(float startX, float startY, float w, float h, float offsetX = 0, float offsetY = 0, Type type=PLAYER);
    bool InitializeAnimations();
    void Update(float deltaTime) override;
    void StartDash(float inputDir) override;
    bool IsPlayer() const override { return true; }
    bool IsBoss() const override { return false; }
    void Shoot();
private:
    void HandleInput();
};
void DrawBulletCollisionBox(std::shared_ptr <Character> character, int colorIndex);


class MenuePlayer :public Character
{
public:
    // Player 类的 InitializeAnimations 实现
    bool InitializeAnimations() {
        if (!LoadAnimation(IDLE, { "player_idle_1.bmp", "player_idle_2.bmp", "player_idle_3.bmp", "player_idle_4.bmp", "player_idle_5.bmp" }, 0.2f)) return false;
        if (!LoadAnimation(RUNNING, { "player_run_1.bmp","player_run_2.bmp","player_run_3.bmp","player_run_4.bmp",
            "player_run_5.bmp","player_run_6.bmp","player_run_7.bmp","player_run_8.bmp" }, 0.15f)) return false;
        if (!LoadAnimation(JUMPING, { "player_jump_1.bmp","player_jump_2.bmp","player_jump_3.bmp","player_jump_4.bmp",
            "player_jump_5.bmp","player_jump_6.bmp","player_jump_7.bmp","player_jump_8.bmp" }, 0.1f)) return false;
        if (!LoadAnimation(ATTACKING, { "player_attack_3.bmp","player_attack_4.bmp",
            "player_attack_5.bmp","player_attack_6.bmp","player_attack_7.bmp","player_attack_8.bmp",
            "player_attack_9.bmp","player_attack_10.bmp","player_attack_11.bmp","player_attack_12.bmp" }, 0.08f)) return false;
        if (!LoadAnimation(ATTACKING2, { "player_attack2_1.bmp","player_attack2_2.bmp","player_attack2_3.bmp","player_attack2_4.bmp",
            "player_attack2_5.bmp","player_attack2_6.bmp","player_attack2_7.bmp","player_attack2_8.bmp",
            "player_attack2_9.bmp","player_attack2_10.bmp" }, 0.1f)) return false;
        if (!LoadAnimation(HURT, { "player_hurt_1.bmp", "player_hurt_2.bmp", "player_hurt_shine_1.bmp",
        "player_hurt_shine_2.bmp" }, 0.1f)) return false;
        if (!LoadAnimation(FREEZE, { "player_freeze_1.bmp", "player_freeze_2.bmp", "player_freeze_shine_1.bmp","player_freeze_shine_2.bmp" }, 0.1f)) return false;
        if (!LoadAnimation(DEAD, { "player_dead_1.bmp", "player_dead_2.bmp", "player_dead_3.bmp",
            "player_dead_4.bmp","player_dead_5.bmp" }, 0.3f)) return false;



        return true;    //全部加载后返回为真，新加动画要加到上方


    }

    MenuePlayer(float x, float y, CharacterState newState) :Character(x, y, 0, 0, 0)
    {  
        state = newState; 
        InitializeAnimations(); 
    };

    void HandleInput()
    {
        return;
    }

    bool IsPlayer() const override { return false; }
    void Update(float deltaTime)
    {
        UpdateAnimation(deltaTime);
    }
};
////////////////////敌人类//////////////////
class Enemy : public Character {
private:
    float patrolRange;
    float startX;
    bool movingRight;

public:
    Enemy(float startX, float startY, float w, float h, float range, float offsetX = 0, float offsetY = 0,Type type=ENEMY);
    bool InitializeAnimations();
    void Update(float deltaTime) override;
    void TakeDamage(int damage, float attackerX) override;
        ~Enemy() override {
        // 在敌人析构时自动注销
        Character::UnregisterCharacter(this);
    }
    //////////////敌人类中被设置为敌人//////////
    bool IsEnemy() const override { return true; }
    bool IsBoss() const override { return false; }
    bool IsPlayer() const override { return false; }
private:
    void Patrol();
};

/////////////游戏物体类////////
class GameObject : public Character {
public:
    GameObject(float x, float y, float w, float h, float offsetX = 0, float offsetY = 0)
        : Character(x, y, w, h, offsetX, offsetY) {
        canTakeDamage = false; // 默认不可被攻击
    }

    // 禁用移动和攻击
    void Update(float deltaTime) override {
        UpdateAnimation(deltaTime); // 仅更新动画
    }
};

class Lighting :public Enemy
{
protected:
    int health = 99999;
    bool canKnockback = false;
    bool canTakeDamage = false;
    float destortTimer = 0.0f;
    float destortDuration = 2.2f;
    static BGMPlayer* se;
public:

    bool InitializeAnimations()
    {
        if (!LoadAnimation(IDLE, { "Lightning_spot1.bmp", "Lightning_spot2.bmp", "Lightning_spot3.bmp", "Lightning_spot4.bmp","Lightning_spot1.bmp", "Lightning_spot2.bmp", "Lightning_spot3.bmp","Lightning_spot4.bmp",
            "Lightning_beginning1.bmp","Lightning_beginning2.bmp","Lightning_beginning3.bmp","Lightning_beginning4.bmp","Lightning_beginning5.bmp",
            "Lightning_cycle1.bmp","Lightning_cycle2.bmp","Lightning_cycle3.bmp","Lightning_cycle4.bmp","Lightning_cycle5.bmp","Lightning_cycle6.bmp",
            "Lightning_end1.bmp","Lightning_end2.bmp","Lightning_end3.bmp" }, 0.1f))
            return false;
        return true;
    }
    void Update(float deltaTime) override
    {
        destortTimer += deltaTime;
        if (destortTimer >= destortDuration)
            markForDeletion = true;
        if (currentFrame >= animations[IDLE].frames.size() - 1)
        {
            markForDeletion = true;
        }
        if(currentFrame==9)
            se->Play(false);
        if (currentFrame == 0 || currentFrame == 1 || currentFrame == 2 || currentFrame == 3 || currentFrame == 4 ||
            currentFrame == 5 || currentFrame == 6 || currentFrame == 7||currentFrame==8||
            currentFrame == 19 || currentFrame == 20 || currentFrame ==21)
        {
            collisionEnabled = false;
        }
        else
        {
            collisionEnabled = true;
        }
        //Character::Update(deltaTime);
        UpdateAnimation(deltaTime);
    }
    Lighting(float x, float y) :Enemy(x, y, 80, 720, 0, 77, 0, ENEMY)
    {
        InitializeAnimations();
        RegisterCharacter(this);
    }
    ~Lighting() override {
        // 在敌人析构时自动注销
        Character::UnregisterCharacter(this);
    }
    //////////////敌人类中被设置为敌人//////////
    bool IsEnemy() const override { return true; }
    bool IsLighting() const override { return true; }
    bool IsBoss() const override { return false; }
    bool IsPlayer() const override { return false; }

};

//游戏物体类派生喷泉
class Fountain : public GameObject {
public:
    bool InitializeAnimations();
    Fountain(float x, float y) : GameObject(x, y, 0, 0) {
        layer = 0;
        SetCollisionEnabled(false); // 禁用碰撞


    }

    // 覆盖Draw方法确保绘制在底层（需在游戏循环中优先绘制）
};
//游戏物体类派生喷泉
class Leaf : public GameObject {
public:
    bool InitializeAnimations();
    void Update(float deltaTime);
    Leaf(float x, float y) : GameObject(x, y, 0, 0) {

        SetCollisionEnabled(false); // 禁用碰撞

    }

// 覆盖Draw方法确保绘制在底层（需在游戏循环中优先绘制）
};
class Cloud : public GameObject {
public:
    bool InitializeAnimations()
    {
        if (!LoadAnimation(IDLE, { "Cloud_as.bmp" }, 99.0f))
            return false;
        return true;
    }
    void SetPosition(float newX, float newY)
    {
        x = newX;
        y = newY;
    }
    Cloud(float x, float y) : GameObject(x, y, 0, 0) {

        SetCollisionEnabled(false); // 禁用碰撞

    }
    // 覆盖Draw方法确保绘制在底层（需在游戏循环中优先绘制）
};
//
class Projectile : public Character {

public:
    static BGMPlayer* se;
    int damage;  // 伤害值

    // 构造函数增加 vx 和 vy 参数，用于设置初始飞行方向与速度
    Projectile(float startX, float startY, float w, float h,
        float offsetX = 0, float offsetY = 0,
        int dmg = 10,
        float initialVx = 0, float initialVy = 0)
        : Character(startX, startY, w, h, offsetX, offsetY, NEUTRAL), damage(dmg)
    {
        // 设置初始速度
        vx = initialVx;
        vy = initialVy;
        // 根据 vx 自动确定 facingRight（vx >= 0 则认为朝右，否则朝左）
        facingRight = (vx >= 0);

        // 飞行道具一般不需要受到其他伤害，所以可以禁用
        canTakeDamage = false;
        // 其他属性可根据需要调整
        state = IDLE;  // 或者可以自定义一个状态，比如 PROJECTILE
        InitializeAnimations();
    }
    ~Projectile() {};
    bool InitializeAnimations();
    // 重写 Update 方法：实现飞行、碰撞检测和自动删除
    void Update(float deltaTime) override;

    
    // 重写 Draw 方法（如有需要）
    //void Draw() override;
};


class PlayerProjectile : public Projectile
{
public:
    // 使用与基类相同的构造函数参数
    PlayerProjectile(float startX, float startY, float w, float h,
        float offsetX, float offsetY,
        int dmg,
        float initialVx, float initialVy)
        : Projectile(startX, startY, w, h, offsetX, offsetY, dmg, initialVx, initialVy)
    {
        // 这里可以再做一些玩家弹专属的初始化
        // 例如：贴图改用别的子弹纹理、速度更快/更慢等
        // 也可写 PlayerProjectile::InitializeAnimations() 覆盖
    }
    bool InitializeAnimations();
    // 覆盖 Update 方法，以便撞到敌人时伤害
    void Update(float deltaTime) override;
    bool IsBullet()const override{ return true; };
};


class SpikeTrap : public GameObject {
public:
    SpikeTrap(float x, float y) : GameObject(x, y, 32, 32) {
        LoadAnimation(IDLE, { "spike_1.bmp" }, 0.2f);
    }

    void Update(float deltaTime) override {
        GameObject::Update(deltaTime);

        // 检测玩家碰撞
        auto collided = CheckCollisionWithOthers();
        for (Character* other : collided) {
            if (auto player = dynamic_cast<Player*>(other)) {
                player->TakeDamage(10, GetHitboxCenterX()); // 造成伤害
            }
        }
    }
};

class TreasureChest : public GameObject {
private:
    int type = 0;
    static BGMPlayer* se;
public:
    bool isOpen = false;
    TreasureChest(float x, float y, float w, float h,int type);

    // 重写 IsEnemy()，宝箱不视作敌人
    virtual bool IsEnemy() const override { return false; }

    // 开启宝箱（触发奖励逻辑等）
    void Open();

    virtual void Update(float deltaTime) override;
    bool InitializeAnimations()
    {
        if (!LoadAnimation(IDLE, { "Chest_open_1.bmp" }, 999))
            return false;
        if (!LoadAnimation(HURT, { "Chest_open_1.bmp" }, 0.2))
            return false;
        if (!LoadAnimation(ATTACKING, { "Chest_open_1.bmp","Chest_open_2.bmp","Chest_open_3.bmp","Chest_open_4.bmp","Chest_open_5.bmp","Chest_open_6.bmp","Chest_open_7.bmp", }, 0.2f))
            return false;
        if (!LoadAnimation(DEAD, { "Chest_open_7.bmp" }, 999))
            return false;
        return true;
    };
};

class FallingChest : public TreasureChest {
private:
    bool isKnockedBack = false; // 标记是否已被击退，防止重复触发
public:
    // 构造函数采用传入宽高，便于重生时调整
    FallingChest(float x, float y, float w, float h)
        : TreasureChest(x, y, w, h,1) {
        // 可根据需要初始化其它状态
    }

    // 当宝箱开启时，同时停止运动
     void Open()  {
        TreasureChest::Open();
        vx = 0;
        vy = 0;
    }

    // 重写 Update 方法
    virtual void Update(float deltaTime) override {
        x = x + vx * deltaTime;
        y = y + vy * deltaTime;
        // 如果宝箱未开启，则先处理物理更新（例如受击、重力、移动等）
        if (!isOpen) {
            // 调用基础物理更新
            Character::Update(deltaTime);

            // 检测子弹碰撞（仅在未开启时）
            for (Character* proj : Character::allCharacters) {
                // 只对 Projectile 类型的对象进行检测
                if (dynamic_cast<Projectile*>(proj) == nullptr)
                    continue;

                // 进行简单的 AABB 碰撞检测
                float leftA = GetX();
                float topA = GetY();
                float rightA = leftA + GetWidth();
                float bottomA = topA + GetHeight();

                float leftB = proj->GetX();
                float topB = proj->GetY();
                float rightB = leftB + proj->GetWidth();
                float bottomB = topB + proj->GetHeight();

                if (leftA < rightB && rightA > leftB &&
                    topA < bottomB && bottomA > topB) {
                    // 根据子弹中心位置应用击退
                    ApplyKnockback(proj->GetHitboxCenterX());
                    isKnockedBack = true;  // 标记已击退
                    break;
                }
            }
        }
        // 如果宝箱已开启，则保证状态更新为已开启状态
        if (isOpen) {
            state = DEAD;  // 这里 DEAD 状态表示已开启（你也可以定义 OPENED 状态）
            currentFrame = 0;
            vx = 0;
            vy = 0;
        }
        // 最后调用父类更新（例如更新动画）
        TreasureChest::Update(deltaTime);
    }

    // 标记此对象为 FallingChest 类型（可用于调试或后续特殊处理）
    virtual bool IsFallingChest() const override { return true; }
};

class TriggerObject : public GameObject {
protected:
    bool hasTriggered = false;
    static BGMPlayer* se;
    bool isPotion = false;
public:
    TriggerObject(float x, float y, float w, float h);

    // 触发器不视作敌人
     bool IsEnemy() const override { return false; }

    // 触发事件，子类可重写
    virtual void OnTrigger(Character* c);

    void Update(float deltaTime) ;
};

class RedPotion :public TriggerObject
{
public:
    bool InitializeAnimations()
    {
        if (!LoadAnimation(IDLE, { "Potion_red.bmp" }, 999))
            return false;
        return true;
    };
    void OnTrigger(Character* c) override
    {
        c->TakeHealth(20);
    }
    RedPotion(float x, float y) :TriggerObject(x, y, 32, 32)
    {
        se->Stop();
        InitializeAnimations();
        RegisterCharacter(this);
    };
};
class BluePotion :public TriggerObject
{
public:
    bool InitializeAnimations()
    {
        if (!LoadAnimation(IDLE, { "Potion_blue.bmp" }, 999))
            return false;
        return true;
    };
    void OnTrigger(Character* c) override
    {
        c->TakeMP(20);
    }
    BluePotion(float x, float y) :TriggerObject(x, y, 32, 32) 
    {
        se->Stop();
        InitializeAnimations();
        RegisterCharacter(this);
    };

};

class Key :public TriggerObject
{
public:
    bool InitializeAnimations()
    {
        if (!LoadAnimation(IDLE, { "key.bmp" }, 999))
            return false;
        return true;
    }
    void OnTrigger(Character* c) override
    {
        c->hasKey=true;
    }
    Key(float x, float y) :TriggerObject(x, y, 60, 60)
    {
        InitializeAnimations();
        RegisterCharacter(this);
    }

};

class NormalKey :public Key
{
public:
    bool InitializeAnimations()
    {
        if (!LoadAnimation(IDLE, { "NormalKey.bmp" }, 999))
            return false;
        return true;
    }
    void OnTrigger(Character* c) override
    {
        c->hasNormalKey = true;
    }
    NormalKey(float x, float y) :Key(x, y)
    {
        InitializeAnimations();
        RegisterCharacter(this);
    }
};
class Wing:public Key
{
public:
    bool InitializeAnimations()
    {
        if (!LoadAnimation(IDLE, { "Wing.bmp" }, 999))
            return false;
        return true;
    }
    void OnTrigger(Character* c) override
    {
        c->canUseDash = true;
    }
    Wing(float x, float y) :Key(x, y)
    {
        InitializeAnimations();
        RegisterCharacter(this);
    }
};
class MagicBall :public Key
{
public:
    bool InitializeAnimations()
    {
        if (!LoadAnimation(IDLE, { "player_bullet.bmp" }, 999))
            return false;
        return true;
    }
    void OnTrigger(Character* c) override
    {
        c->canShoot = true;
    }
    MagicBall(float x, float y) :Key(x, y)
    {
        width= 32;
        height = 32;
        InitializeAnimations();
        RegisterCharacter(this);
    }
};
class Trophy :public Key
{
public:
    bool InitializeAnimations()
    {
        if (!LoadAnimation(IDLE, { "trophy.bmp" }, 999))
            return false;
        return true;
    }
    void OnTrigger(Character* c) override
    {
        c->gameClear= true;
    }
    Trophy(float x, float y) :Key(x, y)
    {
        width= 80;
        height = 80;
        InitializeAnimations();
        RegisterCharacter(this);
    }
};

class Lock :public TriggerObject
{
private:
    float destoryTimer = 0.0f;
    float destoryDuration = 1.5f;
public:
    static BGMPlayer* openSE;
    bool isOpen = false;
    bool InitializeAnimations()
    {
        if (!LoadAnimation(IDLE, { "Lock_1.bmp" }, 999))
            return false;
        if (!LoadAnimation(DEAD, { "Lock_2.bmp" }, 999))
            return false;

        return true;
    }
    void OnTrigger(Character* c) override
    {
        if (!c->hasKey)
            return;
        isOpen = true;  // 新增这行
        state = DEAD;
        openSE->Play(false);
    }
    Lock(float x, float y) :TriggerObject(x, y, 80, 80)
    {
        layer = 0;
        se->Stop();
        InitializeAnimations();
        RegisterCharacter(this);
    }
    void Update(float deltaTime)
    {
        // 如果宝箱还未开启，则检测与玩家的碰撞
        if (!isOpen) {
            // 遍历全局角色列表，检查玩家是否碰撞
            for (Character* c : Character::allCharacters) {
                if (c->IsPlayer() && c->hasKey) {
                    // 简单AABB碰撞检测
                    float leftA = GetX()-5;
                    float topA = GetY()-5;
                    float rightA = leftA + GetWidth()+5;
                    float bottomA = topA + GetHeight()+5;

                    float leftB = c->GetHitboxX();
                    float topB = c->GetHitboxY();
                    float rightB = leftB + c->GetWidth();
                    float bottomB = topB + c->GetHeight();

                    if (leftA < rightB && rightA > leftB && topA < bottomB && bottomA > topB) {
                        OnTrigger(c);
                        break;
                    }
                }
            }
        }
        if (isOpen)
        {
            state = DEAD;
            destoryTimer += deltaTime;
            if (destoryTimer >= destoryDuration)
            {
                markForDeletion = true;
            }
            currentFrame = 0;
        }
        UpdateAnimation(deltaTime);
        // 自身不需要其它逻辑
    }
};
class NormalLock :public Lock

{
private:
    float destoryTimer = 0.0f;
    float destoryDuration = 1.5f;
public:
    bool InitializeAnimations()
    {
        if (!LoadAnimation(IDLE, { "NormalLock_1.bmp" }, 999))
            return false;
        if (!LoadAnimation(DEAD, { "NormalLock_2.bmp" }, 999))
            return false;

        return true;
    }
    void OnTrigger(Character* c)
    {
        if (!c->hasNormalKey)
            return;
        isOpen = true;  // 新增这行
        state = DEAD;
        openSE->Play(false);
    }
    NormalLock(float x, float y) :Lock(x, y) 
    {
        layer = 0;
        InitializeAnimations();
        RegisterCharacter(this);
    };
    void Update(float deltaTime)
    {
        // 如果宝箱还未开启，则检测与玩家的碰撞
        if (!isOpen) {
            // 遍历全局角色列表，检查玩家是否碰撞
            for (Character* c : Character::allCharacters) {
                if (c->IsPlayer() && c->hasNormalKey) {
                    // 简单AABB碰撞检测
                    float leftA = GetX() - 5;
                    float topA = GetY() - 5;
                    float rightA = leftA + GetWidth() + 5;
                    float bottomA = topA + GetHeight() + 5;

                    float leftB = c->GetHitboxX();
                    float topB = c->GetHitboxY();
                    float rightB = leftB + c->GetWidth();
                    float bottomB = topB + c->GetHeight();

                    if (leftA < rightB && rightA > leftB && topA < bottomB && bottomA > topB) {
                        OnTrigger(c);
                        break;
                    }
                }
            }
        }
        if (isOpen)
        {
            state = DEAD;
            destoryTimer += deltaTime;
            if (destoryTimer >= destoryDuration)
            {
                markForDeletion = true;
            }
            currentFrame = 0;
        }
        UpdateAnimation(deltaTime);
        // 自身不需要其它逻辑
    }

};
// BulletEmitter 继承自 GameObject，不需要参与碰撞检测
class BulletEmitter : public GameObject {
public:
    // 目标（比如主角），发射子弹时子弹会朝向这个目标飞行
    Character* target;  // 使用弱引用
    // 子弹飞行速度（像素/帧）
    float bulletSpeed;
    // 子弹伤害
    int bulletDamage;
    // 每次发射子弹的数量范围：最少 minCount，最多 maxCount
    int minBulletCount;
    int maxBulletCount;
    // 发射间隔（秒）
    double fireInterval;
    // 内部计时器，累加每帧的 deltaTime
    double fireTimer;
    std::vector<std::shared_ptr<Projectile>> bullets;
    bool isActive = false;
    void SetActive(bool newBool)
    {
        isActive = (newBool);
    }
    // 构造函数
    // 参数说明：
    //   x, y：发射器的初始位置
    //   target：目标角色（发射子弹时锁定目标方向）
    //   bulletSpeed：子弹飞行速度
    //   bulletDamage：子弹伤害
    //   minBulletCount, maxBulletCount：每次发射的子弹数量范围
    //   fireInterval：发射间隔（秒）
   // 构造函数，改为接受 std::weak_ptr<Character> 或直接传入 std::shared_ptr<Character>
    BulletEmitter(float x, float y, Character* target,
        float bulletSpeed = 12.0f, int bulletDamage = 10,
        int minBulletCount = 1, int maxBulletCount = 3,
        double fireInterval = 0.5)
        : GameObject(x, y, 10, 10, 0, 0),
        target(target),
        bulletSpeed(bulletSpeed),
        bulletDamage(bulletDamage),
        minBulletCount(minBulletCount),
        maxBulletCount(maxBulletCount),
        fireInterval(fireInterval),
        fireTimer(0.0)
    {
        SetCollisionEnabled(false);
        RegisterCharacter(this);
        markForDeletion = false;
    }
    ~BulletEmitter()
    {
        bullets.clear();
        UnregisterCharacter(this);
    }
    // 如果不需要动画，可以简单返回 true
    bool InitializeAnimations()
    {
        if (!LoadAnimation(IDLE, { "" }, 999.0f))
            return false;
        return true;
    }

    virtual void Update(float deltaTime) override {
        if (isActive == false) return;
        // 先检查目标是否存在
        auto tgt = target;
        if (!tgt) {
            markForDeletion = true;
            return;
        }

        // 累加发射计时器
        fireTimer += deltaTime;
        if (fireTimer >= fireInterval) {
            fireTimer = 0.0;
            FireBullets();
        }

        // 更新并绘制内部存储的子弹
        // 这里遍历 bullets 容器，对每个子弹调用 Update() 和 Draw()
        // 并检查是否需要删除已标记删除的子弹
        for (auto it = bullets.begin(); it != bullets.end(); ) {
            (*it)->Update(deltaTime);
            (*it)->Draw();
            if ((*it)->IsMarkedForDeletion()) {
                // 如果子弹已经标记删除，则从容器中移除
                it = bullets.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    bool IsEnemy() const { return false; }

    // 发射子弹函数：以 BulletEmitter 的碰撞体中心作为发射点，
    // 根据目标的位置计算单位方向向量，并创建一定数量的 Projectile 对象
    void FireBullets() {
        // 发射点
        float shootX = GetHitboxCenterX();
        float shootY = GetHitboxCenterY();

        auto tgt = target;
        if (!tgt) return;  // 如果目标已不存在，则直接返回

        float targetX = tgt->GetHitboxCenterX();
        float targetY = tgt->GetHitboxCenterY();

        float dx = targetX - shootX;
        float dy = targetY - shootY;
        float length = std::sqrt(dx * dx + dy * dy);
        if (length == 0) length = 1;
        float unitX = dx / length;
        float unitY = dy / length;

        int bulletCount = 1;
        for (int i = 0; i < bulletCount; i++) {
            // 给每个子弹增加一个微小的随机偏移（例如位置上）
            float offsetX = ((rand() % 11) - 5) * 0.5f;  // 范围大约为 -2.5 到 2.5 像素
            float offsetY = ((rand() % 11) - 5) * 0.5f;
            float bulletStartX = shootX + offsetX;
            float bulletStartY = shootY + offsetY;

            float vx_proj = bulletSpeed * unitX;
            float vy_proj = bulletSpeed * unitY;
            // 使用 std::make_shared 创建子弹对象
            {
                auto proj = std::make_shared<Projectile>(bulletStartX, bulletStartY, 28, 28, 2, 2, bulletDamage, vx_proj, vy_proj);
                // 如果 Projectile 构造函数内部已经调用 RegisterCharacter(this)，请注意可能会产生重复注册问题。
                // 建议 Projectile 仅由发射器管理，不再注册到全局列表，或确保全局列表中不会出现重复项。
                bullets.push_back(proj);
            }

        }
    }
};


class UI : public GameObject
{

private:
    float targetHealth = 100;
public:
    static void RegisterUI(UI* ui);
    static std::vector<UI*> UIs; 
    Bmp* bar_01=nullptr;
    Bmp* bar_02=nullptr;
    Bmp* bar_03=nullptr;
    std::vector<int> bars;
    Character* target=nullptr;
    float oneBlock;
    int blockNum=0;
    int prvBlockNum=0;
    bool InitializeAnimations()
    {
        if (!LoadAnimation(IDLE, { "Player_Sprite.bmp" }, 99.0f))
            return false;
        return true;
    }

    UI(float x,float y):GameObject(x,y,0,0)
    {
        bar_01 = LoadBmp("pic/Player_Sprite_bar_1.bmp", false);
        bar_02 = LoadBmp("pic/Player_Sprite_bar_2.bmp", false);
        bar_03 = LoadBmp("pic/Player_Sprite_bar_3.bmp", false);
        for (int i=0;i<175;i++)
        {
            if (i == 0)
            {
                bars.push_back(0);
            }
            else if (i == 173)
            {
                bars.push_back(2);
            }
            else
            {
                bars.push_back(1);
            }

        }
        if (target == nullptr)
        {
            for (Character* c : Character::allCharacters) {
                if (c->IsPlayer()) {
                    target = c;
                    break;
                }
            }
        }
        oneBlock = targetHealth / 175;
        InitializeAnimations();
         RegisterUI(this);
    }
    void UpdateUI(Character* target)
    {
        prvBlockNum = blockNum;
        targetHealth=target->GetHealth();
        if (targetHealth < 0)
            return;
         blockNum = targetHealth / oneBlock;
        if (blockNum < bars.size())
        {
            for (int i = 0; i <= (bars.size() - blockNum); i++)
            {
                bars.pop_back();
            }
        }
        else if (blockNum > bars.size())
        {
            for (int i = 0; i <= (blockNum-bars.size() ); i++)
            {
                if (bars.size() <= 0)
                {
                    bars.push_back(0);
                }
                else
                {
                bars.push_back(1);
                }
            }
        }
        for (int i = 0; i < (bars.size()); i++)
        {
            if (bars[i] == 0)
            {
                DrawBmp(x, y, bar_01, true);
            }
            else if(bars[i] == 1)
            {
                DrawBmp(x+i*2, y, bar_02, true);
            }
        }
    }
    void Update(float deltaTime)
    {
        if (target == nullptr)
        {
            for (Character* c : Character::allCharacters) {
                if (c->IsPlayer()) {
                    target = c;
                    break;
                }
            }
        }
        UpdateUI(target);
        UpdateAnimation(deltaTime);
    }

};

class MPUI :public UI
{

private:
    float targetMP = 100;
public:
    static void RegisterUI(MPUI* ui);
    Bmp* bar_01 = nullptr;
    Bmp* bar_02 = nullptr;
    Bmp* bar_03 = nullptr;
    std::vector<int> bars;
    Character* target = nullptr;
    float oneBlock;
    bool InitializeAnimations()
    {
        if (!LoadAnimation(IDLE, { "Player_Sprite_mpBar.bmp" }, 99.0f))
            return false;
        return true;
    }

    MPUI(float x, float y) :UI(x, y)
    {
        bar_01 = LoadBmp("pic/Player_Sprite_mpBar_1.bmp", 0);
        bar_02 = LoadBmp("pic/Player_Sprite_mpBar_2.bmp", 0);
        bar_03 = LoadBmp("pic/Player_Sprite_mpBar_3.bmp", 0);
        for (int i = 0; i < 175; i++)
        {
            if (i == 0)
            {
                bars.push_back(0);
            }
            else if (i == 173)
            {
                bars.push_back(2);
            }
            else
            {
                bars.push_back(1);
            }

        }
        if (target == nullptr)
        {
            for (Character* c : Character::allCharacters) {
                if (c->IsPlayer()) {
                    target = c;
                    break;
                }
            }
        }
        oneBlock = targetMP / 175;
        InitializeAnimations();
        RegisterUI(this);
    }
    void UpdateUI(Character* target)
    {
        prvBlockNum = blockNum;
        targetMP = target->GetMP();
        if (targetMP < 0)
            return;
         blockNum = targetMP / oneBlock;
        if (blockNum < bars.size())
        {
            for (int i = 0; i <= (bars.size() - blockNum); i++)
            {
                bars.pop_back();
            }
        }
        else if (blockNum > bars.size())
        {
            for (int i = 0; i <= (blockNum - bars.size()); i++)
            {
                if (bars.size() <= 0)
                {
                    bars.push_back(0);
                }
                else
                {
                    bars.push_back(1);
                }
            }
        }
        for (int i = 0; i < (bars.size()); i++)
        {
            if (bars[i] == 0)
            {
                DrawBmp(x, y, bar_01, true);
            }
            else if (bars[i] == 1)
            {
                DrawBmp(x + i * 2, y, bar_02, true);
            }
        }
    }
    void Update(float deltaTime)
    {
        if (target == nullptr)
        {
            for (Character* c : Character::allCharacters) {
                if (c->IsPlayer()) {
                    target = c;
                    break;
                }
            }
        }
        UpdateUI(target);
        UpdateAnimation(deltaTime);
    }

};


class BossUI : public UI
{
    //97
    //758
    //2
private:
    float targetHP = 500;
public:
    static void RegisterUI(BossUI* ui);
    Bmp* bar_01 = nullptr;
    std::vector<int> bars;
    Character* target = nullptr;
    float oneBlock;
    bool InitializeAnimations()
    {
        if (!LoadAnimation(IDLE, { "Boss_Sprite_1.bmp" }, 99.0f))
            return false;
        return true;
    }

    BossUI(float x, float y) :UI(x, y)
    {
        bar_01 = LoadBmp("pic/Boss_Sprite_2.bmp", 0);
        for (int i = 0; i < 379; i++)
        {
                bars.push_back(1);
        }
        if (target == nullptr)
        {
            for (Character* c : Character::allCharacters) {
                if (c->IsBoss()) {
                    target = c;
                    break;
                }
            }
        }
        oneBlock = targetHP / 379;
        InitializeAnimations();
        RegisterUI(this);
    }
    void UpdateUI(Character* target)
    {
        prvBlockNum = blockNum;
        targetHP = target->GetHealth();
        if (targetHP < 0)
            return;
        blockNum = targetHP / oneBlock;
        if (blockNum < bars.size())
        {
            for (int i = 0; i <= (bars.size() - blockNum); i++)
            {
                bars.pop_back();
            }
        }
        else if (blockNum > bars.size())
        {
            for (int i = 0; i <= (blockNum - bars.size()); i++)
            {
                    bars.push_back(1);
            }
        }
        for (int i = 0; i < (bars.size()); i++)
        {
            if (bars[i] == 1)
            {
                DrawBmp(x + 97 + (i * 2), y, bar_01, true);
            }
        }
    }
    void Update(float deltaTime)
    {
        if (target == nullptr)
        {
            for (Character* c : Character::allCharacters) {
                if (c->IsBoss()) {
                    target = c;
                    break;
                }
            }
        }
        UpdateUI(target);
        UpdateAnimation(deltaTime);
    }

};


class Boss : public Enemy {
private:
        Cloud* attackCloud; // 预加载的云对象
        std::unique_ptr <BulletEmitter> emitter=nullptr;
        Character* player = nullptr;
        std::vector<Enemy*> enemies;
        BossUI* bossUI = nullptr;
        static BGMPlayer* se;
public:
    int phase;           // 阶段
    float patrolSpeed;      // 巡逻速度
    float chaseSpeed;       // 追击速度
    float retreatSpeed;     // 后退速度
    float specialAttackCooldown;  // 特殊攻击的冷却时间
    float specialAttackTimer;     // 特殊攻击计时器
    float stateLockTimer = 0;     // 攻击计时器
    // 预设各个距离阈值（可根据需求调整）
    float chaseThreshold = 300.0f;    // 超过此距离，倾向追击
    float retreatThreshold = 150.0f;  // 低于此距离，倾向后退
    float attackCooldowns[3] = { 0 }; // ATTACKING, ATTACKING2, ATTACKING3的冷却时间
    float actionCooldown = 0;       // 通用行动冷却
    float stateSwitchBuffer = 0.5f; // 状态切换缓冲时间
    float lastStateSwitchTime = 0.0f; // 上次状态切换的时间
    float phaseTimer = 0.0f; // 上次状态切换的时间
    float thunderTimer = 0.0f; // 上次状态切换的时间
    float thunderDuration = 1.5f; // 上次状态切换的时间
    float phase2Duration = 20.0f; // 上次状态切换的时间
    float phase3Duration = 20.0f; // 上次状态切换的时间
    float enemySpawnDuration = 1.0f; // 上次状态切换的时间
    float enemySpawnTimer = 0.0f; // 上次状态切换的时间

    // 预设基础攻击权重
    const float baseAttackWeight = 1.0f;

    // 构造函数（注意调用基类 Enemy 的构造函数）

    Boss(float startX, float startY, float w, float h, float range, float offsetX, float offsetY, Type type = ENEMY);

    ~Boss() = default;

    // 重载动画初始化函数
    bool InitializeAnimations();

    // 重载 Update 方法，实现 Boss 特有的行为
    void Update(float deltaTime) override;

    // 重载受击函数，可以在 Boss 受击时添加特殊效果
    void TakeDamage(int damage, float attackerX) override;
    float timer = 0;

    void Thunder(Character* target);
    // 攻击方法
    bool IsAttackState(CharacterState state)
    {
        if (state == ATTACKING || state == ATTACKING2 || state == ATTACKING3)
            return true;
        else return false;
    }
    bool IsPlayerInFront(Character* player) {
        float dx = player->GetHitboxCenterX() - this->GetHitboxCenterX();
        return (dx > 0 && facingRight) || (dx < 0 && !facingRight);
    }
    // 添加动画时长获取方法
    float GetStateDuration(CharacterState state) {
        auto& anim = animations[state];
        return anim.frames.size() * anim.frameTime;
    }
    // 移动方法
    void ChasePlayer(float deltaTime, Character* player);
    void RetreatFromPlayer(float deltaTime, Character* player);
    void Patrol(float deltaTime);

    // 辅助方法：计算与玩家的距离
    float DistanceTo(Character* target);

    void DecisionMaking(float deltaTime, Character* player);
    float CalculateChaseWeight(float dist, Character* player);
    float CalculateAttackWeight(float dist, bool inFront);
    float CalculateRetreatWeight(float dist);
    void ChooseAttackType(float dist, bool inFront);
    bool IsPlayerInAttackRange(Character* player, CharacterState attackType);
    void SpawnEnemies();
    bool IsBoss() const override { return true; }
    bool IsPlayer() const override { return false; }
    
};



// ========== 键盘输入函数 ==========
bool GetKeyDown(int key);
bool GetKeyUp(int key);
bool GetKeyHold(int key);

// ========== 键盘输入函数定义 ==========
extern bool g_keyStates[256];
extern bool g_prevKeyStates[256];

void UpdateKeyStates();
void ClearKeyStates();


#endif // GAME_CHARACTER_H