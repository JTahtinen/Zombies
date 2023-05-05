#include <jadel.h>
#include <thread>
#include "timer.h"

#define MATH_PI (3.14159265)

#define TO_RADIANS(deg) (deg * (MATH_PI / 180.0f))
#define TO_DEGREES(rad) (rad * (180.0f / MATH_PI))
static float frameTime;

static int windowWidth = 1280;
static int windowHeight = 720;

float aspectWidth = 16.0f;
float aspectHeight = 9.0f;

#define MAX_BULLETS (50)

static jadel::Window window;
static jadel::Surface winSurface;

static jadel::Mat3 viewMatrix(1.0f / aspectWidth, 0.0f, 0.0f,
                              0.0f, 1.0f / aspectHeight, 0.0f,
                              0.0f, 0.0f, 1.0f);

static jadel::Mat3 screenToWorldMatrix(aspectWidth, 0.0f, 0.0f,
                                       0.0f, aspectHeight, 0.0f,
                                       0.0f, 0.0f, 1.0f);

// GAME SETTINGS

static Timer fireTimer;
static bool canFire = true;

static bool aiOn = false;

static jadel::Vec2 linePointA(-2.f, -2.f);
static jadel::Vec2 linePointB(-2.f, 2.f);
static jadel::Vec2 linePointC(2.f, 2.f);
static jadel::Vec2 linePointD(2.f, -2.f);

static float angleOfRect = 0;
struct Entity
{
    jadel::Vec2 pos;
    jadel::Vec2 dir;
    bool alive;
};

struct Bullet
{
    Entity entity;
};

struct WorldBullet
{
    Bullet bulletLastFrame;
    Bullet bulletCurrentFrame;
    bool fired;
    bool inMotion;
    Timer aliveTimer;
    size_t lastShotOrder;
};

static float entityRadius = 0.3f;

static int worldWidth = 9;
static int worldHeight = 5;

static Entity player;

static WorldBullet bullets[MAX_BULLETS];

static Entity enemies[10];

static float cameraX = 0;
static float cameraY = 0;

static float normalSpeed = 0.6f;
static float fastSpeed = 1.2f;
static float currentSpeed = normalSpeed;

static size_t lastShot = 0;

int getRandom(int min, int max)
{
    int result = rand() % (max * 2) + min;
    return result;
}

void initEntity(Entity *entity, jadel::Vec2 pos)
{
    if (!entity)
    {
        return;
    }
    entity->pos = pos;
    entity->dir = jadel::Vec2(0, 0);
    entity->alive = true;
}

jadel::Mat3 getRotationMatrix(float angleInDegrees)
{
    jadel::Mat3 result =
        {
            cos(angleInDegrees), -sin(angleInDegrees), 0,
            sin(angleInDegrees), cos(angleInDegrees), 0,
            0, 0, 1};
    return result;
}

bool init();
void tick();

void renderRect(float x, float y, float width, float height, jadel::Color color)
{
    jadel::Vec2 start = viewMatrix.mul(jadel::Vec2(x, y));
    jadel::Vec2 end = viewMatrix.mul(jadel::Vec2(x + width, y + width));
    jadel::graphicsDrawRectRelative({start.x, start.y, end.x, end.y}, color);
}

void drawLine(jadel::Vec2 point0, jadel::Vec2 point1)
{
    jadel::Vec2 screenPoint0 = viewMatrix.mul(point0);
    jadel::Vec2 screenPoint1 = viewMatrix.mul(point1);
    renderRect(point0.x - 0.1f, point0.y - 0.1f, 0.2f, 0.2f, {1, 1, 1, 0.2});
    renderRect(point1.x - 0.1f, point1.y - 0.1f, 0.2f, 0.2f, {1, 1, 1, 0.2});
    jadel::graphicsDrawLineRelative(screenPoint0, screenPoint1, {1, 0, 1, 0.2});
}

void drawTriangle(jadel::Vec2 point0, jadel::Vec2 point1, jadel::Vec2 point2)
{
    jadel::Vec2 screenPoint0 = viewMatrix.mul(point0);
    jadel::Vec2 screenPoint1 = viewMatrix.mul(point1);
    jadel::Vec2 screenPoint2 = viewMatrix.mul(point2);
    graphicsDrawTriangleRelative(screenPoint0, screenPoint1, screenPoint2, {1, 0, 1, 0});
 
}

void initWorldBullet(WorldBullet *worldBullet)
{
    Entity *currentBulletEntity = &worldBullet->bulletCurrentFrame.entity;
    Entity *lastBulletEntity = &worldBullet->bulletLastFrame.entity;

    initEntity(currentBulletEntity, jadel::Vec2(0, 0));
    initEntity(lastBulletEntity, jadel::Vec2(0, 0));
    worldBullet->inMotion = false;
    worldBullet->fired = false;
    worldBullet->lastShotOrder = false;
}

int JadelMain()
{
    if (!JadelInit(MB(500)))
    {
        jadel::message("Jadel init failed!\n");
        return 0;
    }
    jadel::allocateConsole();
    srand(time(NULL));
    uint32 *winPixels = (uint32 *)winSurface.pixels;

    init();
    Timer frameTimer;
    frameTimer.start();
    uint32 elapsedInMillis = 0;
    uint32 minFrameTime = 1000 / 60;
    while (true)
    {
        JadelUpdate();
        tick();
        jadel::windowUpdate(&window, &winSurface);

        elapsedInMillis = frameTimer.getMillisSinceLastUpdate();
        if (elapsedInMillis < minFrameTime)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(minFrameTime - elapsedInMillis));
        }
        if (elapsedInMillis > 0)
        {
            frameTime = (float)frameTimer.getMillisSinceLastUpdate() * 0.001f;
        }

        // uint32 debugTime = frameTimer.getMillisSinceLastUpdate();
        //        jadel::message("%f\n", frameTime);

        frameTimer.update();
    }
    return 0;
}

bool init()
{
    jadel::windowCreate(&window, "Zombies", windowWidth, windowHeight);
    jadel::graphicsCreateSurface(windowWidth, windowHeight, &winSurface);
    jadel::graphicsPushTargetSurface(&winSurface);
    initEntity(&player, jadel::Vec2(0, 0));

    for (int i = 0; i < 10; ++i)
    {
        // float x = (float)(rand() % 32 - 16);
        // float y = (float)(rand() % 18 - 9);
        float x = getRandom(-16, 16);
        float y = getRandom(-9, 9);
        initEntity(&enemies[i], jadel::Vec2(x, y));
    }

    for (int i = 0; i < MAX_BULLETS; ++i)
    {
        initWorldBullet(&bullets[i]);
    }
    return true;
}

void drawEntities()
{
    renderRect(player.pos.x - entityRadius, player.pos.y - entityRadius, entityRadius * 2.0f, entityRadius * 2.0f, {1, 0, 1, 0});

    for (int i = 0; i < 10; ++i)
    {
        Entity enemy = enemies[i];
        if (enemy.alive)
        {
            renderRect(enemy.pos.x - entityRadius, enemy.pos.y - entityRadius, entityRadius * 2.0f, entityRadius * 2.0f, {1, 1, 0, 0.3});
        }
    }
    for (int i = 0; i < MAX_BULLETS; ++i)
    {
        if (bullets[i].inMotion)
        {
            renderRect(bullets[i].bulletCurrentFrame.entity.pos.x - entityRadius, bullets[i].bulletCurrentFrame.entity.pos.y - entityRadius, entityRadius * 2.0f, entityRadius * 2.0f, {1, 1, 1, 0});
        }
    }
}

void tick()
{
    jadel::Vec2 mouseRel = screenToWorldMatrix.mul(jadel::inputGetMouseRelative());
    player.dir = (mouseRel - player.pos).normalize();
    jadel::graphicsClearTargetSurface();


    if (jadel::inputIsKeyPressed(jadel::KEY_C))
    {
        angleOfRect -= 2.0f * frameTime;
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_V))
    {
        angleOfRect += 2.0f * frameTime;
    }
    if (angleOfRect > 360) angleOfRect = (float)((int)angleOfRect % 360);
    jadel::Mat3 rectRotation = getRotationMatrix(angleOfRect);
    jadel::Vec2 rotatedPointA = rectRotation.mul(linePointA);
    jadel::Vec2 rotatedPointB = rectRotation.mul(linePointB);
    jadel::Vec2 rotatedPointC = rectRotation.mul(linePointC);
    jadel::Vec2 rotatedPointD = rectRotation.mul(linePointD);

    if (jadel::inputIsKeyTyped(jadel::KEY_ESCAPE))
    {
        exit(0);
    }
    if (jadel::inputIsKeyTyped(jadel::KEY_SHIFT))
    {
        currentSpeed = fastSpeed;
    }
    else if (jadel::inputIsKeyReleased(jadel::KEY_SHIFT))
    {
        currentSpeed = normalSpeed;
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_W))
    {
        player.pos.y += currentSpeed * frameTime;
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_S))
    {
        player.pos.y -= currentSpeed * frameTime;
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_D))
    {
        player.pos.x += currentSpeed * frameTime;
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_A))
    {
        player.pos.x -= currentSpeed * frameTime;
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_UP))
    {
        linePointA.y += currentSpeed * frameTime;
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_DOWN))
    {
        linePointA.y -= currentSpeed * frameTime;
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_RIGHT))
    {
        linePointA.x += currentSpeed * frameTime;
    }
    if (jadel::inputIsKeyPressed(jadel::KEY_LEFT))
    {
        linePointA.x -= currentSpeed * frameTime;
    }
    if (jadel::inputIsKeyTyped(jadel::KEY_P))
    {
        aiOn = !aiOn;
    }

    if (aiOn)
    {
        for (int i = 0; i < 10; ++i)
        {
            Entity *enemy = &enemies[i];

            if (!enemy->alive)
                continue;

            jadel::Vec2 dirToPlayer = (player.pos - enemy->pos).normalize();
            enemy->pos += dirToPlayer * (0.5f * frameTime);
        }
    }
    if (jadel::inputIsMouseLeftHeld())
    {
        if (canFire)
        {
            fireTimer.start();
            canFire = false;
            int lowestBulletOrderIndex = 0;
            for (int i = 0; i < MAX_BULLETS; ++i)
            {
                if (!bullets[i].inMotion)
                {
                    bullets[i].lastShotOrder = lastShot++;
                    bullets[i].fired = true;
                    bullets[i].inMotion = true;
                    break;
                }
                else
                {
                    if (bullets[i].lastShotOrder < bullets[lowestBulletOrderIndex].lastShotOrder)
                    {
                        lowestBulletOrderIndex = i;
                    }
                }
                if (i == MAX_BULLETS - 1)
                {
                    bullets[lowestBulletOrderIndex].lastShotOrder = lastShot++;
                    bullets[lowestBulletOrderIndex].fired = true;
                    bullets[lowestBulletOrderIndex].inMotion = true;
                }
            }
        }
        else
        {
            fireTimer.update();
            if (fireTimer.getElapsedInMillis() >= 300)
            {
                canFire = true;
            }
        }
    }

    for (int i = 0; i < MAX_BULLETS; ++i)
    {
        WorldBullet *bullet = &bullets[i];

        if (bullet->fired)
        {
            bullet->bulletCurrentFrame.entity.pos = player.pos;
            // bullet->bulletCurrentFrame.entity.dir = (mouseRel - bullet->bulletCurrentFrame.entity.pos).normalize();
            bullet->bulletCurrentFrame.entity.dir = player.dir;
            bullet->aliveTimer.start();
            bullets[i].fired = false;
        }
        else
        {
            // Collision check every frame except for when the bullet is fired

            static float entityRadius = 0.4f;
            for (int e = 0; e < 10; ++e)
            {
                Entity *enemy = &enemies[e];
                if (!enemy->alive)
                    continue;
                jadel::Mat3 enemyRotation = getRotationMatrix(atan2(enemy->pos.y, enemy->pos.x));
                jadel::Vec2 rotatedEnemy = enemyRotation.mul(enemy->pos);

                float angle = TO_DEGREES(atan2(player.pos.y, player.pos.x));
                if (angle < 0)
                {
                    angle += 360.0f;
                }
                jadel::Vec2 rotatedLastFrameBullet = enemyRotation.mul(bullets[i].bulletLastFrame.entity.pos);
                jadel::Vec2 rotatedCurrentFrameBullet = enemyRotation.mul(bullets[i].bulletCurrentFrame.entity.pos);
                bool yCollision = rotatedCurrentFrameBullet.y >= -entityRadius && rotatedCurrentFrameBullet.y <= entityRadius;
                bool xCollision = rotatedLastFrameBullet.x <= rotatedEnemy.x - entityRadius && rotatedCurrentFrameBullet.x >= rotatedEnemy.x - entityRadius;
                if (xCollision && yCollision)
                {
                    static unsigned int collisionCounter = 0;
                    jadel::message("COLLISION! %d\n", collisionCounter++);
                    enemy->alive = false;
                    bullet->inMotion = false;
                }
            }
        }

        if (bullet->inMotion)
        {
            bullets[i].bulletLastFrame = bullets[i].bulletCurrentFrame;
            bullet->aliveTimer.update();
            if (bullet->aliveTimer.getElapsedInMillis() > 1500)
            {
                bullet->inMotion = false;
            }
            else
            {
                bullet->bulletCurrentFrame.entity.pos += bullet->bulletCurrentFrame.entity.dir * 0.9f;
            }
        }
    }
    for (int y = 0; y < worldHeight; ++y)
    {
        for (int x = 0; x < worldWidth; ++x)
        {
            jadel::Color tileColor = {1, (float)(x % 2 == 1), 0, (float)(x % 2 == 0)};
            renderRect(x * 0.6f, y * 0.6f, 0.6f, 0.6f, tileColor);
        }
 //       drawLine(rotatedPointA, rotatedPointB);
 //       drawLine(rotatedPointB, rotatedPointC);
 //       drawLine(rotatedPointC, rotatedPointD);
 //       drawLine(rotatedPointD, rotatedPointA);
        drawTriangle(rotatedPointA, rotatedPointB, rotatedPointC);
        drawTriangle(rotatedPointC, rotatedPointD, rotatedPointA);
    }

    drawEntities();
}