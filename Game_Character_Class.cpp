#include "Game_Character_Class.h"
#include <functional>
#include <algorithm>
//Game_Character_Class.cpp
// 
// 全局变量定义
bool g_keyStates[256] = { false };
bool g_prevKeyStates[256] = { false };
GameMap* Character::currentMap = nullptr;
// 全局角色列表初始化
std::vector<Character*> Character::allCharacters;
std::vector<UI*> UI::UIs;
std::vector<BGMPlayer*>BGMPlayer::bgms;
BGMPlayer* TriggerObject::se = new BGMPlayer("game-bonus-144751.mp3");
BGMPlayer* Lighting::se = new BGMPlayer("thunder-for-anime-161022.mp3");
BGMPlayer* Projectile::se = new BGMPlayer("medium-explosion-40472.mp3");
BGMPlayer* Boss::se = new BGMPlayer("scream-noise-142446.mp3");
BGMPlayer* TreasureChest::se = new BGMPlayer("chest-opening-87569.mp3");
BGMPlayer* Lock::openSE = new BGMPlayer("unlock-the-door-1-46012.mp3");

// 更新按键状态
void ClearKeyStates() {
    for (int i = 0; i < 256; ++i) {
        g_prevKeyStates[i] = false; // 保存上一帧的按键状态
        g_keyStates[i] = false; // 更新当前帧的按键状态
    }
}
// 更新按键状态
void UpdateKeyStates() {
    for (int i = 0; i < 256; ++i) {
        g_prevKeyStates[i] = g_keyStates[i]; // 保存上一帧的按键状态
        g_keyStates[i] = (GetAsyncKeyState(i) & 0x8000) != 0; // 更新当前帧的按键状态
    }
}

// 检测按键是否按下
bool GetKeyDown(int key) {
    return g_keyStates[key] && !g_prevKeyStates[key];
}

// 检测按键是否释放
bool GetKeyUp(int key) {
    return !g_keyStates[key] && g_prevKeyStates[key];
}

// 检测按键是否持续按下
bool GetKeyHold(int key) {
    return g_keyStates[key];
}

// Character 类的构造函数实现
Character::Character(float startX, float startY, float w, float h, float offsetX, float offsetY, Type type)
    : x(startX), y(startY), vx(0), vy(0), width(w), height(h),
    hitboxOffsetX(offsetX), hitboxOffsetY(offsetY), isOnGround(false),
    state(IDLE), currentFrame(0), frameCounter(0), characterType(type) {
    // 初始化动画
    for (int i = 0; i < 5; i++) {
        animations[i] = Animation{};
    }
    //printf("[Character] Created at %p\n", this); // 打印地址

}


//// 注册角色到全局列表
void Character::RegisterCharacter(Character* character) {
    if (std::find(allCharacters.begin(), allCharacters.end(), character) == allCharacters.end()) {
        allCharacters.push_back(character);
    }
}
// 从全局列表移除角色
void Character::UnregisterCharacter(Character* character) {
    auto it = std::remove(allCharacters.begin(), allCharacters.end(), character);
    if (it != allCharacters.end()) {
        allCharacters.erase(it, allCharacters.end());
    }
}
// ========== 角色攻击判定 ==========
void Character::ActivateAttackCollider(float offsetX, float offsetY, float attackWidth, float attackHeight) {
    // 计算角色碰撞体的左上角位置（碰撞体的位置是角色位置加上碰撞盒偏移）
    float hitboxX = x + hitboxOffsetX;
    float hitboxY = y + hitboxOffsetY;

    // 计算角色碰撞体的中心点
    float centerX = hitboxX + width * 0.5f;
    float centerY = hitboxY + height * 0.5f;

    // 根据角色面向调整水平偏移：如果面向右侧则正偏移，否则取负
    float effectiveOffsetX = (facingRight ? offsetX : -offsetX);

    // 计算攻击碰撞体的中心点（相对于角色碰撞体中心加上偏移）
    float colliderCenterX = centerX + effectiveOffsetX;
    float colliderCenterY = centerY + offsetY;

    // 根据攻击碰撞体中心点和指定的尺寸，计算其左上角坐标
    attackCollider.x = colliderCenterX - attackWidth * 0.5f;
    attackCollider.y = colliderCenterY - attackHeight * 0.5f;
    attackCollider.width = attackWidth;
    attackCollider.height = attackHeight;

    attackCollider.active = true;
}

void Character::DeactivateAttackCollider() {
    attackCollider.active = false;
}

void Character::CheckAttackHit() {
    if (!isAttackActive || hasTriggeredThisFrame) return;
    for (int i = 0; i < Character::allCharacters.size();i++) {
        if (allCharacters[i] == this || !allCharacters[i]->canTakeDamage || allCharacters[i]->IsLighting()) continue;
        if (hitTargets.find(allCharacters[i]) != hitTargets.end()) continue;

        // 碰撞检测逻辑
        const float otherX = allCharacters[i]->GetHitboxX();
        const float otherY = allCharacters[i]->GetHitboxY();
        const float otherW = allCharacters[i]->GetWidth();
        const float otherH = allCharacters[i]->GetHeight();

        if (attackCollider.x < otherX + otherW &&
            attackCollider.x + attackCollider.width > otherX &&
            attackCollider.y < otherY + otherH &&
            attackCollider.y + attackCollider.height > otherY)
        {

            // 将中心点作为参数传入，用于计算击退方向
            if (state == ATTACKING3)
            {
                allCharacters[i]->state = FREEZE;
            }
            allCharacters[i]->TakeDamage(20, this->GetHitboxCenterX());  // this指向攻击者
            hitTargets.insert(allCharacters[i]);
        }

    }

    hasTriggeredThisFrame = true; // 标记本帧已触发
}


void Character::TakeDamage(int damage, float attackerX) {
    if (!canTakeDamage) {
        //printf("无法受击: 无敌状态\n");
        return;
    }

    if (state != FREEZE)
    {
        // 先执行击退
        ApplyKnockback(attackerX);

        // 设置受击状态（仅在扣血时触发）
        state = HURT;
        hurtTimer = hurtDuration;
    }
    else
    {
        // 先执行击退
        //ApplyKnockback(attackerX);
        freezeTimer = freezeDuration;
    }


    // 原有伤害逻辑
    health -= damage;
    ///printf("受到%d点伤害! 剩余生命值: %d\n", damage, health);

    if (health <= 0) {
        Die();
    }
}

void Character::Die() {
    //printf("Character at (%f, %f) died!\n", x, y);
    if (!IsEnemy())
    {
        return;
    }
    if (IsBoss())
        return;
    int type = rand();
    TriggerObject* drop = nullptr;
    switch (type % 2)
    {
    case 0:
        drop = new RedPotion(x, y);
        break;
    case 1:
        drop = new BluePotion(x, y);
        break;
    default:
        break;
    }

    markForDeletion = true; // 标记为待删除
}

////////////// Character 类的 LoadAnimation 实现/////////////////
bool Character::LoadAnimation(CharacterState action, const std::vector<std::string>& framePaths, float frameTime) {
    std::lock_guard<std::mutex> lock(animationMutex); // 加锁

    animations[action].frames.clear(); // 清空之前的帧
    animations[action].frameTime = frameTime;

    for (const auto& path : framePaths) {
        std::string fullPath = "pic/" + path;
        auto bmp = std::shared_ptr<Bmp>(LoadBmp(fullPath.c_str()), [](Bmp* bmp) { if (bmp) DeleteBmp(&bmp); });
        if (!bmp) {
            //printf("Error: Failed to load BMP file: %s\n", fullPath.c_str());
            animations[action].frames.clear(); // 加载失败时清空帧
            return false;
        }
        animations[action].frames.push_back(bmp);
    }
    return true;
}

////////////// Character 类的 Update 实现/////////////
void Character::Update(float deltaTime)
{
    
    // 1) 受击计时器
    if (state == HURT) {
        hurtTimer -= deltaTime;
        if (hurtTimer <= 0) {
            state = IDLE;
            vx = 0;
            vy = 0;
        }
        invincibilityTimer = invincibilityDuration;
        canTakeDamage = false;
    }
    if (state == FREEZE) {
        // 每帧都保持 vx, vy = 0
        vx = 0;
        vy = 0;

        // 递减计时
        freezeTimer -= deltaTime;

        // 如果时间到了，就退回到 IDLE
        if (freezeTimer <= 0) {
            freezeTimer = 0;   // 保险起见，让它变成0
            state = IDLE;
        }
    }

    if (invincibilityTimer > 0)
    {
        invincibilityTimer -= deltaTime;
    }
    else {
        canTakeDamage = true;
    }
    // 2) 保存上一帧地面状态
    wasOnGround = isOnGround;
    // 开始本帧先认为不在地面
    isOnGround = false;

    // 3) 重力（排除Dash/Knockback）
    if (!isDashing && !isKnockback) {
        // 原本你用 *60, 看你项目帧率需求，这里保留
        vy += gravity * 60 * deltaTime;
    }

    // 4) 计算本帧移动量
    float moveX = vx * 60 * deltaTime;
    float moveY = vy * 60 * deltaTime;

    // ============= 水平方向碰撞 (分轴/小步) =============
    {
        float newX = x;
        float step = (moveX > 0) ? 1.0f : -1.0f;
        int steps = static_cast<int>(fabs(moveX));  // 逐像素或每步1

        for (int i = 0; i < steps; ++i) {
            float testX = newX + step;
            // 检查碰撞
            if (currentMap && currentMap->CheckCollision(
                testX + hitboxOffsetX,
                y + hitboxOffsetY,
                width,
                height))
            {
                // 撞到墙 => vx=0
                vx = 0;
                break;
            }
            else {
                newX = testX; // 可以移动
            }
        }
        // 如果 moveX 不足1像素(小于1)，再一次性处理
        float remainX = moveX - (steps * step);
        if (fabs(remainX) > 0) {
            float testX = newX + remainX;
            if (currentMap && currentMap->CheckCollision(
                testX + hitboxOffsetX,
                y + hitboxOffsetY,
                width,
                height))
            {
                vx = 0;
            }
            else {
                newX = testX;
            }
        }

        x = newX;
    }

    // ============= 垂直方向碰撞 (分轴/小步) =============
    {
        float newY = y;
        float step = (moveY > 0) ? 1.0f : -1.0f;
        int steps = static_cast<int>(fabs(moveY));

        for (int i = 0; i < steps; ++i) {
            float testY = newY + step;
            if (currentMap && currentMap->CheckCollision(
                x + hitboxOffsetX,
                testY + hitboxOffsetY,
                width,
                height))
            {
                // 撞到地面/平台(若step>0表示下落)
                if (step > 0) {
                    // 落地
                    isOnGround = true;
                    vy = 0;
                }
                else {
                    // 撞到顶部
                    vy = 0;
                }
                break;
            }
            else {
                newY = testY;
            }
        }

        // 处理余量
        float remainY = moveY - (steps * step);
        if (fabs(remainY) > 0) {
            float testY = newY + remainY;
            if (currentMap && currentMap->CheckCollision(
                x + hitboxOffsetX,
                testY + hitboxOffsetY,
                width,
                height))
            {
                if (remainY > 0) {
                    // 落地
                    isOnGround = true;
                    vy = 0;
                }
                else {
                    // 头顶
                    vy = 0;
                }
            }
            else {
                newY = testY;
            }
        }

        y = newY;
    }

    // 5) 如果本帧刚落地 => 重置空中系统
    if (!wasOnGround && isOnGround) {
        ResetAirSystem();
    }

    // 6) 冲刺计时 / 伤害 / 击退
    UpdateDash(deltaTime);
    if (isKnockback) {
        knockbackTimer -= deltaTime;
        if (knockbackTimer <= 0) {
            isKnockback = false;
            vx *= 0.5f;
        }
    }
    // 如果处于击退冷却中，递减 canKnockbackTimer
    if (!canKnockback) {
        canKnockbackTimer -= deltaTime;
        if (canKnockbackTimer <= 0) {
            canKnockback = true;
        }
    }

    // 7) 动画更新
    UpdateAnimation(deltaTime);

    // 8) 攻击碰撞体
    if (attackCollider.active) {
        CheckAttackHit();
    }
    if (state != ATTACKING && attackCollider.active) {
        DeactivateAttackCollider();
        hitTargets.clear();
    }

    // 9) 检测角色之间碰撞(伤害)
    if (state != HURT) {
        auto collided = CheckCollisionWithOthers();
        for (Character* other : collided) {
            // 如果碰撞对象是一方为 Boss 而另一方为 Lighting，则跳过碰撞处理
            if ((this->IsBoss() && dynamic_cast<Lighting*>(other) != nullptr) ||
                (dynamic_cast<Lighting*>(this) != nullptr && other->IsBoss()))
            {
                continue;
            }
            // 如果双方都是敌人，则相互击退
            if (this->IsEnemy() && other->IsEnemy()) {
                // 计算双方碰撞盒的中心点 X 坐标
                float otherCenterX = other->GetHitboxCenterX();
                float thisCenterX = this->GetHitboxCenterX();
                if (!this->IsBoss() && !other->IsBoss())
                {
                    this->TakeDamage(10, otherCenterX);
                    other->TakeDamage(10,thisCenterX);
                }
                else
                {
                    this->ApplyKnockback(otherCenterX);
                    other->ApplyKnockback(thisCenterX);
                }

                continue;
            }
            // 如果对方是敌人，而当前对象不是敌人，则当前对象受到伤害
            if (auto enemy = dynamic_cast<Enemy*>(other)) {
                if (!this->IsEnemy()) {
                    float enemyCenterX = enemy->GetHitboxCenterX();
                    this->TakeDamage(10, enemyCenterX);
                    break;
                }
            }
        }
    }

    // 更新动画帧标记
    if (currentFrame != previousAnimationFrame) {
        hasTriggeredThisFrame = false;
        previousAnimationFrame = currentFrame;
    }
}

////////////////// Character 类的 Draw 实现//////////////////
void Character::Draw() {
    std::lock_guard<std::mutex> lock(animationMutex); // 加锁
    if (!animations[state].frames.empty() && currentFrame < animations[state].frames.size()) {
        DrawBmp(x, y, animations[state].frames[currentFrame].get(), ((facingRight) ? 0 : BMP_HINV), true);//三元运算决定是否翻转
    }

}

//////////// Character 类的 UpdateAnimation 实现////////////////////////
void Character::UpdateAnimation(float deltaTime) {
    std::lock_guard<std::mutex> lock(animationMutex); // 加锁
    if (animations[state].frames.empty()) {
        return;
    }

    frameCounter += deltaTime;
    if (frameCounter >= animations[state].frameTime) {
        frameCounter = 0;
        currentFrame = (currentFrame + 1) % animations[state].frames.size();
    }
}
//////////////////////////////////////////updateDash
void Character::UpdateDash(float deltaTime) {
    if (isDashing) {
        dashTimer -= deltaTime;

        // 平滑结束（最后10%时间减速）
        if (dashTimer < dashDuration * 0.1f) {
            vx *= 0.9f;
        }

        if (dashTimer <= 0) {
            isDashing = false;
            vx = 0;
            canTakeDamage = true;
            //printf("冲刺结束\n");
        }
    }
}


// Player 类的构造函数实现
Player::Player(float startX, float startY, float w, float h, float offsetX, float offsetY,Type type)
    : Character(startX, startY, w, h, offsetX, offsetY, type) {
    RegisterCharacter(this); // 注册到全局列表
   // printf("[Player] Created at %p\n", this);
    if (!this->IsPlayer())
        return;
    runningSE = new BGMPlayer("footsteps_running-40448.mp3");
    attackingSE = new BGMPlayer("golf-swing-practice-01-255625.mp3");
    jumpSE = new BGMPlayer("cartoon-jump-6462.mp3");
    magicSE = new BGMPlayer("elemental-magic-spell-impact-outgoing-228342.mp3");
    dead = LoadBmp("pic/DEAD.bmp");
    standUp = LoadBmp("pic/STANDUP.bmp");
}

// Player 类的 InitializeAnimations 实现
bool Player::InitializeAnimations() {
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
    "player_hurt_shine_2.bmp"}, 0.1f)) return false;
    if (!LoadAnimation(FREEZE, { "player_freeze_1.bmp", "player_freeze_2.bmp", "player_freeze_shine_1.bmp","player_freeze_shine_2.bmp" }, 0.1f)) return false;
    if (!LoadAnimation(DEAD, { "player_dead_1.bmp", "player_dead_2.bmp", "player_dead_3.bmp",
        "player_dead_4.bmp","player_dead_5.bmp" }, 0.3f)) return false;



    return true;    //全部加载后返回为真，新加动画要加到上方


}
// ========== 玩家类攻击扩展 ==========

// Player 类的 Update 实现
// Player.cpp
void Player::Update(float deltaTime) {
    cureTimer += deltaTime;
    if (cureTimer >= cureDuration&&state!=DEAD)
    {
        
        TakeHealth(1);
        TakeMP(1);
        cureTimer = 0;
    }
    if (state == DEAD)
    {
        deadTimer += deltaTime;
        if (deadTimer >= deadDuration)
        {
            drawWhat = 1;
        }
        else drawWhat=0;
        if (currentFrame >= 4)
        {
            hasDead = true;
        }
        if (!hasDead)
        {
            UpdateAnimation(deltaTime);
        }
        if (hasDead)
        {
            if (GetKeyDown(PK_ENTER))
            {
                health = 100;
                mp = 100;
                state = IDLE;
                hasDead = false;
                collisionEnabled = true;
                deadTimer = 0;
                return;
            }
        }

        return;
    }
    if (state != FREEZE)
    {
        HandleInput();

    }
    else {
        vx = 0;
        vy = 0;
    };

    Character::Update(deltaTime);

    // 状态变化时强制关闭攻击碰撞体
    if (state != ATTACKING &&state !=ATTACKING2&& attackCollider.active) {
        DeactivateAttackCollider();
        hitTargets.clear(); // 清空命中记录
        //printf("状态切换：强制关闭攻击碰撞体\n");
    }
    if (state == ATTACKING) {
        const bool isAttackFrame3 = (currentFrame == 3);
        const bool isAttackFrame6 = (currentFrame == 6);
        const bool isAttackFrame8 = (currentFrame == 8);

        if (currentFrame >= animations[ATTACKING].frames.size() - 1) {
            DeactivateAttackCollider();
            OnAttackComboEnd();
        }
        else if (isAttackFrame3 || isAttackFrame6 || isAttackFrame8) {
            // 仅在首次进入该攻击帧时清空记录
            if (!hasTriggeredThisFrame) {
                OnAttackFrameStart(); // 清空 hitTargets
                ActivateAttackCollider(55.0f, 5.0f, 60.0f, 65.0f);
            }
            CheckAttackHit(); // 受 hasTriggeredThisFrame 控制
        }
        else {
            DeactivateAttackCollider();
        }
    }
    else if (state == ATTACKING2)
    {
        const bool isAttackFrame5 = (currentFrame == 5);

        if (currentFrame >= animations[ATTACKING2].frames.size() - 1) {
            state = IDLE;
            // 攻击动作结束后重置 hasTriggeredThisFrame
            hasTriggeredThisFrame = false;
        }
        else if (isAttackFrame5 && !hasTriggeredThisFrame) {
            // 同一帧只会执行一次
            Shoot();
            hasTriggeredThisFrame = true;  // 标记已触发
        }
    }
    if (state != HURT) {
        auto collided = CheckCollisionWithOthers();
        for (Character* other : collided) {
            if (auto enemy = dynamic_cast<Enemy*>(other)) {
                // 仅玩家受伤，敌人不受普通碰撞伤害
                this->TakeDamage(10, enemy->GetHitboxCenterX());
                break;
            }
        }
    }

    if (health <= 0)
    {
        state = DEAD;
        collisionEnabled = false;
    }
    if (state == RUNNING && isOnGround)
        runningSE->Play(true);
    else runningSE->Stop();
}

//dash+
void Player::StartDash(float inputDir) {
    if (!canUseDash)
        return;
    canTakeDamage = false;
    state = RUNNING;
    isDashing = true;
    dashTimer = dashDuration;
    invincibilityTimer = invincibilityDuration;
    // 方向处理
    if (inputDir == 0) {
        vx = facingRight ? dashSpeed : -dashSpeed;
        vy = 0;
    }
    else {
        vx = dashSpeed * inputDir;
        facingRight = (inputDir > 0);
        vy = 0;
    }

    // 资源消耗
    if (isOnGround) {
        canGroundDash = false;
    }
    else {
        remainingAirDashes--;
    }
}

void Player::Shoot()
{
    if (!canShoot)
        return;
    if (mp-10 < 0)
    {
        return;
    }
    // 1) 计算子弹起始坐标(根据玩家位置+朝向微调)
    float bulletX = GetHitboxCenterX() + (facingRight ? 25.0f : -60.0f);
    float bulletY = GetHitboxCenterY()-12.0f;

    // 2) 计算子弹速度(根据朝向)
    float speed = 6.0f; // 可调
    float vxBullet = facingRight ? speed : -speed;
    float vyBullet = 0;

    // 3) 创建子弹(假设宽高是10x10, offset都设0)
    PlayerProjectile* bullet = new PlayerProjectile(
        bulletX, bulletY,
        30.0f, 30.0f,
        0, 0,
        15,           // 子弹伤害
        vxBullet, vyBullet
    );

    bullet->InitializeAnimations();

    // 4) 加入全局角色列表(或你自己的对象管理容器)
    Character::RegisterCharacter(bullet);
    magicSE->Play(false);
    Cost(10);
}
// Player 类的 HandleInput 实现
void Player::HandleInput() {
    CharacterState previousState = state; // 记录之前的状态
    bool isAttacking = false;
    if (state == ATTACKING || state == ATTACKING2)
         isAttacking = true;
    // === 状态优先级控制 ===
    if (state == HURT || isDashing) return;

    // === 移动输入 ===
    float inputDir = 0.0f;
    if (!isAttacking)
    {
        if (GetKeyHold(PK_LEFT) || GetKeyHold(PK_A)) inputDir -= 1.0f;
        if (GetKeyHold(PK_RIGHT) || GetKeyHold(PK_D)) inputDir += 1.0f;
    }


    // === 速度控制 ===
    if (!isDashing) {
        vx = inputDir * moveSpeed;
        if (inputDir != 0) facingRight = (inputDir > 0);
    }

    // === 状态切换 ===
    if (isOnGround) {
        state = (inputDir != 0) ? RUNNING : IDLE;
    }
    else {
        state = (vy < 0) ? JUMPING :JUMPING;
    }

    // === 跳跃系统 ===
    if (GetKeyDown(PK_SP) && !isAttacking) {
        if (isOnGround) {
            vy = jumpForce;
            isOnGround = false;
            jumpSE->SetVolume(50);
            jumpSE->Play(false);
        }
    }

    // === 攻击系统 ===
    if (GetKeyHold(PK_Z)) {
        state = ATTACKING;
        attackingSE->Play(true);
    }
    else { attackingSE->Stop(); }
    // === 攻击系统 ===
    if (GetKeyHold(PK_X)&&canShoot) {
        state = ATTACKING2;

    }
    //cure
    if (GetKeyDown(PK_C))
    {
        Cure();
    }

    // === 冲刺系统 ===
    if (GetKeyDown(PK_SHIFT)&&!isAttacking) {
        if (isOnGround ? canGroundDash : remainingAirDashes > 0) {
            if (inputDir > 0)
            {
                inputDir = 1;
            }
            else if (inputDir < 0)
            {
                inputDir = -1;
            }
            StartDash(inputDir); // 传入当前输入方向
        }
    }

    // === 动画同步 ===
    if (state != previousState) {
        currentFrame = 0;
        frameCounter = 0;
    }
}

// Enemy 类的构造函数实现
Enemy::Enemy(float startX, float startY, float w, float h, float range, float offsetX, float offsetY, Type type)
    : Character(startX, startY, w, h, offsetX, offsetY, type ), patrolRange(range), startX(startX),movingRight(true) {
    RegisterCharacter(this); // 注册到全局列表

}
// ========== 敌人类攻击扩展 ==========



void Enemy::TakeDamage(int damage, float attackerX) {
    if (!canTakeDamage) return;
    Character::TakeDamage(damage, attackerX);
    // 先执行击退
    ApplyKnockback(attackerX);  // 调用继承来的击退方法

    // 敌人特定逻辑：需要扣血时才执行以下
    health -= damage;
    if (health > 0) {
        //printf("Enemy took damage! Remaining health: %d\n", health);
        state = HURT;  // 仅当存活时才保持受击状态
        hurtTimer = hurtDuration;
    }
    else {
        Die();
    }
}
// Enemy 类的 InitializeAnimations 实现
bool Enemy::InitializeAnimations() {
    if (!LoadAnimation(IDLE, { "enemy_idle_1.bmp" }, 0.3f)) return false;
    if (!LoadAnimation(RUNNING, { "enemy_run_1.bmp" }, 0.2f)) return false;
    if (!LoadAnimation(HURT, { "enemy_red.bmp" }, 0.3f)) return false;
    return true;
}

// Enemy 类的 Update 实现
void Enemy::Update(float deltaTime) {
     Patrol(); 
    Character::Update(deltaTime);
}

// Enemy 类的 Patrol 实现
void Enemy::Patrol() {
    if (this->IsBoss()||state==HURT)
        return;
    if (x > 1300 || x < -10)
    {
        markForDeletion = true;
    }
    if (state == HURT || isKnockback||state==ATTACKING) return; // isKnockback判断
    if (movingRight) {
        vx = 1.0f;
        if (x > startX + patrolRange) {
            movingRight = false;
        }
    }
    else {
        vx = -1.0f;
        if (x < startX) {
            movingRight = true;
        }
    }
    state = RUNNING;
    facingRight = movingRight;
}
bool Fountain::InitializeAnimations()
{
    if (!LoadAnimation(IDLE, { "fountain_idle_1.bmp","fountain_idle_2.bmp","fountain_idle_3.bmp","fountain_idle_4.bmp", }, 0.3f)) return false;
    return true;
}
bool Leaf::InitializeAnimations()
{
    if (!LoadAnimation(IDLE, { "leaf_1.bmp","leaf_2.bmp","leaf_3.bmp","leaf_4.bmp","leaf_5.bmp","leaf_6.bmp" }, 0.2f)) return false;
    return true;
}

void Leaf::Update(float deltaTime)
{
    if (x >= 1280 || x <= 0)
    {
        x=(rand() % 2000) - 720-200;
    }
    if (y >= 720)
    {
        y = rand() % 1500-750;
    }
    x = x + deltaTime * 40;
    y = y + deltaTime * 60;
    UpdateAnimation(deltaTime);
}
bool Projectile::InitializeAnimations()
{
    if (!LoadAnimation(IDLE, { "bullet.bmp" }, 0.3f)) return false;
    if (!LoadAnimation(DEAD, { "Explosion_gas1.bmp","Explosion_gas2.bmp","Explosion_gas3.bmp","Explosion_gas4.bmp","Explosion_gas5.bmp",
        "Explosion_gas6.bmp","Explosion_gas7.bmp","Explosion_gas8.bmp","Explosion_gas9.bmp","Explosion_gas10.bmp" }, 0.1f)) return false;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
    return true;
}
void Projectile::Update(float deltaTime)
{
    if (state == DEAD)
    {
        UpdateAnimation(deltaTime);

        vx = 0;
        vy = 0;
        if (currentFrame >= animations[DEAD].frames.size() - 1)
        {
            markForDeletion = true;
        }

    }
    // 根据当前速度更新位置
    // 注意：这里采用固定帧率转换（乘以60），你也可以根据你的项目做相应调整
    x += vx * 60 * deltaTime;
    y += vy * 60 * deltaTime;

    // 检查与地图的碰撞（地形）
    // 使用角色的碰撞盒位置：x + hitboxOffsetX, y + hitboxOffsetY, 宽高为 width, height
    if (Character::GetCurrentMap() &&
        Character::GetCurrentMap()->CheckCollision(x + hitboxOffsetX, y + hitboxOffsetY, width, height))
    {
        collisionEnabled = false;
        state = DEAD;
        x -= 128;
        y -= 128;
        se->Play(false);
        return;
    }

    // 检查与其他角色的碰撞（例如主角）
    // 这里利用已有的 CheckCollisionWithOthers 方法（返回与本角色碰撞的角色列表）
    std::vector<Character*> collided = CheckCollisionWithOthers();
    for (Character* other : collided)
    {
        // 如果其他角色是玩家，则对其造成伤害
        if (other->IsPlayer()) {
            other->TakeDamage(damage, GetHitboxCenterX());  // 此处传入 x 作为攻击者坐标，实际可以根据需要调整
            collisionEnabled = false;        
            state = DEAD;
            x -= 128;
            y -= 128;
            se->Play(false);
            break;
        }
    }

    // 可选：如果飞行道具飞出屏幕范围，也标记删除
    // 例如屏幕宽1280，高720（根据项目实际情况调整）
    if (x < -width || x > 1280 || y < -height || y > 720) {
        markForDeletion = true;
    }
}


bool PlayerProjectile::InitializeAnimations()
{
    // 如果你想改贴图路径，例如 "player_bullet.bmp"
    if (!LoadAnimation(IDLE, { "player_bullet.bmp" }, 0.3f))
        return false;
    if (!LoadAnimation(DEAD, { "Explosion_blue_circle1.bmp","Explosion_blue_circle2.bmp","Explosion_blue_circle3.bmp","Explosion_blue_circle4.bmp" ,"Explosion_blue_circle5.bmp",
        "Explosion_blue_circle6.bmp","Explosion_blue_circle7.bmp","Explosion_blue_circle8.bmp","Explosion_blue_circle9.bmp","Explosion_blue_circle10.bmp" }, 0.1f))//
        return false;
    return true;
}

void PlayerProjectile::Update(float deltaTime)
{

    if (state == DEAD)
    {
        
        UpdateAnimation(deltaTime);
        vx = 0;
        vy = 0;
        if (currentFrame >= animations[DEAD].frames.size() - 1)
        {
            markForDeletion = true;
        }

    }
    // 1) 根据速度更新坐标
    x += vx * 60.0f * deltaTime;
    y += vy * 60.0f * deltaTime;

    // 2) 地图碰撞检测
    if (Character::GetCurrentMap() &&
        Character::GetCurrentMap()->CheckCollision(x + hitboxOffsetX, y + hitboxOffsetY, width, height)&&state!=DEAD)
    {
        collisionEnabled = false;
        state = DEAD;

        x -= 128;
        y -= 128;
        se->Play(false);
        return;
    }

    // 3) 检查与其他角色碰撞
    //    这里只对敌人造成伤害
    std::vector<Character*> collided = CheckCollisionWithOthers();
    for (Character* other : collided)
    {
        if (other->IsEnemy()&& state != DEAD) {
            other->TakeDamage(damage, GetHitboxX()); // 伤害 + 击退方向
            collisionEnabled = false;
            state = DEAD;
            x -= 128;
            y -= 128;
            se->Play(false);
            break;
        }
        else if (other->IsFallingChest() &&state != DEAD)
        {
            other->TakeDamage(0,GetHitboxCenterX()); // 伤害 + 击退方向
            collisionEnabled = false;
            state = DEAD;
            x -= 128;
            y -= 128;
            se->Play(false);
            break;
        }
    }
    
    // 4) 若飞出屏幕也删除
    if (x < -width || x > 1280 || y < -height || y > 720) {
        markForDeletion = true;
    }

}
void UI::RegisterUI(UI* ui) {
        UIs.push_back(ui);
}

void MPUI::RegisterUI(MPUI* ui) {
    UI:: UIs.push_back(ui);
}

void BossUI::RegisterUI(BossUI* ui) {
    UI::UIs.push_back(ui);
}




// Boss 类构造函数：调用基类 Enemy 的构造函数，并初始化 Boss 独有的属性
Boss::Boss(float startX, float startY, float w, float h, float range, float offsetX, float offsetY, Type type)
    : Enemy(startX, startY, w, h, range, offsetX, offsetY, type),
    phase(1),
    chaseThreshold(300.0f),
    retreatThreshold(150.0f),
    patrolSpeed(1.0f),
    chaseSpeed(4.0f),
    retreatSpeed(6.0f)
{
    // 增加 Boss 的生命值，使其比普通敌人更坚韧
    health = 500;
    state = IDLE;
    facingRight = !facingRight;
    attackCloud = new Cloud(300, 300);
    attackCloud->InitializeAnimations();
    if (player == nullptr)
    {
        for (Character* c : Character::allCharacters) {
            if (c->IsPlayer()) {
                player = c;
                break;
            }
        }
    }
    emitter = std::make_unique<BulletEmitter>(640,50,player,14.0f,10,1,3,0.15);
    emitter->SetActive(false);
    //markForDeletion = true;
    // 注意：Enemy 构造函数中可能已经调用了 RegisterCharacter，
    // 若没有，则可以在此调用：
    // RegisterCharacter(this);
    bossUI = new BossUI(150, 660);
};

// 初始化 Boss 动画（这里示例加载了空闲、攻击和受击的动画）
bool Boss::InitializeAnimations()
{
    // 加载 Boss 空闲动画
    if (!LoadAnimation(IDLE, { "Boss_Idle_1.bmp", "Boss_Idle_2.bmp", "Boss_Idle_3.bmp", "Boss_Idle_4.bmp", "Boss_Idle_5.bmp", "Boss_Idle_6.bmp", "Boss_Idle_7.bmp" }, 0.2f))
        return false;
    if (!LoadAnimation(RUNNING, { "Boss_walk_1.bmp", "Boss_walk_2.bmp", "Boss_walk_3.bmp", "Boss_walk_4.bmp", 
        "Boss_walk_5.bmp", "Boss_walk_6.bmp", "Boss_walk_7.bmp","Boss_walk_8.bmp", 
        "Boss_walk_9.bmp", "Boss_walk_10.bmp", "Boss_walk_11.bmp", "Boss_walk_12.bmp", 
        "Boss_walk_13.bmp"}, 0.2f))
        return false; 
    if (!LoadAnimation(CHASE, { "Boss_walk_1.bmp", "Boss_walk_2.bmp", "Boss_walk_3.bmp", "Boss_walk_4.bmp", 
        "Boss_walk_5.bmp", "Boss_walk_6.bmp", "Boss_walk_7.bmp","Boss_walk_8.bmp", 
        "Boss_walk_9.bmp", "Boss_walk_10.bmp", "Boss_walk_11.bmp", "Boss_walk_12.bmp", 
        "Boss_walk_13.bmp"}, 0.2f))
        return false; 
    if (!LoadAnimation(RETREAT, { "Boss_walk_1.bmp", "Boss_walk_2.bmp", "Boss_walk_3.bmp", "Boss_walk_4.bmp", 
        "Boss_walk_5.bmp", "Boss_walk_6.bmp", "Boss_walk_7.bmp","Boss_walk_8.bmp", 
        "Boss_walk_9.bmp", "Boss_walk_10.bmp", "Boss_walk_11.bmp", "Boss_walk_12.bmp", 
        "Boss_walk_13.bmp"}, 0.2f))
        return false;

    // 加载 Boss 攻击动画（可根据实际情况设置帧数和播放时间）
    if (!LoadAnimation(ATTACKING, { "Boss_Attack_1.bmp", "Boss_Attack_2.bmp", "Boss_Attack_3.bmp", "Boss_Attack_4.bmp",
         "Boss_Attack_5.bmp", "Boss_Attack_6.bmp", "Boss_Attack_7.bmp", "Boss_Attack_8.bmp", 
         "Boss_Attack_9.bmp", "Boss_Attack_10.bmp", "Boss_Attack_11.bmp", "Boss_Attack_12.bmp", 
         "Boss_Attack_13.bmp", "Boss_Attack_14.bmp", "Boss_Attack_15.bmp", "Boss_Attack_16.bmp", }, 0.08f))
        return false;
    // 加载 Boss 攻击动画（可根据实际情况设置帧数和播放时间）
    if (!LoadAnimation(ATTACKING2, { "Boss_Attack2_1.bmp", "Boss_Attack2_2.bmp", "Boss_Attack2_3.bmp", "Boss_Attack2_4.bmp",
         "Boss_Attack2_5.bmp", "Boss_Attack2_6.bmp", "Boss_Attack2_7.bmp" }, 0.08f))
        return false;
    // 加载 Boss 攻击动画（可根据实际情况设置帧数和播放时间）
    if (!LoadAnimation(ATTACKING3, { "Boss_Attack3_1.bmp", "Boss_Attack3_2.bmp", "Boss_Attack3_3.bmp", "Boss_Attack3_4.bmp",
         "Boss_Attack3_5.bmp", "Boss_Attack3_6.bmp", "Boss_Attack3_7.bmp","Boss_Attack3_8.bmp", "Boss_Attack3_9.bmp", "Boss_Attack3_10.bmp" }, 0.15f))
        return false;

    // 加载 Boss 受击动画
    if (!LoadAnimation(HURT, { "Boss_Hurt_1.bmp", "Boss_Hurt_2.bmp", "Boss_Hurt_3.bmp" }, 0.2f))
        return false;
    // 加载 Boss dead动画
    if (!LoadAnimation(DEAD, { "Boss_Dead_1.bmp","Boss_Dead_1.bmp","Boss_Dead_1.bmp","Boss_Dead_1.bmp", "Boss_Dead_2.bmp", "Boss_Dead_3.bmp" }, 0.4f))
        return false;

    return true;
}

void Boss:: Thunder(Character* target) {

    auto tgt = target;
    if (!tgt) return;  // 如果目标已不存在，则直接返回

    float targetX = tgt->GetHitboxCenterX()-40-77;
    float targetY = tgt->GetHitboxCenterY();

    Lighting* thunder = new Lighting(targetX, -70);

}

// 重载 Update 方法，添加 Boss 特有的行为逻辑
void Boss::Update(float deltaTime)
{
    if (health <= 0)
    {
        state = DEAD;
        SetCollisionEnabled(false);
        if (state == DEAD)
        {
            if (currentFrame >= animations[DEAD].frames.size() - 1)
            {
                hasDead = true;
                auto* trophy = new Trophy(GetHitboxCenterX(), GetHitboxCenterY() - 120);
            }
            if (hasDead)
                currentFrame = animations[DEAD].frames.size() - 1;
        }
        if(!hasDead)
            UpdateAnimation(deltaTime);
        return;
    }
    //bossUI->Update(deltaTime);
    // 更新所有冷却时间
    for (int i = 0; i < 3; i++) {
        if (attackCooldowns[i] > 0)
            attackCooldowns[i] -= deltaTime;
    }
    if (actionCooldown > 0)
        actionCooldown -= deltaTime;
    if (player == nullptr)
    {
        for (Character* c : Character::allCharacters) {
            if (c->IsPlayer()) {
                player = c;
                break;
            }
        }
    }
    if (player == nullptr) {
        // 没有玩家，不进行攻击行为
        //Patrol(deltaTime);
        Enemy::Update(deltaTime);
        return;
    }


    // 状态变化时强制关闭攻击碰撞体
    if (state != ATTACKING && state != ATTACKING2 && state != ATTACKING3 && attackCollider.active) {
        DeactivateAttackCollider();
        hitTargets.clear(); // 清空命中记录
        //printf("状态切换：强制关闭攻击碰撞体\n");
    }

    if (state == ATTACKING) {
        vx = 0;
        vy = 0;
        if (currentFrame >= animations[ATTACKING].frames.size() - 1) {
            DeactivateAttackCollider();
            OnAttackComboEnd();
            state = IDLE;
        }
        else {
            if (currentFrame == 6 || currentFrame == 7 || currentFrame == 8 || currentFrame == 9 || currentFrame == 10 )
            {
                // 仅在首次进入该攻击帧时清空记录
                if (!hasTriggeredThisFrame) {
                    OnAttackFrameStart(); // 清空 hitTargets
                    ActivateAttackCollider(0.0f, 0.0f, 180.0f, 150.0f);
                }
                CheckAttackHit(); // 受 hasTriggeredThisFrame 控制
                SetCollisionEnabled(false);
            }
            else { DeactivateAttackCollider();
            SetCollisionEnabled(true);
            }

        }
    }

    else if (state == ATTACKING2) {
        vx = 0;
        vy = 0;
        if (currentFrame >= animations[ATTACKING2].frames.size() - 1) {
            DeactivateAttackCollider();
            OnAttackComboEnd();
            state = IDLE;
        }
        else {
            if (currentFrame == 4||currentFrame==5)
            {
                // 仅在首次进入该攻击帧时清空记录
                if (!hasTriggeredThisFrame) {
                    OnAttackFrameStart(); // 清空 hitTargets
                    //attackCollider.active = true;
                    ActivateAttackCollider(95.0f, 30.0f, 65.0f, 90.0f);
                }
                CheckAttackHit(); // 受 hasTriggeredThisFrame 控制
            }
            else {
                DeactivateAttackCollider();
            }

        }
    }
    else if (state == ATTACKING3) 
    {
        vx = 0;
        vy = 0;
        if (currentFrame >= animations[ATTACKING3].frames.size() - 1) {
            DeactivateAttackCollider();
            OnAttackComboEnd();
            state = IDLE;
        }
        else {
            if (currentFrame == 7||currentFrame==8||currentFrame==9)
            {
                if (currentFrame == 7)
                {

                    // 仅在首次进入该攻击帧时清空记录
                    if (!hasTriggeredThisFrame) {
                        OnAttackFrameStart(); // 清空 hitTargets
                        ActivateAttackCollider( 250.0f, 10.0f, 350.0f, 120.0f);
                    }
                    CheckAttackHit(); // 受 hasTriggeredThisFrame 控制
                    // 设置云的位置并绘制
                    attackCloud->SetPosition(GetHitboxCenterX() + (facingRight ? 110.0f : -450.0f),GetHitboxTopY());
                    attackCloud->Draw();
                    se->Play(false);
                }
                else {
                    DeactivateAttackCollider();
                }

            }

        }

    }
    // 状态锁定期间只更新基础逻辑
    if (state == ATTACKING || state == ATTACKING2 || state == ATTACKING3) {
        if (stateLockTimer > 0) {
            stateLockTimer -= deltaTime;
            Enemy::Update(deltaTime);
            return;
        }
    }

    // 攻击状态结束处理
    if (IsAttackState(state) && currentFrame >= animations[state].frames.size() - 1) {
        state = IDLE;
        actionCooldown = 0.2f; // 攻击后强制短时间冷却
    }

    switch (phase)
    {
    case 1:    // 通用行为决策（只在非冷却时进行）
        phaseTimer += deltaTime;
        thunderDuration = 8.0f;
        if (thunderTimer >= thunderDuration)
        {
            Thunder(player);
            thunderTimer = 0;
        }
        thunderTimer += deltaTime;
        if (health <= 250)
        {
            phase = 2;
            phaseTimer = 0;
        }
        gravity = 0.5f;
        canTakeDamage = true;
        if (actionCooldown <= 0) {
            DecisionMaking(deltaTime, player);
        }
    break;
    case 2:
    {
        phaseTimer += deltaTime;
        thunderDuration = 1.5f;
        if (thunderTimer>=thunderDuration)
        {
            Thunder(player);
            thunderTimer = 0;
        }
        thunderTimer += deltaTime;

        if (phaseTimer >= phase2Duration)
        {
            phaseTimer = 0;
            phase = 3;
            emitter->SetActive(false);  
            break;

        }
        if (!IsPlayerInFront(player))
        {
            facingRight = !facingRight;
        }
        vx = 0;
        vy = 0;
        gravity = 0.0f;
        state = IDLE;  // 请注意要用 '=' 而不是 '==' 做赋值
        canTakeDamage = false;

        static bool isArrived = false;

        const float targetX = 500.0f;
        const float targetY = 5.0f;
        const float moveSpeed = 120.0f; // 随意调大/调小控制移动速度


        // 如果还没到达目标位置，就多帧逐步移动
        if (!isArrived)
        {
            float currentX = GetX();  // 取当前坐标
            float currentY = GetY();

            float dx = targetX - currentX;
            float dy = targetY - currentY;
            float dist = std::sqrt(dx * dx + dy * dy);

            // 距离大于一定阈值 -> 继续往目标靠近
            if (dist > 1.0f)
            {
                // 归一化方向
                float nx = dx / dist;
                float ny = dy / dist;

                float newX = currentX + nx * moveSpeed * deltaTime;
                float newY = currentY + ny * moveSpeed * deltaTime;

                // 用 SetPosition 进行更新，而不是直接操作 x,y
                SetPosition(newX, newY);
            }
            else
            {
                // 距离足够近 -> 视为到达，直接设到目标点
                SetPosition(targetX, targetY);

                isArrived = true;
                // 在这里再开启特效或做下一步
                emitter->SetActive(true);
            }
        }

        // 后面加你的其他逻辑，如果暂时没有就 break
    }
    break;
    case 3:
        vx = 0;
        vy = 0;
        gravity = 0.0f;
        state = IDLE;  // 请注意要用 '=' 而不是 '==' 做赋值
        canTakeDamage = false;
        enemySpawnTimer += deltaTime;
        phaseTimer += deltaTime;
        if (enemySpawnTimer >=enemySpawnDuration&&allCharacters.size()<10 )
        {
            enemySpawnTimer = 0;
            SpawnEnemies();
        }
        if (phaseTimer >= phase3Duration)
        {
            phase = 4;
            phaseTimer = 0;
            for (int i = 0; i < enemies.size(); i++)
            {
                enemies[i]->markForDeletion = true;
            }
        }
    break;
    case 4: 
        phaseTimer += deltaTime;
        thunderDuration = 3.0f;
        if (thunderTimer >= thunderDuration)
        {
            Thunder(player);
            thunderTimer = 0;
        }
        thunderTimer += deltaTime;
        gravity = 0.5f;
        canTakeDamage = true;
        if (actionCooldown <= 0) {
            DecisionMaking(deltaTime, player);
        }
    break;
    default:
        break;

    }


    Enemy::Update(deltaTime);

}
void Boss::SpawnEnemies()
{
    int randtyp=0;
    randtyp = rand() % 100;
    Enemy* enemy = new Enemy(110+11*randtyp, 100, 55, 50, 100, 5, 5);
    enemy->InitializeAnimations();
    enemies.push_back(enemy);
}
// 重载 TakeDamage 方法，Boss 受击时可附加特殊逻辑（例如：在攻击过程中受击减半伤害）
void Boss::TakeDamage(int damage, float attackerX)
{
    // 调用基类方法处理受击、击退和扣血
    if (!canTakeDamage) {
        //printf("无法受击: 无敌状态\n");
        return;
    }

    // 设置受击状态（仅在扣血时触发）
    if (state != ATTACKING && state != ATTACKING2 && state != ATTACKING3)
    {
        // 先执行击退
        ApplyKnockback(attackerX);
        state = HURT;
    }

    hurtTimer = hurtDuration;

    // 原有伤害逻辑
    health -= damage;
    ///printf("受到%d点伤害! 剩余生命值: %d\n", damage, health);

    if (health <= 0) {
        Die();
    }
}

float Boss::DistanceTo(Character* target) {
    float dx = target->GetHitboxCenterX() - GetHitboxCenterX();
    float dy = target->GetHitboxCenterY() - GetHitboxCenterY();
    return std::sqrt(dx * dx + dy * dy);
}
void Boss::ChasePlayer(float deltaTime, Character* player) {

    if (state != RUNNING) {
        state = RUNNING;
        currentFrame = 0; // 重置动画帧
    }
    // 当距离>150时启用冲刺模式
    float currentDist = DistanceTo(player);
    float boostMultiplier = (currentDist > 120.0f) ? 1.8f : 1.0f;

    // 增加角度修正：当玩家在侧面时转向更快
    float dx = player->GetHitboxCenterX() - GetHitboxCenterX();
    if (fabs(dx) > 50.0f) {
        facingRight = (dx > 0);
    }

    vx = chaseSpeed * boostMultiplier * (facingRight ? 1.0f : -1.0f);
}

void Boss::RetreatFromPlayer(float deltaTime, Character* player) {
    if (state != RETREAT) {
        state = RETREAT;
        currentFrame = 0; // 重置动画帧
    }
    // Boss 远离玩家
    float dx = GetHitboxCenterX() - player->GetHitboxCenterX();
    float dy = GetHitboxCenterY() - player->GetHitboxCenterY();
    float len = std::sqrt(dx * dx + dy * dy);
    if (len == 0) return;
    float nx = dx / len;
    float ny = dy / len;
    vx = retreatSpeed * nx;
    facingRight = (nx >= 0);
}

void Boss::DecisionMaking(float deltaTime, Character* player) {
    // 如果距离上次状态切换时间过短，则保持当前状态
    if (player->GetCharacterState() == DEAD)
    {
        vx = 0; 
        vy = 0;
        state = IDLE;
        return;
    }
    if (lastStateSwitchTime > 0) {
        lastStateSwitchTime -= deltaTime;
        return;
    }
    float dist = DistanceTo(player);
    bool inFront = IsPlayerInFront(player);
    float attackDesire = 2.0f;
    if (phase == 4)
    {
        attackDesire = 4.0f; // 基础攻击欲望（可调整）
    }


    if (player->GetCharacterState() == FREEZE)
    {
        attackDesire = 10.0f;
    }


    // 基础行为权重计算
    float weightChase = CalculateChaseWeight(dist,player);
    float weightRetreat = CalculateRetreatWeight(dist);
    float weightAttack = CalculateAttackWeight(dist, inFront);

    // 计算权重差值
    float chaseRetreatDiff = fabs(weightChase - weightRetreat);

    // 如果权重差值小于阈值，则保持当前状态
    if (chaseRetreatDiff < 0.3f) { // 0.2f 是阈值，可调整
        return;
    }

    // 加入平滑随机扰动
    float randomFactor = (rand() % 200 - 100) / 1000.0f; // ±10%扰动
    weightAttack = std::clamp(weightAttack + randomFactor, 0.0f, 1.0f);
    // 如果玩家不在面前，降低攻击权重
    if (state==HURT) {
        weightRetreat *= 1.5f; 
    }
    if (!inFront)
    {
        weightChase *= 1.5f;
        weightAttack *= 0.3f; // 降低70%攻击权重
        weightRetreat *= 0.1f;
    }
    // 行为选择
    if (weightAttack >= weightChase && weightAttack >= weightRetreat) {
        ChooseAttackType(dist, inFront);
    }
    else if (weightChase >= weightRetreat) {
        ChasePlayer(deltaTime, player);
        stateLockTimer = 0.4f;
    }
    else {
        RetreatFromPlayer(deltaTime, player);
        stateLockTimer = 0.3f;
    }
    // 记录状态切换时间
    lastStateSwitchTime = stateSwitchBuffer;
}

float Boss::CalculateChaseWeight(float dist, Character* player) {
    // 追击权重：距离越远权重越高
    float baseWeight = std::clamp((dist - 300.0f) / 390.0f, 0.0f, 1.0f);

    // 如果玩家不在面前，大幅提高追击权重
    if (!IsPlayerInFront(player)) {
        baseWeight += 3.0f; // 增加50%权重
    }
    static float lastDist = 0.0f;
    static bool wasChasing = false;


    // 追击权重：距离越远权重越高，但增加平滑过渡
    if (dist < 50.0f) return 0.0f; // 近距离不追击
    if (dist > 400.0f) return 1.0f; // 远距离强制追击
    return std::clamp(baseWeight, 0.0f, 1.0f);
}

float Boss::CalculateRetreatWeight(float dist) {
    // 撤退权重：距离越近权重越高，但增加平滑过渡
    if (GetHitboxCenterX() <= 0 || GetHitboxCenterX() >= 1280)
    {
        return 0.0f;
    }
    if (dist > 300.0f) return 0.0f; // 远距离不撤退
    if (dist < 10.0f) return 1.0f; // 近距离强制撤退

    return std::clamp((290.0f - dist) / 290.0f, 0.0f, 1.0f);
}

float Boss::CalculateAttackWeight(float dist, bool inFront) {
    // 三段式距离衰减
    float distanceFactor = 0.0f;
    if (dist <= 150.0f) {
        distanceFactor = 2.5f - dist / 150.0f; // 近距离权重高
    }
    else if (dist <= 300.0f) {
        distanceFactor = 1.8f - (dist - 150.0f) / 150.0f * 0.4f; // 中距离中等权重
    }
    else {
        distanceFactor = 1.0f - (dist - 300.0f) / 200.0f * 0.3f; // 远距离低权重
    }

    // 方向修正：侧后方攻击权重减半
    float directionFactor = inFront ? 2.5f : 1.0f;

    return std::clamp(distanceFactor * directionFactor, 0.0f, 1.0f);
}

void Boss::ChooseAttackType(float dist, bool inFront) {
    // 在原有逻辑基础上，增加冷却时间调整
    const float baseCooldown = 0.5f; // 基础冷却时间
    const float maxCooldown = 2.0f;  // 最大冷却时间

    // 根据攻击欲望动态调整冷却
    float cooldown = baseCooldown + (rand() % 100) / 100.0f * (maxCooldown - baseCooldown);
    struct AttackOption {
        CharacterState type;
        float weight;
        float minDist;
        float maxDist;
        bool requireFront;
    };

    const AttackOption options[3] = {
        {ATTACKING,  (phase == 1) ? 1.2f : 0.8f, 0, 80, true},  // 范围攻击
        {ATTACKING2, (phase == 1) ? 1.1f : 0.9f, 30, 140, true},   // 扫尾攻击（更高优先级）
        {ATTACKING3, (phase == 1) ? 0.5f:1.2f, (phase == 1) ? 200 : 100, 400, true}  // 远程攻击
    };

    // 筛选可用攻击
    std::vector<AttackOption> validAttacks;
    for (const auto& opt : options) {
        if (dist >= opt.minDist && dist <= opt.maxDist &&
            (!opt.requireFront || inFront) &&
            attackCooldowns[opt.type - ATTACKING] <= 0) {

            // 距离衰减因子
            float distanceFactor = 1.0f - (dist - opt.minDist) / (opt.maxDist - opt.minDist);
            AttackOption adjustedOpt = opt;
            adjustedOpt.weight *= (0.5f + 0.5f * distanceFactor);
            validAttacks.push_back(adjustedOpt);
        }
    }
    if (validAttacks.empty()) {
        // 这里可以返回一个特殊值，或者设置一个状态
        // return false; 
        // 或者你也可以把 state = IDLE
        // 视你的代码风格而定，只要能让 DecisionMaking 知道“不攻了”
        return;
    }
    // 选择权重最高的攻击
    if (!validAttacks.empty()) {
        auto bestAttack = std::max_element(validAttacks.begin(), validAttacks.end(),
            [](const auto& a, const auto& b) { return a.weight < b.weight; });

        state = bestAttack->type;
        int attackIndex = state - ATTACKING;

        // 动态冷却时间：远程攻击冷却更长
        float cooldownMultiplier = (state == ATTACKING3) ? 3.0f : 0.3f;
        attackCooldowns[attackIndex] = (GetStateDuration(state) + 2.0f) * cooldownMultiplier;

        stateLockTimer = GetStateDuration(state);
        currentFrame = 0;
        actionCooldown = 0.3f; // 延长行为冷却防止抖动
    }
}

bool Boss::IsPlayerInAttackRange(Character* player, CharacterState attackType) {
    float dist = DistanceTo(player);
    switch (attackType) {
    case ATTACKING:
        return dist >= 0 && dist <= 80;
    case ATTACKING2:
        return dist >= 80 && dist <= 120 && IsPlayerInFront(player);
    case ATTACKING3:
        return dist >= 300 && dist <= 400 && IsPlayerInFront(player);
    default:
        return false;
    }
}

////////////////////////宝箱
////////////////////////宝箱
////////////////////////宝箱
TreasureChest::TreasureChest(float x, float y, float w, float h,int type)
    : GameObject(x, y, w, h),type(type)
{
    layer=0;
    RegisterCharacter(this);
    InitializeAnimations();
}


void TreasureChest::Open() {
    switch (type)
    {
    case 1:
    {
        Key * key = new Key(this->GetX() + 10, this->GetY() - 120);
        break;

    }
    case 2:
    {
        Wing * wing = new Wing(this->GetX() + 10, this->GetY() - 120);
        break;

    }
    case 3:
    {
        MagicBall* magicBall = new MagicBall(this->GetX() + 5, this->GetY() - 80);
        break;
    }

    case 4:
    {
        NormalKey* key = new NormalKey(this->GetX() + 10, this->GetY() - 120);
        break;
    }
    default:
        break;

    }
    se->Play(false);
}

void TreasureChest::Update(float deltaTime) {
    // 如果宝箱还未开启，则检测与玩家的碰撞
    if (!isOpen) {
        // 遍历全局角色列表，检查玩家是否碰撞
        for (Character* c : Character::allCharacters) {
            if (c->IsPlayer()) {
                // 简单AABB碰撞检测
                float leftA = GetX();
                float topA = GetY();
                float rightA = leftA + GetWidth();
                float bottomA = topA + GetHeight();

                float leftB = c->GetHitboxX();
                float topB = c->GetHitboxY();
                float rightB = leftB + c->GetWidth();
                float bottomB = topB + c->GetHeight();

                if (leftA < rightB && rightA > leftB && topA < bottomB && bottomA > topB) {
                    state = ATTACKING;
                    se->Play(false);
                    break;
                }
            }
        }
    }
    if (isOpen)
    {
        state = DEAD;
        currentFrame = 0;
    }
    if (state == ATTACKING&&!isOpen)
    {
        if (currentFrame >= animations[ATTACKING].frames.size() - 1)
        {
            currentFrame = animations[ATTACKING].frames.size() - 1;
            isOpen = true;

            Open();

        }
    }
    UpdateAnimation(deltaTime);
    // 宝箱自身不需要其它逻辑
}

//////触发器
TriggerObject::TriggerObject(float x, float y, float w, float h)
    : GameObject(x, y, w, h)
{
    // 初始化时未触发
    hasTriggered = false;
    se->Play(false);
}


void TriggerObject::OnTrigger(Character* c) {
    // 默认行为：输出触发信息，子类可以重写做其他处理
}

void TriggerObject::Update(float deltaTime) {
    // 遍历全局角色列表，检测碰撞
    for (Character* c : Character::allCharacters) {
        // 例如只对玩家触发，或扩展支持其它类型
        if (c->IsPlayer()) {
            float leftA = GetX();
            float topA = GetY();
            float rightA = leftA + GetWidth();
            float bottomA = topA + GetHeight();

            float leftB = c->GetHitboxX();
            float topB = c->GetHitboxY();
            float rightB = leftB + c->GetWidth();
            float bottomB = topB + c->GetHeight();

            if (leftA < rightB && rightA > leftB && topA < bottomB && bottomA > topB) {
                if (!hasTriggered) {
                    OnTrigger(c);
                    hasTriggered = true; // 只触发一次，或根据需要重置
                }
                break;
            }
        }
    } 

    if (hasTriggered)
        markForDeletion = true;
    UpdateAnimation(deltaTime);
}
