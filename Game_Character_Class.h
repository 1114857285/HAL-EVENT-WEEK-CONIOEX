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
// ��ɫ����״̬
enum CharacterState {
    IDLE, RUNNING, JUMPING, FALLING, ATTACKING,ATTACKING2,ATTACKING3,HURT,DEAD,FREEZE, CHASE, RETREAT
};

// �����ṹ��
struct Animation {
    std::vector<std::shared_ptr<Bmp>> frames;
    float frameTime;
    Animation() : frameTime(0.2f) {}
};

// �򵥵� BGM ��������
class BGMPlayer {
private:
    int* soundHandle;    // �� MciOpenSound ���ص��������
    bool isLoop;         // �Ƿ�ѭ������
    float delayTimer;    // �ӳٲ��ż�ʱ������λ��
    bool delayed;        // �Ƿ������ӳٲ���
    bool OnPlay;         //�Ƿ����ڲ���

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
        // ���ǰ��·�������� "sound/"��Ȼ�������Ӵ���� path
        std::string fullPath = "sound/";
        fullPath += path;
        soundHandle = MciOpenSound(fullPath.c_str());
        RegisterBGM(this);
    }

    // ����������ֹͣ���Ų��رվ��
    ~BGMPlayer() {
        Stop();
        if (soundHandle) {
            MciCloseSound(soundHandle);
            soundHandle = nullptr;
        }
        UnRegisterBGM(this);
    }

    // �������������� loop ��ʾ�Ƿ�ѭ�����ţ�delaySeconds ָ���ӳٲ��ŵ�ʱ�䣨��λ�룩
    void Play(bool loop = false, float delaySeconds = 0.0f) {
        if (!soundHandle||OnPlay) return;
        isLoop = loop;
        if (delaySeconds > 0.0f) {
            delayTimer = delaySeconds;
            delayed = true;
        }
        else {
            // ����֮ǰ�Ȼص���ʼλ��
            MciPlaySound(soundHandle, loop ? 1 : 0);
        }
        OnPlay = true;
    }

    // ֹͣ����
    void Stop() {
        if (!soundHandle||!OnPlay) return;
        MciStopSound(soundHandle);
        delayed = false;
        delayTimer = 0.0f;
        OnPlay = false;
    }

    // ÿ֡���£����ڴ����ӳٲ����Լ�ѭ�����ŵĸ���
    void Update(float deltaTime) {
        if (!soundHandle) return;
        // �����ӳٲ����߼�
        if (delayed) {
            delayTimer -= deltaTime;
            if (delayTimer <= 0.0f) {
                delayed = false;
                MciPlaySound(soundHandle, isLoop ? 1 : 0);
            }
        }
        // �����ѭ�����ţ����鲥��״̬��������ȷ��ѭ��
        if (isLoop) {
            MciUpdateSound(soundHandle);
        }
        if (!isLoop && !delayed && !MciCheckSound(soundHandle)) {
            OnPlay = false;
        }

    }

    // ���ò���������0~100��
    void SetVolume(int percent) {
        if (!soundHandle) return;
        MciSetVolume(soundHandle, percent);
    }

    // ����Ƿ����ڲ��ţ����� true ��ʾ���ڲ��ţ�
    bool IsPlaying() const {
        if (!soundHandle) return false;
        return MciCheckSound(soundHandle) != 0;
    }
};

// ������ײ��ṹ��
struct AttackCollider {
    float x, y;          // ��ײ��λ��
    float width, height; // ��ײ���С
    bool active = false; // �Ƿ񼤻�
};

// ͨ�ý�ɫ����
class Character {
protected:
    float x, y;           // λ��
    float vx, vy;         // �ٶ�
    float width, height;  // ��ײ�д�С
    float hitboxOffsetX;  // ��ײ��X��ƫ��
    float hitboxOffsetY;  // ��ײ��Y��ƫ��
    bool isOnGround;      // �Ƿ��ڵ�����
    bool facingRight = true; // �Ƿ������Ҳ�
    CharacterState state; // ��ǰ״̬
    Animation animations[12]; // ÿ��״̬�Ķ���
    std::atomic<int> currentFrame;     // ��ǰ֡����
    float frameCounter;      // ֡��ʱ��
    float gravity = 0.5f; // �������ٶ�
    std::mutex animationMutex;
    AttackCollider attackCollider; // ������ײ��
    Character* target = nullptr;   // ����Ŀ��

    int health = 100;             // ����ֵ
    int mp = 100;             // ����ֵ
    bool canTakeDamage = true;    // �˺�������ȴ
    float damageCooldown = 0.5f;  // �˺���ȴʱ��


    bool isAttackActive = false;       // ��ǰ������ײ���Ƿ񼤻�
    std::unordered_set<Character*> hitTargets; // ��ǰ����֡�����е�Ŀ��
    bool hasTriggeredThisFrame = false; // ��Ǳ�֡�Ƿ��Ѵ����˺�����ֹ��һ�ι���֡���ж�δ����ܻ����ܣ�
    int previousAnimationFrame = -1;  // ��¼��һ֡�Ķ�������

    CharacterState previousState = IDLE; // ��¼��һ֡״̬

    // �����������
    float knockbackForceX = 5.0f;    // ˮƽ������
    float knockbackForceY = 1.0f;    // ��ֱ������
    float hurtDuration = 0.2;       // �ܻ�״̬����ʱ��
    float hurtTimer = 0.0f;          // �ܻ�״̬��ʱ��
    float invincibilityTimer = 0.0f;
    float invincibilityDuration = 0.4f;
    float freezeTimer = 0.0f;          // �ܻ�״̬��ʱ��
    float freezeDuration = 2.0f;          // freeze״̬��ʱ��
    bool isKnockback = false;      // ����״̬���
    bool canKnockback = true;      // ����״̬���
    float knockbackDuration = 0.2f; // ���˳���ʱ��
    float canKnockbackDuration = 0.2f; // ���˳���ʱ��
    float knockbackTimer = 0.0f;
    float canKnockbackTimer = 0.0f;
    float cureTimer = 0.0f;
    float cureDuration = 1.0f;

    // === ���ϵͳ ===
    float dashSpeed = 12.0f;    // 25����/֡ 
    float dashDuration = 0.2f;  // �����ʱ��
    float dashCooldown = 0.8f;  // ��ȴʱ��
    float dashTimer = 0.0f;         // ��̼�ʱ��
    float dashCooldownTimer = 0.0f; // ��ȴ��ʱ��
    bool isDashing = false;         // ���״̬
    bool canDash = true;            // �Ƿ�ɳ��
    int maxAirDashes = 1;           // �����г�̴���
    int remainingAirDashes = 1;     // ʣ����г�̴���
    bool wasOnGround = isOnGround;
    bool canGroundDash = true;
    struct DashTrail {
        float x, y;
        float alpha;
    };
    std::vector<DashTrail> dashTrails;
    float trailInterval = 0.0f;

    static GameMap* currentMap; // ������̬��ͼָ��

public:
    bool hasKey = 0;
    bool hasNormalKey = 0;
    bool canUseDash = 0;
    bool canShoot = 0;
    bool gameClear = 0;
    int layer = 1;
    bool hasDead = false;
    bool markForDeletion = false; // �Ƿ�Ӧ�ñ��ͷ��ڴ�(��ɫ����������ԭ���ٱ���Ҫ)
    enum Type { PLAYER, ENEMY, NEUTRAL };
    Type characterType = NEUTRAL;
    explicit Character(Type type) : characterType(type) {}
    Type GetCharacterType() const { return characterType; }
    bool collisionEnabled = true; // ������ײ����
    void SetCollisionEnabled(bool enabled) {
        collisionEnabled = enabled;
    }
    static void SetGameMap(GameMap* map) { currentMap = map; }
    static GameMap* GetCurrentMap() { return currentMap; }
    static void SetCurrentMap(GameMap* map) { currentMap = map; }
    void ResetAirSystem() { // �������÷���
        remainingAirDashes = maxAirDashes;
        canDash = true;
        canGroundDash = true;
        dashCooldownTimer = 0.0f;
    }
    virtual void UpdateSE()
    {
    };


    //ȫ�ֽ�ɫ�б���ɫ����ʱ�Զ��Ǽǵ��б��У�����ʱ�Զ���ɾ���б�
    static std::vector<Character*> allCharacters;

    ///////////////////////////��ɫ�๹�캯������������///////////////////////////
    Character(float startX, float startY, float w, float h, float offsetX = 0, float offsetY = 0, Type type=NEUTRAL);
    virtual ~Character()
    {
        // �Զ���ȫ���б��Ƴ�
        auto it = std::find(allCharacters.begin(), allCharacters.end(), this);
        if (it != allCharacters.end()) {
            allCharacters.erase(it);
        }
    }


    ////////////////////////////�������غ�������//////////////////////////////////////////
    bool LoadAnimation(CharacterState action, const std::vector<std::string>& framePaths, float frameTime);

    /////////////////////////��ɫ���ºͽ�ɫ�滭��������/////////////////////////////
    virtual void Update(float deltaTime);
    virtual void Draw();

    /////////////////////////��ɫ�������ʶ���/////////////////////////////
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
    /////////////////////////��ɫ�ܻ��Լ�����/////////////////////////////
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
    
    ////////////////////��ɫ���б�ǼǺ�ɾ������/////////////////////////
    bool IsMarkedForDeletion() const { return markForDeletion; } //������Ҫ��ɾ��ʱ�Ĳ��� ����
    static void RegisterCharacter(Character* character);
    static void UnregisterCharacter(Character* character);

    ///////////////////////////��ɫ����ʱ����������ײ���Լ����ù���Ŀ��ͻ�ȡ����Ŀ��///////////////////
    const AttackCollider& GetAttackCollider() const { return attackCollider; }
    void SetTarget(Character* newTarget) { target = newTarget; }
    Character* GetTarget() const { return target; }
    void ClearHitTargets() { hitTargets.clear(); } // ������м�¼

   
    ////////////////////// ͨ����ײ��ⷽ�����Ż��棩/////////////////////
  // ��ɫ������
    virtual bool IsEnemy() const { return false; }
    virtual bool IsPlayer() const { return false; }
    virtual bool IsBoss() const { return false; }
    virtual bool IsBullet() const { return false; }
    virtual bool IsFallingChest() const { return false; }
    virtual bool IsLighting() const { return false; }
    // ������ײ���Ĵ����ԣ�ֻ���𷵻���ײ����
    std::vector<Character*> CheckCollisionWithOthers() {
        if (!collisionEnabled) return {};
        std::vector<Character*> collidedCharacters;

        // ʹ�ø���ȷ����ײ�б�����
        const float thisLeft = GetHitboxX();
        const float thisTop = GetHitboxY();
        const float thisRight = thisLeft + GetWidth();
        const float thisBottom = thisTop + GetHeight();

        for (Character* other : allCharacters) {
            if (other == this||!other->collisionEnabled) continue;

            // �Ż����˻����߼���ͨ���麯��ʵ�ֶ�̬�ж�
            //if (this->IsEnemy() && other->IsEnemy()) continue;

            // ������ײ�м����߼�
            const float otherLeft = other->GetHitboxX();
            const float otherTop = other->GetHitboxY();
            const float otherRight = otherLeft + other->GetWidth();
            const float otherBottom = otherTop + other->GetHeight();

            // ��������AABB��ײ����
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

////////////////���˺���///////////////////////
void ApplyKnockback(float attackerX) {
    // ������ڻ�����ȴ��Ͳ��ٴ����µĻ���
    if (!canKnockback||this->IsBoss()) {
        return;
    }

    // ������ʼ����
    float direction = (GetHitboxCenterX() < attackerX) ? -1.0f : 1.0f;
    vx = direction * knockbackForceX;  // ˮƽ�ٶ�
    vy = -knockbackForceY;            // ��һ�����Ϸ���
    isOnGround = false;               // ���

    // �������״̬
    isKnockback = true;
    knockbackTimer = knockbackDuration;

    // �ر��ٴλ��˵Ŀ�����
    canKnockback = false;
    canKnockbackTimer = canKnockbackDuration;
}
/////////////////��������ײ�弤��ʱ���ã�ÿ�ι���֡��ʼʱ��/////////////////////
    void OnAttackFrameStart() {
        hitTargets.clear();  // �����ʷ���м�¼
        isAttackActive = true;
    }

////////////��������ײ��ر�ʱ���ã�ÿ�ι���֡����ʱ��///////////
    void OnAttackFrameEnd() {
        isAttackActive = false;
    }

/////////// ������ȫ����ʱ���ã�������������ʱ��////////////////////
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

//////////////////������///////////////////////
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
    // Player ��� InitializeAnimations ʵ��
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



        return true;    //ȫ�����غ󷵻�Ϊ�棬�¼Ӷ���Ҫ�ӵ��Ϸ�


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
////////////////////������//////////////////
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
        // �ڵ�������ʱ�Զ�ע��
        Character::UnregisterCharacter(this);
    }
    //////////////�������б�����Ϊ����//////////
    bool IsEnemy() const override { return true; }
    bool IsBoss() const override { return false; }
    bool IsPlayer() const override { return false; }
private:
    void Patrol();
};

/////////////��Ϸ������////////
class GameObject : public Character {
public:
    GameObject(float x, float y, float w, float h, float offsetX = 0, float offsetY = 0)
        : Character(x, y, w, h, offsetX, offsetY) {
        canTakeDamage = false; // Ĭ�ϲ��ɱ�����
    }

    // �����ƶ��͹���
    void Update(float deltaTime) override {
        UpdateAnimation(deltaTime); // �����¶���
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
        // �ڵ�������ʱ�Զ�ע��
        Character::UnregisterCharacter(this);
    }
    //////////////�������б�����Ϊ����//////////
    bool IsEnemy() const override { return true; }
    bool IsLighting() const override { return true; }
    bool IsBoss() const override { return false; }
    bool IsPlayer() const override { return false; }

};

//��Ϸ������������Ȫ
class Fountain : public GameObject {
public:
    bool InitializeAnimations();
    Fountain(float x, float y) : GameObject(x, y, 0, 0) {
        layer = 0;
        SetCollisionEnabled(false); // ������ײ


    }

    // ����Draw����ȷ�������ڵײ㣨������Ϸѭ�������Ȼ��ƣ�
};
//��Ϸ������������Ȫ
class Leaf : public GameObject {
public:
    bool InitializeAnimations();
    void Update(float deltaTime);
    Leaf(float x, float y) : GameObject(x, y, 0, 0) {

        SetCollisionEnabled(false); // ������ײ

    }

// ����Draw����ȷ�������ڵײ㣨������Ϸѭ�������Ȼ��ƣ�
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

        SetCollisionEnabled(false); // ������ײ

    }
    // ����Draw����ȷ�������ڵײ㣨������Ϸѭ�������Ȼ��ƣ�
};
//
class Projectile : public Character {

public:
    static BGMPlayer* se;
    int damage;  // �˺�ֵ

    // ���캯������ vx �� vy �������������ó�ʼ���з������ٶ�
    Projectile(float startX, float startY, float w, float h,
        float offsetX = 0, float offsetY = 0,
        int dmg = 10,
        float initialVx = 0, float initialVy = 0)
        : Character(startX, startY, w, h, offsetX, offsetY, NEUTRAL), damage(dmg)
    {
        // ���ó�ʼ�ٶ�
        vx = initialVx;
        vy = initialVy;
        // ���� vx �Զ�ȷ�� facingRight��vx >= 0 ����Ϊ���ң�������
        facingRight = (vx >= 0);

        // ���е���һ�㲻��Ҫ�ܵ������˺������Կ��Խ���
        canTakeDamage = false;
        // �������Կɸ�����Ҫ����
        state = IDLE;  // ���߿����Զ���һ��״̬������ PROJECTILE
        InitializeAnimations();
    }
    ~Projectile() {};
    bool InitializeAnimations();
    // ��д Update ������ʵ�ַ��С���ײ�����Զ�ɾ��
    void Update(float deltaTime) override;

    
    // ��д Draw ������������Ҫ��
    //void Draw() override;
};


class PlayerProjectile : public Projectile
{
public:
    // ʹ���������ͬ�Ĺ��캯������
    PlayerProjectile(float startX, float startY, float w, float h,
        float offsetX, float offsetY,
        int dmg,
        float initialVx, float initialVy)
        : Projectile(startX, startY, w, h, offsetX, offsetY, dmg, initialVx, initialVy)
    {
        // �����������һЩ��ҵ�ר���ĳ�ʼ��
        // ���磺��ͼ���ñ���ӵ������ٶȸ���/������
        // Ҳ��д PlayerProjectile::InitializeAnimations() ����
    }
    bool InitializeAnimations();
    // ���� Update �������Ա�ײ������ʱ�˺�
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

        // ��������ײ
        auto collided = CheckCollisionWithOthers();
        for (Character* other : collided) {
            if (auto player = dynamic_cast<Player*>(other)) {
                player->TakeDamage(10, GetHitboxCenterX()); // ����˺�
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

    // ��д IsEnemy()�����䲻��������
    virtual bool IsEnemy() const override { return false; }

    // �������䣨���������߼��ȣ�
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
    bool isKnockedBack = false; // ����Ƿ��ѱ����ˣ���ֹ�ظ�����
public:
    // ���캯�����ô����ߣ���������ʱ����
    FallingChest(float x, float y, float w, float h)
        : TreasureChest(x, y, w, h,1) {
        // �ɸ�����Ҫ��ʼ������״̬
    }

    // �����俪��ʱ��ͬʱֹͣ�˶�
     void Open()  {
        TreasureChest::Open();
        vx = 0;
        vy = 0;
    }

    // ��д Update ����
    virtual void Update(float deltaTime) override {
        x = x + vx * deltaTime;
        y = y + vy * deltaTime;
        // �������δ���������ȴ���������£������ܻ����������ƶ��ȣ�
        if (!isOpen) {
            // ���û����������
            Character::Update(deltaTime);

            // ����ӵ���ײ������δ����ʱ��
            for (Character* proj : Character::allCharacters) {
                // ֻ�� Projectile ���͵Ķ�����м��
                if (dynamic_cast<Projectile*>(proj) == nullptr)
                    continue;

                // ���м򵥵� AABB ��ײ���
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
                    // �����ӵ�����λ��Ӧ�û���
                    ApplyKnockback(proj->GetHitboxCenterX());
                    isKnockedBack = true;  // ����ѻ���
                    break;
                }
            }
        }
        // ��������ѿ�������֤״̬����Ϊ�ѿ���״̬
        if (isOpen) {
            state = DEAD;  // ���� DEAD ״̬��ʾ�ѿ�������Ҳ���Զ��� OPENED ״̬��
            currentFrame = 0;
            vx = 0;
            vy = 0;
        }
        // �����ø�����£�������¶�����
        TreasureChest::Update(deltaTime);
    }

    // ��Ǵ˶���Ϊ FallingChest ���ͣ������ڵ��Ի�������⴦��
    virtual bool IsFallingChest() const override { return true; }
};

class TriggerObject : public GameObject {
protected:
    bool hasTriggered = false;
    static BGMPlayer* se;
    bool isPotion = false;
public:
    TriggerObject(float x, float y, float w, float h);

    // ����������������
     bool IsEnemy() const override { return false; }

    // �����¼����������д
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
        isOpen = true;  // ��������
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
        // ������仹δ��������������ҵ���ײ
        if (!isOpen) {
            // ����ȫ�ֽ�ɫ�б��������Ƿ���ײ
            for (Character* c : Character::allCharacters) {
                if (c->IsPlayer() && c->hasKey) {
                    // ��AABB��ײ���
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
        // ������Ҫ�����߼�
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
        isOpen = true;  // ��������
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
        // ������仹δ��������������ҵ���ײ
        if (!isOpen) {
            // ����ȫ�ֽ�ɫ�б��������Ƿ���ײ
            for (Character* c : Character::allCharacters) {
                if (c->IsPlayer() && c->hasNormalKey) {
                    // ��AABB��ײ���
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
        // ������Ҫ�����߼�
    }

};
// BulletEmitter �̳��� GameObject������Ҫ������ײ���
class BulletEmitter : public GameObject {
public:
    // Ŀ�꣨�������ǣ��������ӵ�ʱ�ӵ��ᳯ�����Ŀ�����
    Character* target;  // ʹ��������
    // �ӵ������ٶȣ�����/֡��
    float bulletSpeed;
    // �ӵ��˺�
    int bulletDamage;
    // ÿ�η����ӵ���������Χ������ minCount����� maxCount
    int minBulletCount;
    int maxBulletCount;
    // ���������룩
    double fireInterval;
    // �ڲ���ʱ�����ۼ�ÿ֡�� deltaTime
    double fireTimer;
    std::vector<std::shared_ptr<Projectile>> bullets;
    bool isActive = false;
    void SetActive(bool newBool)
    {
        isActive = (newBool);
    }
    // ���캯��
    // ����˵����
    //   x, y���������ĳ�ʼλ��
    //   target��Ŀ���ɫ�������ӵ�ʱ����Ŀ�귽��
    //   bulletSpeed���ӵ������ٶ�
    //   bulletDamage���ӵ��˺�
    //   minBulletCount, maxBulletCount��ÿ�η�����ӵ�������Χ
    //   fireInterval�����������룩
   // ���캯������Ϊ���� std::weak_ptr<Character> ��ֱ�Ӵ��� std::shared_ptr<Character>
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
    // �������Ҫ���������Լ򵥷��� true
    bool InitializeAnimations()
    {
        if (!LoadAnimation(IDLE, { "" }, 999.0f))
            return false;
        return true;
    }

    virtual void Update(float deltaTime) override {
        if (isActive == false) return;
        // �ȼ��Ŀ���Ƿ����
        auto tgt = target;
        if (!tgt) {
            markForDeletion = true;
            return;
        }

        // �ۼӷ����ʱ��
        fireTimer += deltaTime;
        if (fireTimer >= fireInterval) {
            fireTimer = 0.0;
            FireBullets();
        }

        // ���²������ڲ��洢���ӵ�
        // ������� bullets ��������ÿ���ӵ����� Update() �� Draw()
        // ������Ƿ���Ҫɾ���ѱ��ɾ�����ӵ�
        for (auto it = bullets.begin(); it != bullets.end(); ) {
            (*it)->Update(deltaTime);
            (*it)->Draw();
            if ((*it)->IsMarkedForDeletion()) {
                // ����ӵ��Ѿ����ɾ��������������Ƴ�
                it = bullets.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    bool IsEnemy() const { return false; }

    // �����ӵ��������� BulletEmitter ����ײ��������Ϊ����㣬
    // ����Ŀ���λ�ü��㵥λ����������������һ�������� Projectile ����
    void FireBullets() {
        // �����
        float shootX = GetHitboxCenterX();
        float shootY = GetHitboxCenterY();

        auto tgt = target;
        if (!tgt) return;  // ���Ŀ���Ѳ����ڣ���ֱ�ӷ���

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
            // ��ÿ���ӵ�����һ��΢С�����ƫ�ƣ�����λ���ϣ�
            float offsetX = ((rand() % 11) - 5) * 0.5f;  // ��Χ��ԼΪ -2.5 �� 2.5 ����
            float offsetY = ((rand() % 11) - 5) * 0.5f;
            float bulletStartX = shootX + offsetX;
            float bulletStartY = shootY + offsetY;

            float vx_proj = bulletSpeed * unitX;
            float vy_proj = bulletSpeed * unitY;
            // ʹ�� std::make_shared �����ӵ�����
            {
                auto proj = std::make_shared<Projectile>(bulletStartX, bulletStartY, 28, 28, 2, 2, bulletDamage, vx_proj, vy_proj);
                // ��� Projectile ���캯���ڲ��Ѿ����� RegisterCharacter(this)����ע����ܻ�����ظ�ע�����⡣
                // ���� Projectile ���ɷ�������������ע�ᵽȫ���б���ȷ��ȫ���б��в�������ظ��
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
        Cloud* attackCloud; // Ԥ���ص��ƶ���
        std::unique_ptr <BulletEmitter> emitter=nullptr;
        Character* player = nullptr;
        std::vector<Enemy*> enemies;
        BossUI* bossUI = nullptr;
        static BGMPlayer* se;
public:
    int phase;           // �׶�
    float patrolSpeed;      // Ѳ���ٶ�
    float chaseSpeed;       // ׷���ٶ�
    float retreatSpeed;     // �����ٶ�
    float specialAttackCooldown;  // ���⹥������ȴʱ��
    float specialAttackTimer;     // ���⹥����ʱ��
    float stateLockTimer = 0;     // ������ʱ��
    // Ԥ�����������ֵ���ɸ������������
    float chaseThreshold = 300.0f;    // �����˾��룬����׷��
    float retreatThreshold = 150.0f;  // ���ڴ˾��룬�������
    float attackCooldowns[3] = { 0 }; // ATTACKING, ATTACKING2, ATTACKING3����ȴʱ��
    float actionCooldown = 0;       // ͨ���ж���ȴ
    float stateSwitchBuffer = 0.5f; // ״̬�л�����ʱ��
    float lastStateSwitchTime = 0.0f; // �ϴ�״̬�л���ʱ��
    float phaseTimer = 0.0f; // �ϴ�״̬�л���ʱ��
    float thunderTimer = 0.0f; // �ϴ�״̬�л���ʱ��
    float thunderDuration = 1.5f; // �ϴ�״̬�л���ʱ��
    float phase2Duration = 20.0f; // �ϴ�״̬�л���ʱ��
    float phase3Duration = 20.0f; // �ϴ�״̬�л���ʱ��
    float enemySpawnDuration = 1.0f; // �ϴ�״̬�л���ʱ��
    float enemySpawnTimer = 0.0f; // �ϴ�״̬�л���ʱ��

    // Ԥ���������Ȩ��
    const float baseAttackWeight = 1.0f;

    // ���캯����ע����û��� Enemy �Ĺ��캯����

    Boss(float startX, float startY, float w, float h, float range, float offsetX, float offsetY, Type type = ENEMY);

    ~Boss() = default;

    // ���ض�����ʼ������
    bool InitializeAnimations();

    // ���� Update ������ʵ�� Boss ���е���Ϊ
    void Update(float deltaTime) override;

    // �����ܻ������������� Boss �ܻ�ʱ�������Ч��
    void TakeDamage(int damage, float attackerX) override;
    float timer = 0;

    void Thunder(Character* target);
    // ��������
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
    // ��Ӷ���ʱ����ȡ����
    float GetStateDuration(CharacterState state) {
        auto& anim = animations[state];
        return anim.frames.size() * anim.frameTime;
    }
    // �ƶ�����
    void ChasePlayer(float deltaTime, Character* player);
    void RetreatFromPlayer(float deltaTime, Character* player);
    void Patrol(float deltaTime);

    // ������������������ҵľ���
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



// ========== �������뺯�� ==========
bool GetKeyDown(int key);
bool GetKeyUp(int key);
bool GetKeyHold(int key);

// ========== �������뺯������ ==========
extern bool g_keyStates[256];
extern bool g_prevKeyStates[256];

void UpdateKeyStates();
void ClearKeyStates();


#endif // GAME_CHARACTER_H