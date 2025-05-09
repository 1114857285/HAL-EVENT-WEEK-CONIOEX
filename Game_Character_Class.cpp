#include "Game_Character_Class.h"
#include <functional>
#include <algorithm>
//Game_Character_Class.cpp
// 
// ȫ�ֱ�������
bool g_keyStates[256] = { false };
bool g_prevKeyStates[256] = { false };
GameMap* Character::currentMap = nullptr;
// ȫ�ֽ�ɫ�б��ʼ��
std::vector<Character*> Character::allCharacters;
std::vector<UI*> UI::UIs;
std::vector<BGMPlayer*>BGMPlayer::bgms;
BGMPlayer* TriggerObject::se = new BGMPlayer("game-bonus-144751.mp3");
BGMPlayer* Lighting::se = new BGMPlayer("thunder-for-anime-161022.mp3");
BGMPlayer* Projectile::se = new BGMPlayer("medium-explosion-40472.mp3");
BGMPlayer* Boss::se = new BGMPlayer("scream-noise-142446.mp3");
BGMPlayer* TreasureChest::se = new BGMPlayer("chest-opening-87569.mp3");
BGMPlayer* Lock::openSE = new BGMPlayer("unlock-the-door-1-46012.mp3");

// ���°���״̬
void ClearKeyStates() {
    for (int i = 0; i < 256; ++i) {
        g_prevKeyStates[i] = false; // ������һ֡�İ���״̬
        g_keyStates[i] = false; // ���µ�ǰ֡�İ���״̬
    }
}
// ���°���״̬
void UpdateKeyStates() {
    for (int i = 0; i < 256; ++i) {
        g_prevKeyStates[i] = g_keyStates[i]; // ������һ֡�İ���״̬
        g_keyStates[i] = (GetAsyncKeyState(i) & 0x8000) != 0; // ���µ�ǰ֡�İ���״̬
    }
}

// ��ⰴ���Ƿ���
bool GetKeyDown(int key) {
    return g_keyStates[key] && !g_prevKeyStates[key];
}

// ��ⰴ���Ƿ��ͷ�
bool GetKeyUp(int key) {
    return !g_keyStates[key] && g_prevKeyStates[key];
}

// ��ⰴ���Ƿ��������
bool GetKeyHold(int key) {
    return g_keyStates[key];
}

// Character ��Ĺ��캯��ʵ��
Character::Character(float startX, float startY, float w, float h, float offsetX, float offsetY, Type type)
    : x(startX), y(startY), vx(0), vy(0), width(w), height(h),
    hitboxOffsetX(offsetX), hitboxOffsetY(offsetY), isOnGround(false),
    state(IDLE), currentFrame(0), frameCounter(0), characterType(type) {
    // ��ʼ������
    for (int i = 0; i < 5; i++) {
        animations[i] = Animation{};
    }
    //printf("[Character] Created at %p\n", this); // ��ӡ��ַ

}


//// ע���ɫ��ȫ���б�
void Character::RegisterCharacter(Character* character) {
    if (std::find(allCharacters.begin(), allCharacters.end(), character) == allCharacters.end()) {
        allCharacters.push_back(character);
    }
}
// ��ȫ���б��Ƴ���ɫ
void Character::UnregisterCharacter(Character* character) {
    auto it = std::remove(allCharacters.begin(), allCharacters.end(), character);
    if (it != allCharacters.end()) {
        allCharacters.erase(it, allCharacters.end());
    }
}
// ========== ��ɫ�����ж� ==========
void Character::ActivateAttackCollider(float offsetX, float offsetY, float attackWidth, float attackHeight) {
    // �����ɫ��ײ������Ͻ�λ�ã���ײ���λ���ǽ�ɫλ�ü�����ײ��ƫ�ƣ�
    float hitboxX = x + hitboxOffsetX;
    float hitboxY = y + hitboxOffsetY;

    // �����ɫ��ײ������ĵ�
    float centerX = hitboxX + width * 0.5f;
    float centerY = hitboxY + height * 0.5f;

    // ���ݽ�ɫ�������ˮƽƫ�ƣ���������Ҳ�����ƫ�ƣ�����ȡ��
    float effectiveOffsetX = (facingRight ? offsetX : -offsetX);

    // ���㹥����ײ������ĵ㣨����ڽ�ɫ��ײ�����ļ���ƫ�ƣ�
    float colliderCenterX = centerX + effectiveOffsetX;
    float colliderCenterY = centerY + offsetY;

    // ���ݹ�����ײ�����ĵ��ָ���ĳߴ磬���������Ͻ�����
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

        // ��ײ����߼�
        const float otherX = allCharacters[i]->GetHitboxX();
        const float otherY = allCharacters[i]->GetHitboxY();
        const float otherW = allCharacters[i]->GetWidth();
        const float otherH = allCharacters[i]->GetHeight();

        if (attackCollider.x < otherX + otherW &&
            attackCollider.x + attackCollider.width > otherX &&
            attackCollider.y < otherY + otherH &&
            attackCollider.y + attackCollider.height > otherY)
        {

            // �����ĵ���Ϊ�������룬���ڼ�����˷���
            if (state == ATTACKING3)
            {
                allCharacters[i]->state = FREEZE;
            }
            allCharacters[i]->TakeDamage(20, this->GetHitboxCenterX());  // thisָ�򹥻���
            hitTargets.insert(allCharacters[i]);
        }

    }

    hasTriggeredThisFrame = true; // ��Ǳ�֡�Ѵ���
}


void Character::TakeDamage(int damage, float attackerX) {
    if (!canTakeDamage) {
        //printf("�޷��ܻ�: �޵�״̬\n");
        return;
    }

    if (state != FREEZE)
    {
        // ��ִ�л���
        ApplyKnockback(attackerX);

        // �����ܻ�״̬�����ڿ�Ѫʱ������
        state = HURT;
        hurtTimer = hurtDuration;
    }
    else
    {
        // ��ִ�л���
        //ApplyKnockback(attackerX);
        freezeTimer = freezeDuration;
    }


    // ԭ���˺��߼�
    health -= damage;
    ///printf("�ܵ�%d���˺�! ʣ������ֵ: %d\n", damage, health);

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

    markForDeletion = true; // ���Ϊ��ɾ��
}

////////////// Character ��� LoadAnimation ʵ��/////////////////
bool Character::LoadAnimation(CharacterState action, const std::vector<std::string>& framePaths, float frameTime) {
    std::lock_guard<std::mutex> lock(animationMutex); // ����

    animations[action].frames.clear(); // ���֮ǰ��֡
    animations[action].frameTime = frameTime;

    for (const auto& path : framePaths) {
        std::string fullPath = "pic/" + path;
        auto bmp = std::shared_ptr<Bmp>(LoadBmp(fullPath.c_str()), [](Bmp* bmp) { if (bmp) DeleteBmp(&bmp); });
        if (!bmp) {
            //printf("Error: Failed to load BMP file: %s\n", fullPath.c_str());
            animations[action].frames.clear(); // ����ʧ��ʱ���֡
            return false;
        }
        animations[action].frames.push_back(bmp);
    }
    return true;
}

////////////// Character ��� Update ʵ��/////////////
void Character::Update(float deltaTime)
{
    
    // 1) �ܻ���ʱ��
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
        // ÿ֡������ vx, vy = 0
        vx = 0;
        vy = 0;

        // �ݼ���ʱ
        freezeTimer -= deltaTime;

        // ���ʱ�䵽�ˣ����˻ص� IDLE
        if (freezeTimer <= 0) {
            freezeTimer = 0;   // ����������������0
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
    // 2) ������һ֡����״̬
    wasOnGround = isOnGround;
    // ��ʼ��֡����Ϊ���ڵ���
    isOnGround = false;

    // 3) �������ų�Dash/Knockback��
    if (!isDashing && !isKnockback) {
        // ԭ������ *60, ������Ŀ֡���������ﱣ��
        vy += gravity * 60 * deltaTime;
    }

    // 4) ���㱾֡�ƶ���
    float moveX = vx * 60 * deltaTime;
    float moveY = vy * 60 * deltaTime;

    // ============= ˮƽ������ײ (����/С��) =============
    {
        float newX = x;
        float step = (moveX > 0) ? 1.0f : -1.0f;
        int steps = static_cast<int>(fabs(moveX));  // �����ػ�ÿ��1

        for (int i = 0; i < steps; ++i) {
            float testX = newX + step;
            // �����ײ
            if (currentMap && currentMap->CheckCollision(
                testX + hitboxOffsetX,
                y + hitboxOffsetY,
                width,
                height))
            {
                // ײ��ǽ => vx=0
                vx = 0;
                break;
            }
            else {
                newX = testX; // �����ƶ�
            }
        }
        // ��� moveX ����1����(С��1)����һ���Դ���
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

    // ============= ��ֱ������ײ (����/С��) =============
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
                // ײ������/ƽ̨(��step>0��ʾ����)
                if (step > 0) {
                    // ���
                    isOnGround = true;
                    vy = 0;
                }
                else {
                    // ײ������
                    vy = 0;
                }
                break;
            }
            else {
                newY = testY;
            }
        }

        // ��������
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
                    // ���
                    isOnGround = true;
                    vy = 0;
                }
                else {
                    // ͷ��
                    vy = 0;
                }
            }
            else {
                newY = testY;
            }
        }

        y = newY;
    }

    // 5) �����֡����� => ���ÿ���ϵͳ
    if (!wasOnGround && isOnGround) {
        ResetAirSystem();
    }

    // 6) ��̼�ʱ / �˺� / ����
    UpdateDash(deltaTime);
    if (isKnockback) {
        knockbackTimer -= deltaTime;
        if (knockbackTimer <= 0) {
            isKnockback = false;
            vx *= 0.5f;
        }
    }
    // ������ڻ�����ȴ�У��ݼ� canKnockbackTimer
    if (!canKnockback) {
        canKnockbackTimer -= deltaTime;
        if (canKnockbackTimer <= 0) {
            canKnockback = true;
        }
    }

    // 7) ��������
    UpdateAnimation(deltaTime);

    // 8) ������ײ��
    if (attackCollider.active) {
        CheckAttackHit();
    }
    if (state != ATTACKING && attackCollider.active) {
        DeactivateAttackCollider();
        hitTargets.clear();
    }

    // 9) ����ɫ֮����ײ(�˺�)
    if (state != HURT) {
        auto collided = CheckCollisionWithOthers();
        for (Character* other : collided) {
            // �����ײ������һ��Ϊ Boss ����һ��Ϊ Lighting����������ײ����
            if ((this->IsBoss() && dynamic_cast<Lighting*>(other) != nullptr) ||
                (dynamic_cast<Lighting*>(this) != nullptr && other->IsBoss()))
            {
                continue;
            }
            // ���˫�����ǵ��ˣ����໥����
            if (this->IsEnemy() && other->IsEnemy()) {
                // ����˫����ײ�е����ĵ� X ����
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
            // ����Է��ǵ��ˣ�����ǰ�����ǵ��ˣ���ǰ�����ܵ��˺�
            if (auto enemy = dynamic_cast<Enemy*>(other)) {
                if (!this->IsEnemy()) {
                    float enemyCenterX = enemy->GetHitboxCenterX();
                    this->TakeDamage(10, enemyCenterX);
                    break;
                }
            }
        }
    }

    // ���¶���֡���
    if (currentFrame != previousAnimationFrame) {
        hasTriggeredThisFrame = false;
        previousAnimationFrame = currentFrame;
    }
}

////////////////// Character ��� Draw ʵ��//////////////////
void Character::Draw() {
    std::lock_guard<std::mutex> lock(animationMutex); // ����
    if (!animations[state].frames.empty() && currentFrame < animations[state].frames.size()) {
        DrawBmp(x, y, animations[state].frames[currentFrame].get(), ((facingRight) ? 0 : BMP_HINV), true);//��Ԫ��������Ƿ�ת
    }

}

//////////// Character ��� UpdateAnimation ʵ��////////////////////////
void Character::UpdateAnimation(float deltaTime) {
    std::lock_guard<std::mutex> lock(animationMutex); // ����
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

        // ƽ�����������10%ʱ����٣�
        if (dashTimer < dashDuration * 0.1f) {
            vx *= 0.9f;
        }

        if (dashTimer <= 0) {
            isDashing = false;
            vx = 0;
            canTakeDamage = true;
            //printf("��̽���\n");
        }
    }
}


// Player ��Ĺ��캯��ʵ��
Player::Player(float startX, float startY, float w, float h, float offsetX, float offsetY,Type type)
    : Character(startX, startY, w, h, offsetX, offsetY, type) {
    RegisterCharacter(this); // ע�ᵽȫ���б�
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

// Player ��� InitializeAnimations ʵ��
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



    return true;    //ȫ�����غ󷵻�Ϊ�棬�¼Ӷ���Ҫ�ӵ��Ϸ�


}
// ========== ����๥����չ ==========

// Player ��� Update ʵ��
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

    // ״̬�仯ʱǿ�ƹرչ�����ײ��
    if (state != ATTACKING &&state !=ATTACKING2&& attackCollider.active) {
        DeactivateAttackCollider();
        hitTargets.clear(); // ������м�¼
        //printf("״̬�л���ǿ�ƹرչ�����ײ��\n");
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
            // �����״ν���ù���֡ʱ��ռ�¼
            if (!hasTriggeredThisFrame) {
                OnAttackFrameStart(); // ��� hitTargets
                ActivateAttackCollider(55.0f, 5.0f, 60.0f, 65.0f);
            }
            CheckAttackHit(); // �� hasTriggeredThisFrame ����
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
            // ������������������ hasTriggeredThisFrame
            hasTriggeredThisFrame = false;
        }
        else if (isAttackFrame5 && !hasTriggeredThisFrame) {
            // ͬһֻ֡��ִ��һ��
            Shoot();
            hasTriggeredThisFrame = true;  // ����Ѵ���
        }
    }
    if (state != HURT) {
        auto collided = CheckCollisionWithOthers();
        for (Character* other : collided) {
            if (auto enemy = dynamic_cast<Enemy*>(other)) {
                // ��������ˣ����˲�����ͨ��ײ�˺�
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
    // ������
    if (inputDir == 0) {
        vx = facingRight ? dashSpeed : -dashSpeed;
        vy = 0;
    }
    else {
        vx = dashSpeed * inputDir;
        facingRight = (inputDir > 0);
        vy = 0;
    }

    // ��Դ����
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
    // 1) �����ӵ���ʼ����(�������λ��+����΢��)
    float bulletX = GetHitboxCenterX() + (facingRight ? 25.0f : -60.0f);
    float bulletY = GetHitboxCenterY()-12.0f;

    // 2) �����ӵ��ٶ�(���ݳ���)
    float speed = 6.0f; // �ɵ�
    float vxBullet = facingRight ? speed : -speed;
    float vyBullet = 0;

    // 3) �����ӵ�(��������10x10, offset����0)
    PlayerProjectile* bullet = new PlayerProjectile(
        bulletX, bulletY,
        30.0f, 30.0f,
        0, 0,
        15,           // �ӵ��˺�
        vxBullet, vyBullet
    );

    bullet->InitializeAnimations();

    // 4) ����ȫ�ֽ�ɫ�б�(�����Լ��Ķ����������)
    Character::RegisterCharacter(bullet);
    magicSE->Play(false);
    Cost(10);
}
// Player ��� HandleInput ʵ��
void Player::HandleInput() {
    CharacterState previousState = state; // ��¼֮ǰ��״̬
    bool isAttacking = false;
    if (state == ATTACKING || state == ATTACKING2)
         isAttacking = true;
    // === ״̬���ȼ����� ===
    if (state == HURT || isDashing) return;

    // === �ƶ����� ===
    float inputDir = 0.0f;
    if (!isAttacking)
    {
        if (GetKeyHold(PK_LEFT) || GetKeyHold(PK_A)) inputDir -= 1.0f;
        if (GetKeyHold(PK_RIGHT) || GetKeyHold(PK_D)) inputDir += 1.0f;
    }


    // === �ٶȿ��� ===
    if (!isDashing) {
        vx = inputDir * moveSpeed;
        if (inputDir != 0) facingRight = (inputDir > 0);
    }

    // === ״̬�л� ===
    if (isOnGround) {
        state = (inputDir != 0) ? RUNNING : IDLE;
    }
    else {
        state = (vy < 0) ? JUMPING :JUMPING;
    }

    // === ��Ծϵͳ ===
    if (GetKeyDown(PK_SP) && !isAttacking) {
        if (isOnGround) {
            vy = jumpForce;
            isOnGround = false;
            jumpSE->SetVolume(50);
            jumpSE->Play(false);
        }
    }

    // === ����ϵͳ ===
    if (GetKeyHold(PK_Z)) {
        state = ATTACKING;
        attackingSE->Play(true);
    }
    else { attackingSE->Stop(); }
    // === ����ϵͳ ===
    if (GetKeyHold(PK_X)&&canShoot) {
        state = ATTACKING2;

    }
    //cure
    if (GetKeyDown(PK_C))
    {
        Cure();
    }

    // === ���ϵͳ ===
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
            StartDash(inputDir); // ���뵱ǰ���뷽��
        }
    }

    // === ����ͬ�� ===
    if (state != previousState) {
        currentFrame = 0;
        frameCounter = 0;
    }
}

// Enemy ��Ĺ��캯��ʵ��
Enemy::Enemy(float startX, float startY, float w, float h, float range, float offsetX, float offsetY, Type type)
    : Character(startX, startY, w, h, offsetX, offsetY, type ), patrolRange(range), startX(startX),movingRight(true) {
    RegisterCharacter(this); // ע�ᵽȫ���б�

}
// ========== �����๥����չ ==========



void Enemy::TakeDamage(int damage, float attackerX) {
    if (!canTakeDamage) return;
    Character::TakeDamage(damage, attackerX);
    // ��ִ�л���
    ApplyKnockback(attackerX);  // ���ü̳����Ļ��˷���

    // �����ض��߼�����Ҫ��Ѫʱ��ִ������
    health -= damage;
    if (health > 0) {
        //printf("Enemy took damage! Remaining health: %d\n", health);
        state = HURT;  // �������ʱ�ű����ܻ�״̬
        hurtTimer = hurtDuration;
    }
    else {
        Die();
    }
}
// Enemy ��� InitializeAnimations ʵ��
bool Enemy::InitializeAnimations() {
    if (!LoadAnimation(IDLE, { "enemy_idle_1.bmp" }, 0.3f)) return false;
    if (!LoadAnimation(RUNNING, { "enemy_run_1.bmp" }, 0.2f)) return false;
    if (!LoadAnimation(HURT, { "enemy_red.bmp" }, 0.3f)) return false;
    return true;
}

// Enemy ��� Update ʵ��
void Enemy::Update(float deltaTime) {
     Patrol(); 
    Character::Update(deltaTime);
}

// Enemy ��� Patrol ʵ��
void Enemy::Patrol() {
    if (this->IsBoss()||state==HURT)
        return;
    if (x > 1300 || x < -10)
    {
        markForDeletion = true;
    }
    if (state == HURT || isKnockback||state==ATTACKING) return; // isKnockback�ж�
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
    // ���ݵ�ǰ�ٶȸ���λ��
    // ע�⣺������ù̶�֡��ת��������60������Ҳ���Ը��������Ŀ����Ӧ����
    x += vx * 60 * deltaTime;
    y += vy * 60 * deltaTime;

    // ������ͼ����ײ�����Σ�
    // ʹ�ý�ɫ����ײ��λ�ã�x + hitboxOffsetX, y + hitboxOffsetY, ���Ϊ width, height
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

    // �����������ɫ����ײ���������ǣ�
    // �����������е� CheckCollisionWithOthers �����������뱾��ɫ��ײ�Ľ�ɫ�б�
    std::vector<Character*> collided = CheckCollisionWithOthers();
    for (Character* other : collided)
    {
        // ���������ɫ����ң����������˺�
        if (other->IsPlayer()) {
            other->TakeDamage(damage, GetHitboxCenterX());  // �˴����� x ��Ϊ���������꣬ʵ�ʿ��Ը�����Ҫ����
            collisionEnabled = false;        
            state = DEAD;
            x -= 128;
            y -= 128;
            se->Play(false);
            break;
        }
    }

    // ��ѡ��������е��߷ɳ���Ļ��Χ��Ҳ���ɾ��
    // ������Ļ��1280����720��������Ŀʵ�����������
    if (x < -width || x > 1280 || y < -height || y > 720) {
        markForDeletion = true;
    }
}


bool PlayerProjectile::InitializeAnimations()
{
    // ����������ͼ·�������� "player_bullet.bmp"
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
    // 1) �����ٶȸ�������
    x += vx * 60.0f * deltaTime;
    y += vy * 60.0f * deltaTime;

    // 2) ��ͼ��ײ���
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

    // 3) �����������ɫ��ײ
    //    ����ֻ�Ե�������˺�
    std::vector<Character*> collided = CheckCollisionWithOthers();
    for (Character* other : collided)
    {
        if (other->IsEnemy()&& state != DEAD) {
            other->TakeDamage(damage, GetHitboxX()); // �˺� + ���˷���
            collisionEnabled = false;
            state = DEAD;
            x -= 128;
            y -= 128;
            se->Play(false);
            break;
        }
        else if (other->IsFallingChest() &&state != DEAD)
        {
            other->TakeDamage(0,GetHitboxCenterX()); // �˺� + ���˷���
            collisionEnabled = false;
            state = DEAD;
            x -= 128;
            y -= 128;
            se->Play(false);
            break;
        }
    }
    
    // 4) ���ɳ���ĻҲɾ��
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




// Boss �๹�캯�������û��� Enemy �Ĺ��캯��������ʼ�� Boss ���е�����
Boss::Boss(float startX, float startY, float w, float h, float range, float offsetX, float offsetY, Type type)
    : Enemy(startX, startY, w, h, range, offsetX, offsetY, type),
    phase(1),
    chaseThreshold(300.0f),
    retreatThreshold(150.0f),
    patrolSpeed(1.0f),
    chaseSpeed(4.0f),
    retreatSpeed(6.0f)
{
    // ���� Boss ������ֵ��ʹ�����ͨ���˸�����
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
    // ע�⣺Enemy ���캯���п����Ѿ������� RegisterCharacter��
    // ��û�У�������ڴ˵��ã�
    // RegisterCharacter(this);
    bossUI = new BossUI(150, 660);
};

// ��ʼ�� Boss ����������ʾ�������˿��С��������ܻ��Ķ�����
bool Boss::InitializeAnimations()
{
    // ���� Boss ���ж���
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

    // ���� Boss �����������ɸ���ʵ���������֡���Ͳ���ʱ�䣩
    if (!LoadAnimation(ATTACKING, { "Boss_Attack_1.bmp", "Boss_Attack_2.bmp", "Boss_Attack_3.bmp", "Boss_Attack_4.bmp",
         "Boss_Attack_5.bmp", "Boss_Attack_6.bmp", "Boss_Attack_7.bmp", "Boss_Attack_8.bmp", 
         "Boss_Attack_9.bmp", "Boss_Attack_10.bmp", "Boss_Attack_11.bmp", "Boss_Attack_12.bmp", 
         "Boss_Attack_13.bmp", "Boss_Attack_14.bmp", "Boss_Attack_15.bmp", "Boss_Attack_16.bmp", }, 0.08f))
        return false;
    // ���� Boss �����������ɸ���ʵ���������֡���Ͳ���ʱ�䣩
    if (!LoadAnimation(ATTACKING2, { "Boss_Attack2_1.bmp", "Boss_Attack2_2.bmp", "Boss_Attack2_3.bmp", "Boss_Attack2_4.bmp",
         "Boss_Attack2_5.bmp", "Boss_Attack2_6.bmp", "Boss_Attack2_7.bmp" }, 0.08f))
        return false;
    // ���� Boss �����������ɸ���ʵ���������֡���Ͳ���ʱ�䣩
    if (!LoadAnimation(ATTACKING3, { "Boss_Attack3_1.bmp", "Boss_Attack3_2.bmp", "Boss_Attack3_3.bmp", "Boss_Attack3_4.bmp",
         "Boss_Attack3_5.bmp", "Boss_Attack3_6.bmp", "Boss_Attack3_7.bmp","Boss_Attack3_8.bmp", "Boss_Attack3_9.bmp", "Boss_Attack3_10.bmp" }, 0.15f))
        return false;

    // ���� Boss �ܻ�����
    if (!LoadAnimation(HURT, { "Boss_Hurt_1.bmp", "Boss_Hurt_2.bmp", "Boss_Hurt_3.bmp" }, 0.2f))
        return false;
    // ���� Boss dead����
    if (!LoadAnimation(DEAD, { "Boss_Dead_1.bmp","Boss_Dead_1.bmp","Boss_Dead_1.bmp","Boss_Dead_1.bmp", "Boss_Dead_2.bmp", "Boss_Dead_3.bmp" }, 0.4f))
        return false;

    return true;
}

void Boss:: Thunder(Character* target) {

    auto tgt = target;
    if (!tgt) return;  // ���Ŀ���Ѳ����ڣ���ֱ�ӷ���

    float targetX = tgt->GetHitboxCenterX()-40-77;
    float targetY = tgt->GetHitboxCenterY();

    Lighting* thunder = new Lighting(targetX, -70);

}

// ���� Update ��������� Boss ���е���Ϊ�߼�
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
    // ����������ȴʱ��
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
        // û����ң������й�����Ϊ
        //Patrol(deltaTime);
        Enemy::Update(deltaTime);
        return;
    }


    // ״̬�仯ʱǿ�ƹرչ�����ײ��
    if (state != ATTACKING && state != ATTACKING2 && state != ATTACKING3 && attackCollider.active) {
        DeactivateAttackCollider();
        hitTargets.clear(); // ������м�¼
        //printf("״̬�л���ǿ�ƹرչ�����ײ��\n");
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
                // �����״ν���ù���֡ʱ��ռ�¼
                if (!hasTriggeredThisFrame) {
                    OnAttackFrameStart(); // ��� hitTargets
                    ActivateAttackCollider(0.0f, 0.0f, 180.0f, 150.0f);
                }
                CheckAttackHit(); // �� hasTriggeredThisFrame ����
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
                // �����״ν���ù���֡ʱ��ռ�¼
                if (!hasTriggeredThisFrame) {
                    OnAttackFrameStart(); // ��� hitTargets
                    //attackCollider.active = true;
                    ActivateAttackCollider(95.0f, 30.0f, 65.0f, 90.0f);
                }
                CheckAttackHit(); // �� hasTriggeredThisFrame ����
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

                    // �����״ν���ù���֡ʱ��ռ�¼
                    if (!hasTriggeredThisFrame) {
                        OnAttackFrameStart(); // ��� hitTargets
                        ActivateAttackCollider( 250.0f, 10.0f, 350.0f, 120.0f);
                    }
                    CheckAttackHit(); // �� hasTriggeredThisFrame ����
                    // �����Ƶ�λ�ò�����
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
    // ״̬�����ڼ�ֻ���»����߼�
    if (state == ATTACKING || state == ATTACKING2 || state == ATTACKING3) {
        if (stateLockTimer > 0) {
            stateLockTimer -= deltaTime;
            Enemy::Update(deltaTime);
            return;
        }
    }

    // ����״̬��������
    if (IsAttackState(state) && currentFrame >= animations[state].frames.size() - 1) {
        state = IDLE;
        actionCooldown = 0.2f; // ������ǿ�ƶ�ʱ����ȴ
    }

    switch (phase)
    {
    case 1:    // ͨ����Ϊ���ߣ�ֻ�ڷ���ȴʱ���У�
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
        state = IDLE;  // ��ע��Ҫ�� '=' ������ '==' ����ֵ
        canTakeDamage = false;

        static bool isArrived = false;

        const float targetX = 500.0f;
        const float targetY = 5.0f;
        const float moveSpeed = 120.0f; // �������/��С�����ƶ��ٶ�


        // �����û����Ŀ��λ�ã��Ͷ�֡���ƶ�
        if (!isArrived)
        {
            float currentX = GetX();  // ȡ��ǰ����
            float currentY = GetY();

            float dx = targetX - currentX;
            float dy = targetY - currentY;
            float dist = std::sqrt(dx * dx + dy * dy);

            // �������һ����ֵ -> ������Ŀ�꿿��
            if (dist > 1.0f)
            {
                // ��һ������
                float nx = dx / dist;
                float ny = dy / dist;

                float newX = currentX + nx * moveSpeed * deltaTime;
                float newY = currentY + ny * moveSpeed * deltaTime;

                // �� SetPosition ���и��£�������ֱ�Ӳ��� x,y
                SetPosition(newX, newY);
            }
            else
            {
                // �����㹻�� -> ��Ϊ���ֱ���赽Ŀ���
                SetPosition(targetX, targetY);

                isArrived = true;
                // �������ٿ�����Ч������һ��
                emitter->SetActive(true);
            }
        }

        // �������������߼��������ʱû�о� break
    }
    break;
    case 3:
        vx = 0;
        vy = 0;
        gravity = 0.0f;
        state = IDLE;  // ��ע��Ҫ�� '=' ������ '==' ����ֵ
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
// ���� TakeDamage ������Boss �ܻ�ʱ�ɸ��������߼������磺�ڹ����������ܻ������˺���
void Boss::TakeDamage(int damage, float attackerX)
{
    // ���û��෽�������ܻ������˺Ϳ�Ѫ
    if (!canTakeDamage) {
        //printf("�޷��ܻ�: �޵�״̬\n");
        return;
    }

    // �����ܻ�״̬�����ڿ�Ѫʱ������
    if (state != ATTACKING && state != ATTACKING2 && state != ATTACKING3)
    {
        // ��ִ�л���
        ApplyKnockback(attackerX);
        state = HURT;
    }

    hurtTimer = hurtDuration;

    // ԭ���˺��߼�
    health -= damage;
    ///printf("�ܵ�%d���˺�! ʣ������ֵ: %d\n", damage, health);

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
        currentFrame = 0; // ���ö���֡
    }
    // ������>150ʱ���ó��ģʽ
    float currentDist = DistanceTo(player);
    float boostMultiplier = (currentDist > 120.0f) ? 1.8f : 1.0f;

    // ���ӽǶ�������������ڲ���ʱת�����
    float dx = player->GetHitboxCenterX() - GetHitboxCenterX();
    if (fabs(dx) > 50.0f) {
        facingRight = (dx > 0);
    }

    vx = chaseSpeed * boostMultiplier * (facingRight ? 1.0f : -1.0f);
}

void Boss::RetreatFromPlayer(float deltaTime, Character* player) {
    if (state != RETREAT) {
        state = RETREAT;
        currentFrame = 0; // ���ö���֡
    }
    // Boss Զ�����
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
    // ��������ϴ�״̬�л�ʱ����̣��򱣳ֵ�ǰ״̬
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
        attackDesire = 4.0f; // ���������������ɵ�����
    }


    if (player->GetCharacterState() == FREEZE)
    {
        attackDesire = 10.0f;
    }


    // ������ΪȨ�ؼ���
    float weightChase = CalculateChaseWeight(dist,player);
    float weightRetreat = CalculateRetreatWeight(dist);
    float weightAttack = CalculateAttackWeight(dist, inFront);

    // ����Ȩ�ز�ֵ
    float chaseRetreatDiff = fabs(weightChase - weightRetreat);

    // ���Ȩ�ز�ֵС����ֵ���򱣳ֵ�ǰ״̬
    if (chaseRetreatDiff < 0.3f) { // 0.2f ����ֵ���ɵ���
        return;
    }

    // ����ƽ������Ŷ�
    float randomFactor = (rand() % 200 - 100) / 1000.0f; // ��10%�Ŷ�
    weightAttack = std::clamp(weightAttack + randomFactor, 0.0f, 1.0f);
    // �����Ҳ�����ǰ�����͹���Ȩ��
    if (state==HURT) {
        weightRetreat *= 1.5f; 
    }
    if (!inFront)
    {
        weightChase *= 1.5f;
        weightAttack *= 0.3f; // ����70%����Ȩ��
        weightRetreat *= 0.1f;
    }
    // ��Ϊѡ��
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
    // ��¼״̬�л�ʱ��
    lastStateSwitchTime = stateSwitchBuffer;
}

float Boss::CalculateChaseWeight(float dist, Character* player) {
    // ׷��Ȩ�أ�����ԽԶȨ��Խ��
    float baseWeight = std::clamp((dist - 300.0f) / 390.0f, 0.0f, 1.0f);

    // �����Ҳ�����ǰ��������׷��Ȩ��
    if (!IsPlayerInFront(player)) {
        baseWeight += 3.0f; // ����50%Ȩ��
    }
    static float lastDist = 0.0f;
    static bool wasChasing = false;


    // ׷��Ȩ�أ�����ԽԶȨ��Խ�ߣ�������ƽ������
    if (dist < 50.0f) return 0.0f; // �����벻׷��
    if (dist > 400.0f) return 1.0f; // Զ����ǿ��׷��
    return std::clamp(baseWeight, 0.0f, 1.0f);
}

float Boss::CalculateRetreatWeight(float dist) {
    // ����Ȩ�أ�����Խ��Ȩ��Խ�ߣ�������ƽ������
    if (GetHitboxCenterX() <= 0 || GetHitboxCenterX() >= 1280)
    {
        return 0.0f;
    }
    if (dist > 300.0f) return 0.0f; // Զ���벻����
    if (dist < 10.0f) return 1.0f; // ������ǿ�Ƴ���

    return std::clamp((290.0f - dist) / 290.0f, 0.0f, 1.0f);
}

float Boss::CalculateAttackWeight(float dist, bool inFront) {
    // ����ʽ����˥��
    float distanceFactor = 0.0f;
    if (dist <= 150.0f) {
        distanceFactor = 2.5f - dist / 150.0f; // ������Ȩ�ظ�
    }
    else if (dist <= 300.0f) {
        distanceFactor = 1.8f - (dist - 150.0f) / 150.0f * 0.4f; // �о����е�Ȩ��
    }
    else {
        distanceFactor = 1.0f - (dist - 300.0f) / 200.0f * 0.3f; // Զ�����Ȩ��
    }

    // ������������󷽹���Ȩ�ؼ���
    float directionFactor = inFront ? 2.5f : 1.0f;

    return std::clamp(distanceFactor * directionFactor, 0.0f, 1.0f);
}

void Boss::ChooseAttackType(float dist, bool inFront) {
    // ��ԭ���߼������ϣ�������ȴʱ�����
    const float baseCooldown = 0.5f; // ������ȴʱ��
    const float maxCooldown = 2.0f;  // �����ȴʱ��

    // ���ݹ���������̬������ȴ
    float cooldown = baseCooldown + (rand() % 100) / 100.0f * (maxCooldown - baseCooldown);
    struct AttackOption {
        CharacterState type;
        float weight;
        float minDist;
        float maxDist;
        bool requireFront;
    };

    const AttackOption options[3] = {
        {ATTACKING,  (phase == 1) ? 1.2f : 0.8f, 0, 80, true},  // ��Χ����
        {ATTACKING2, (phase == 1) ? 1.1f : 0.9f, 30, 140, true},   // ɨβ�������������ȼ���
        {ATTACKING3, (phase == 1) ? 0.5f:1.2f, (phase == 1) ? 200 : 100, 400, true}  // Զ�̹���
    };

    // ɸѡ���ù���
    std::vector<AttackOption> validAttacks;
    for (const auto& opt : options) {
        if (dist >= opt.minDist && dist <= opt.maxDist &&
            (!opt.requireFront || inFront) &&
            attackCooldowns[opt.type - ATTACKING] <= 0) {

            // ����˥������
            float distanceFactor = 1.0f - (dist - opt.minDist) / (opt.maxDist - opt.minDist);
            AttackOption adjustedOpt = opt;
            adjustedOpt.weight *= (0.5f + 0.5f * distanceFactor);
            validAttacks.push_back(adjustedOpt);
        }
    }
    if (validAttacks.empty()) {
        // ������Է���һ������ֵ����������һ��״̬
        // return false; 
        // ������Ҳ���԰� state = IDLE
        // ����Ĵ����������ֻҪ���� DecisionMaking ֪���������ˡ�
        return;
    }
    // ѡ��Ȩ����ߵĹ���
    if (!validAttacks.empty()) {
        auto bestAttack = std::max_element(validAttacks.begin(), validAttacks.end(),
            [](const auto& a, const auto& b) { return a.weight < b.weight; });

        state = bestAttack->type;
        int attackIndex = state - ATTACKING;

        // ��̬��ȴʱ�䣺Զ�̹�����ȴ����
        float cooldownMultiplier = (state == ATTACKING3) ? 3.0f : 0.3f;
        attackCooldowns[attackIndex] = (GetStateDuration(state) + 2.0f) * cooldownMultiplier;

        stateLockTimer = GetStateDuration(state);
        currentFrame = 0;
        actionCooldown = 0.3f; // �ӳ���Ϊ��ȴ��ֹ����
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

////////////////////////����
////////////////////////����
////////////////////////����
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
    // ������仹δ��������������ҵ���ײ
    if (!isOpen) {
        // ����ȫ�ֽ�ɫ�б��������Ƿ���ײ
        for (Character* c : Character::allCharacters) {
            if (c->IsPlayer()) {
                // ��AABB��ײ���
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
    // ����������Ҫ�����߼�
}

//////������
TriggerObject::TriggerObject(float x, float y, float w, float h)
    : GameObject(x, y, w, h)
{
    // ��ʼ��ʱδ����
    hasTriggered = false;
    se->Play(false);
}


void TriggerObject::OnTrigger(Character* c) {
    // Ĭ����Ϊ�����������Ϣ�����������д����������
}

void TriggerObject::Update(float deltaTime) {
    // ����ȫ�ֽ�ɫ�б������ײ
    for (Character* c : Character::allCharacters) {
        // ����ֻ����Ҵ���������չ֧����������
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
                    hasTriggered = true; // ֻ����һ�Σ��������Ҫ����
                }
                break;
            }
        }
    } 

    if (hasTriggered)
        markForDeletion = true;
    UpdateAnimation(deltaTime);
}
