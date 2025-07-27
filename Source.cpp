#include <iostream>
#include <cmath>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include<fstream>

using namespace sf;
using namespace std;

int screen_x = 1200;
int screen_y = 900;


float cameraOffset = 0;
class Player {
protected:
    float player_x, player_y;

    int healthPoints;
    Texture texture;
    Texture jumpTexture;
    bool isInAir = false;
    const Texture* currentTexture = nullptr;
    Clock airTimeClock;
    float airTimeThreshold = 0.1f; // chnages to jump sprite after 0.1 sec from ground
    const int cell_size = 64;

    Sprite sprite;
    float speed;
    float velocityY = 0;

    Texture carryTexture;
    const Texture* originalTexture = nullptr;
    bool isBeingCarried = false;
    bool isCarrying = false;
    float velocityX = 0;
    Clock invincibilityClock;


    bool onGround = false;
    Clock aiDamageClock;
    bool showDamage = false;


public:

    bool isBoosted = false;
    Clock boostClock;
    float originalSpeed;
    bool isInvincible = false;
    static int sharedHealth;

    Player(float x, float y) : player_x(x), player_y(y), healthPoints(3) {}
    virtual void loadTexture(const string& file) {

        if (!texture.loadFromFile(file)) cout << "Failed to load texture: " << file << endl;
        sprite.setTexture(texture);

        sprite.setTexture(texture);
        currentTexture = &texture;

    }
    bool isActive = true;

    virtual void draw(RenderWindow& window) {
        if (!isActive) return;

        // computer controlled players animate dmagae by flickering but no hp lost
        if (showDamage) {
            if (aiDamageClock.getElapsedTime().asSeconds() < 0.2f) {
                sprite.setColor(Color(255, 255, 255, 150));
            }
            else {
                sprite.setColor(Color::White);  // back to normal
                showDamage = false;
            }
        }
        else {
            sprite.setColor(Color::White);
        }

        sprite.setPosition(player_x - cameraOffset, player_y);
        window.draw(sprite);
    }

    Sprite& getSprite() { return sprite; }

    void setJumpTexture(const string& file) {
        if (!jumpTexture.loadFromFile(file)) {
            cout << "Failed to load jump texture: " << file << endl;
        }
    }
    void setX(float x) {
        player_x = x;
    }
    void setY(float y) {
        player_y = y;
    }

    float& getX() {
        return player_x;
    }
    float& getY() {
        return player_y;
    }
    int getHealth() const {
        return sharedHealth;
    }

    // 1 sec invincibility
    void reduceHealth() {
        if (!isInvincible && sharedHealth > 0) {
            sharedHealth--;
            isInvincible = true;
            invincibilityClock.restart();
        }
    }

    bool checkInvincibilityTimeout() {
        if (isInvincible && invincibilityClock.getElapsedTime().asSeconds() > 1.0f) {
            isInvincible = false;
            return true;
        }
        return false;
    }

    bool getInvincibleStatus() const {
        return isInvincible;
    }
    bool getActiveStatus() const {
        return isActive;
    }
    void deactivate() {
        isActive = false;
    }
    // animation of damage of coputer controlled start 
    void triggerDamageFlash() {
        showDamage = true;
        aiDamageClock.restart();
    }

    void applyGravity(char** lvl, int& hit_box_factor_x, int& hit_box_factor_y, int Pheight, int Pwidth, float gravity, float terminalVelocity) {
        float offset_y = player_y + velocityY;

        // Bottom collision detection
        int row = (int)(offset_y + hit_box_factor_y + Pheight) / cell_size;
        int colLeft = (int)(player_x + hit_box_factor_x) / cell_size;
        int colMid = (int)(player_x + hit_box_factor_x + Pwidth / 2) / cell_size;
        int colRight = (int)(player_x + hit_box_factor_x + Pwidth) / cell_size;

        char bL = lvl[row][colLeft];
        char bM = lvl[row][colMid];
        char bR = lvl[row][colRight];

        bool isSolidBelow = (bL == 'w' || bM == 'w' || bR == 'w' || bL == 'p' || bM == 'p' || bR == 'p' || bL == 'b' || bM == 'b' || bR == 'b'); // treating breakable walls as solid


        // top collisions.. player can jump upward from platfroms 
        int topRow = (int)(offset_y + hit_box_factor_y) / cell_size;
        char tL = lvl[topRow][colLeft];
        char tM = lvl[topRow][colMid];
        char tR = lvl[topRow][colRight];
        bool isSolidAbove = (tL == 'w' || tM == 'w' || tR == 'w');


        if (velocityY > 0) {  // if falling down
            if (isSolidBelow) {
                velocityY = 0;
                onGround = true;
                player_y = row * cell_size - hit_box_factor_y - Pheight;  // if ground detected the stop there
            }
            else {
                player_y = offset_y;
                onGround = false;  //else continue movemnet till ground
            }
        }
        else if (velocityY < 0) {  // if jumping
            if (isSolidAbove) {   // and solid( platform or ground) detected then stop
                velocityY = 0;
            }
            else {
                player_y = offset_y;
            }
            onGround = false;
        }
        else {                 // else keep jumping 
            player_y = offset_y;
            onGround = false;
        }

        // gravity
        if (!onGround) {   // if player not on ground then increase falling sped
            velocityY += gravity;
            if (velocityY > terminalVelocity) velocityY = terminalVelocity;

        }

        if (abs(velocityY) < 0.01f)
            velocityY = 0;

        if (!isBeingCarried && !isCarrying) {
            if (!onGround || !isInAir) {
                sprite.setTexture(jumpTexture);   // if not on ground and not flying then chnage to jump sprite
                isInAir = true;
            }
            else if (onGround || isInAir) {
                sprite.setTexture(texture);
                isInAir = false;    // normal idle standing sprite
            }
        }

        // checking wall collisions
        int midRow = (int)(player_y + hit_box_factor_y + Pheight / 2) / cell_size;
        int leftTile = (int)(player_x + hit_box_factor_x - 1) / cell_size;
        int rightTile = (int)(player_x + hit_box_factor_x + Pwidth + 1) / cell_size;

        bool leftSolid = (lvl[midRow][leftTile] == 'w' || lvl[midRow][leftTile] == 'b');
        bool rightSolid = (lvl[midRow][rightTile] == 'w' || lvl[midRow][rightTile] == 'b');


        if (Keyboard::isKeyPressed(Keyboard::Left) && leftSolid) {
            player_x = (leftTile + 1) * cell_size - hit_box_factor_x;
        }

        if (Keyboard::isKeyPressed(Keyboard::Right) && rightSolid) {
            player_x = rightTile * cell_size - hit_box_factor_x - Pwidth;
        }
    }

    virtual void move(float jumpStrength, float acceleration, float friction) = 0;  // pure virtual function because each player has its own differnt movemnet so no payer object can be made

    float getSpeed() const {
        return speed;
    }
    void setSpeed(float newSpeed) {
        speed = newSpeed;
    }
    void setCarryTexture(const string& file) {
        if (!carryTexture.loadFromFile(file)) {
            cout << "Failed to load carry texture: " << file << endl;
        }
    }

    void switchToCarrySprite() {
        sprite.setTexture(carryTexture);
        currentTexture = &carryTexture;
    }

    void switchToIdleSprite() {
        sprite.setTexture(texture);
        currentTexture = &texture;
    }

    void setCarried(bool carried) {
        isBeingCarried = carried;
    }
    bool getCarried() const {
        return isBeingCarried;
    }

    virtual ~Player() {}
};
int Player::sharedHealth = 3;


class Sonic : public Player {
public:
    Sonic(float x, float y) : Player(x, y) {
        speed = 10.0f;
        loadTexture("Data/0right_still.png");
        setJumpTexture("Data/sonic-jump.png");
        setCarryTexture("Data/sonic-fly.png");
        sprite.setScale(2.5f, 2.5f);
    }

    void move(float jumpStrength, float acceleration, float friction) override {  // function is over ridden because it exits in parent class also
        if (!isActive) return;

        bool moving = false;

        if (Keyboard::isKeyPressed(Keyboard::Right)) {   // controlling movemnet with arrows
            velocityX += acceleration;
            moving = true;
        }
        if (Keyboard::isKeyPressed(Keyboard::Left)) {
            velocityX -= acceleration;
            moving = true;
        }

        if (!moving) {   //if not moving then applying friction 
            if (velocityX > 0) velocityX -= friction;
            else if (velocityX < 0) velocityX += friction;

            if (abs(velocityX) < 0.1f) velocityX = 0;
        }

        if (velocityX > speed) velocityX = speed;
        if (velocityX < -speed) velocityX = -speed;

        player_x += velocityX;

        if (Keyboard::isKeyPressed(Keyboard::Up) && onGround) {
            velocityY = jumpStrength;   // passed from each level that varies 
            onGround = false;
        }
    }
};


class Tails : public Player {

private:
    bool isFlying = false;
    Clock flyClock;
    float flyDuration = 7.0f;
    Texture flyTexture;
    Player* selectedSonic = nullptr;
    Player* selectedKnuckles = nullptr;

public:
    Tails(float x, float y) : Player(x, y) {
        speed = 6.0f;
        loadTexture("Data/tails.png");
        setJumpTexture("Data/tails-jump.png");
        setFlyTexture("Data/tails-fly.png");
        sprite.setScale(2.5f, 2.5f);
    }

    void setFlyTexture(const string& file) {
        if (!flyTexture.loadFromFile(file)) {
            cout << "Failed to load fly texture: " << file << endl;
        }
    }
    void setCarriedCharacters(Player* sonic, Player* knuckles) {
        selectedSonic = sonic;
        selectedKnuckles = knuckles;
    }
    void increaseFlyDuration(float extraTime) {
        flyDuration += extraTime;
    }

    void move(float jumpStrength, float acceleration, float friction) override {
        if (!isActive)
            return;

        bool moving = false;

        if (Keyboard::isKeyPressed(Keyboard::Right)) {
            velocityX += acceleration;
            moving = true;
        }
        if (Keyboard::isKeyPressed(Keyboard::Left)) {
            velocityX -= acceleration;
            moving = true;
        }

        if (!moving) {
            if (velocityX > 0) velocityX -= friction;
            else if (velocityX < 0) velocityX += friction;

            if (abs(velocityX) < 0.1f) velocityX = 0;
        }

        if (velocityX > speed) velocityX = speed;
        if (velocityX < -speed) velocityX = -speed;

        player_x += velocityX;

        if (Keyboard::isKeyPressed(Keyboard::Up) && onGround && !isFlying) {
            velocityY = jumpStrength;
            onGround = false;
        }

        if (Keyboard::isKeyPressed(Keyboard::F) && !isFlying) {  // tails flying "F"
            isFlying = true;
            isCarrying = true;
            flyClock.restart();
            sprite.setTexture(flyTexture);
        }

        // flying logic of tails 
        if (isFlying) {
            if (flyClock.getElapsedTime().asSeconds() <= flyDuration) {
                velocityY = -5.0f;
                onGround = false;

                if (selectedKnuckles && selectedKnuckles->getActiveStatus()) {   // taking kuckles while flying
                    selectedKnuckles->setX(player_x);
                    selectedKnuckles->setY(player_y + 70);
                    selectedKnuckles->switchToCarrySprite();
                    selectedKnuckles->setCarried(true);
                }

                if (selectedSonic && selectedSonic->getActiveStatus()) {   // taking sonic while flying
                    selectedSonic->setX(player_x);
                    selectedSonic->setY(player_y + 140);
                    selectedSonic->switchToCarrySprite();
                    selectedSonic->setCarried(true);
                }
            }
            else {
                isFlying = false;
                isCarrying = false;
                sprite.setTexture(texture);

                if (selectedKnuckles && selectedKnuckles->getActiveStatus()) {
                    selectedKnuckles->switchToIdleSprite();
                    selectedKnuckles->setCarried(false);
                }

                if (selectedSonic && selectedSonic->getActiveStatus()) {
                    selectedSonic->switchToIdleSprite();
                    selectedSonic->setCarried(false);
                }
            }
        }
    }
};

class Knuckles : public Player {

private:
    Texture punchTexture;
    bool isPunching = false;
    Clock punchClock;
    float punchDuration = 0.3f;


public:
    Knuckles(float x, float y) : Player(x, y) {
        speed = 5.0f;
        loadTexture("Data/idle-knuckle.png");
        setJumpTexture("Data/kuncles-jump.png");
        setCarryTexture("Data/knuckles-fly.png");
        punchTexture.loadFromFile("Data/Punch-knuckle.png");
        sprite.setScale(2.5f, 2.5f);
    }
    void move(float jumpStrength, float acceleration, float friction) override {
        if (!isActive) return;

        bool moving = false;

        if (Keyboard::isKeyPressed(Keyboard::Right)) {
            velocityX += acceleration;
            moving = true;
        }
        if (Keyboard::isKeyPressed(Keyboard::Left)) {
            velocityX -= acceleration;
            moving = true;
        }

        if (!moving) {
            if (velocityX > 0) velocityX -= friction;
            else if (velocityX < 0) velocityX += friction;

            if (abs(velocityX) < 0.1f) velocityX = 0;
        }

        if (velocityX > speed) velocityX = speed;
        if (velocityX < -speed) velocityX = -speed;

        player_x += velocityX;

        if (Keyboard::isKeyPressed(Keyboard::Up) && onGround) {
            velocityY = jumpStrength;
            onGround = false;
        }
    }


    void punch(char** lvl) {
        if (!isPunching && Keyboard::isKeyPressed(Keyboard::Space)) {   // can beak walls b with space 
            isPunching = true;
            punchClock.restart();
            sprite.setTexture(punchTexture);

            // Knuckles position + offset based on facing direction
            float punchReachX = player_x + 50; // punchces on right
            float punchReachY = player_y + 25; // around mid height

            int tileCol = (int)(punchReachX) / cell_size;
            int tileRow = (int)(punchReachY) / cell_size;

            if (lvl[tileRow][tileCol] == 'b') {
                lvl[tileRow][tileCol] = '\0'; // break it
            }
        }

        if (isPunching && punchClock.getElapsedTime().asSeconds() > punchDuration) {
            sprite.setTexture(texture);  // punching sprite 
            isPunching = false;
        }
    }
};
bool isPlayerNearBreakableWall(Player& p, char** lvl) {    // if knuckles is near the breakable wall its gets selected on it own and break the wall b
    const int cell_size = 64;
    int tileX = (int)(p.getX()) / cell_size;
    int tileY = (int)(p.getY()) / cell_size;

    for (int i = tileY - 1; i <= tileY + 1; i++) {
        for (int j = tileX - 1; j <= tileX + 1; j++) {
            if (i >= 0 && i < 14 && j >= 0 && j < 300 && lvl[i][j] == 'b') {   // finding te location of breable wall
                return true;
            }
        }
    }
    return false;
}


class Obstacle {

public:
    virtual void draw(RenderWindow& window, float cameraOffset) = 0;
    virtual bool checkCollision(float px, float py, int pwidth, int pheight) = 0;
    virtual ~Obstacle() {}
};

class Spike : public Obstacle {
private:
    Sprite sprite;
    float x, y;
public:
    Spike(Texture& texture, float posX, float posY) : x(posX), y(posY) {
        sprite.setTexture(texture);
        sprite.setScale(1.5f, 1.5f);
        sprite.setPosition(x, y);
    }

    void draw(RenderWindow& window, float cameraOffset) override {
        sprite.setPosition(x - cameraOffset, y);
        window.draw(sprite);
    }

    bool checkCollision(float px, float py, int pwidth, int pheight) override {
        FloatRect spikeBounds(x, y, sprite.getGlobalBounds().width, sprite.getGlobalBounds().height);
        FloatRect playerBottom(px, py + pheight - 5, pwidth, 5);
        return spikeBounds.intersects(playerBottom);
    }
};

class Pit : public Obstacle {
private:
    FloatRect pitArea;
public:
    Pit(float x, float y, float width, float height) {
        pitArea = FloatRect(x, y, width, height);
    }

    void draw(RenderWindow& window, float) override {}

    bool checkCollision(float px, float py, int pwidth, int pheight) override {
        FloatRect player(px, py, pwidth, pheight);
        return pitArea.intersects(player);
    }
};

class Enemy {
protected:
    Sprite sprite;
    Texture texture;
    float x, y;
    int hp;
    bool alive = true;
    FloatRect activeSection;
    const int cell_size = 64;


public:
    virtual void load(const string& file, float px, float py, int health, FloatRect section) {
        texture.loadFromFile(file);
        sprite.setTexture(texture);
        sprite.setScale(2.0f, 2.0f);
        x = px;
        y = py;
        hp = health;
        activeSection = section;
        sprite.setPosition(x, y);
    }

    virtual void update(float deltaTime, FloatRect playerBounds, bool isPlayerJumpingOrRolling, Player* player) = 0;

    virtual void draw(RenderWindow& window, float cameraOffset, float playerX) {
        if (alive && playerX >= activeSection.left && playerX <= activeSection.left + activeSection.width) {
            sprite.setPosition(x - cameraOffset, y);
            window.draw(sprite);
        }
    }



    virtual bool checkCollision(FloatRect playerBounds, bool isPlayerJumpingOrRolling, Player* player) {
        if (!alive || !sprite.getGlobalBounds().intersects(playerBounds))
            return false;

        if (isPlayerJumpingOrRolling) {
            hp--;
            if (hp <= 0) alive = false;
            return true; // Enemy was damaged
        }
        else {
            // player takes damage
            static Clock hitCooldown;
            if (hitCooldown.getElapsedTime().asSeconds() > 2) {
                player->reduceHealth();
                hitCooldown.restart();
            }
        }

        return false;
    }

    virtual bool isAlive() const {
        return alive;
    }
    virtual FloatRect getBounds() const {
        return sprite.getGlobalBounds();
    }

    int getHP() const {
        return hp;
    }
    virtual ~Enemy() {}
};


// ========== FlyingEnemy Base ==========
class FlyingEnemy : public Enemy {

protected:
    float speedX = 0.5f;
    float direction = 1.0f; //1=right, -1left
public:

    void update(float deltaTime, FloatRect playerBounds, bool isPlayerJumpingOrRolling, Player* player) {
        x += direction * speedX;
        checkCollision(playerBounds, isPlayerJumpingOrRolling, player);
    }

};

// ========== CrawlingEnemy Base ==========
class CrawlingEnemy : public Enemy {
protected:
    float speedX = 0.2f;
    float direction = 1.0f;
public:
    void update(float deltaTime, FloatRect playerBounds, bool isPlayerJumpingOrRolling, Player* player) {
        x += direction * speedX;
        checkCollision(playerBounds, isPlayerJumpingOrRolling, player);
    }
};

class Projectile {
private:
    float x, y, speedX, speedY;
    Sprite sprite;
    Texture texture;
    bool active = true;

public:
    Projectile(float startX, float startY, float dirX, float dirY, const string& textureFile) {
        x = startX;
        y = startY;
        speedX = dirX;
        speedY = dirY;
        texture.loadFromFile(textureFile);
        sprite.setTexture(texture);
        sprite.setPosition(x, y);
        sprite.setScale(0.5f, 0.5f);
    }

    void update(float deltaTime) {
        x += speedX * deltaTime;
        y += speedY * deltaTime;
        sprite.setPosition(x, y);
    }

    bool isActive() const {
        return active;
    }

    void deactivate() {
        active = false;
    }

    FloatRect getBounds() const { return sprite.getGlobalBounds(); }

    void draw(RenderWindow& window) {
        if (active) window.draw(sprite);
    }
};


// ========== BatBrain ==========
class BatBrain : public FlyingEnemy {
public:
    void update(float deltaTime, FloatRect playerBounds, bool isPlayerJumpingOrRolling, Player* player) override {
        if (!alive) return;

        float targetX = playerBounds.left + playerBounds.width / 2;
        float targetY = playerBounds.top + playerBounds.height / 2;

        float dx = targetX - x;
        float dy = targetY - y;

        x += dx * 0.02f;
        y += dy * 0.02f;
        sprite.setPosition(x, y);
        checkCollision(playerBounds, isPlayerJumpingOrRolling, player);
    }
};

//  BeeBot 
class BeeBot : public FlyingEnemy {
private:
    float zigzagAmplitude = 20;
    float zigzagSpeed = 3.0f;
    float time = 0;
    Projectile* projectiles[5];  // max 5 active projectiles
    int projectileCount = 0;
    Clock shootClock;
    float shootCooldown = 2.0f;

public:
    BeeBot() {
        for (int i = 0; i < 5; i++)
            projectiles[i] = nullptr;
    }

    void update(float deltaTime, FloatRect playerBounds, bool isPlayerJumpingOrRolling, Player* player) override {
        if (!alive)
            return;

        // zigzag diagonal movement
        time += deltaTime;

        //  X direction
        x += direction * speedX;

        // zig-zag in Y with diagonal effect
        y += sin(time * zigzagSpeed) * zigzagAmplitude * deltaTime;


        if (x < activeSection.left || x > activeSection.left + activeSection.width)//y movment
            direction *= -1;

        // shooting every 5 seconds
        if (shootClock.getElapsedTime().asSeconds() > 5.0f) {
            for (int i = 0; i < 5; i++) {
                if (!projectiles[i] || !projectiles[i]->isActive()) {
                    delete projectiles[i];
                    projectiles[i] = new Projectile(x, y, direction * 200, 0, "Data/Projectile.png");
                    shootClock.restart();
                    break;
                }
            }
        }


        // update projectiles
        for (int i = 0; i < projectileCount; ++i) {
            if (projectiles[i] && projectiles[i]->isActive()) {
                projectiles[i]->update(deltaTime);
                if (projectiles[i]->getBounds().intersects(playerBounds)) {
                    if (!player->isInvincible)
                        player->reduceHealth();
                    projectiles[i]->deactivate();
                }
            }
        }

        checkCollision(playerBounds, isPlayerJumpingOrRolling, player);
    }
    void drawProjectiles(RenderWindow& window) {
        for (int i = 0; i < projectileCount; ++i)
            if (projectiles[i] && projectiles[i]->isActive())
                projectiles[i]->draw(window);
    }
};


class CrabMeat : public CrawlingEnemy {
    Projectile* projectiles[5];
    int projectileCount = 0;
    Clock shootClock;
    float shootCooldown = 2.0f;

public:
    CrabMeat() {
        for (int i = 0; i < 5; i++)
            projectiles[i] = nullptr;
    }
    void update(float deltaTime, FloatRect playerBounds, bool isPlayerJumpingOrRolling, Player* player) override {
        if (!alive)
            return;

        x += direction * speedX;
        if (x < activeSection.left || x > activeSection.left + activeSection.width)
            direction *= -1;

        // shooting projectles at intervalss
        if (shootClock.getElapsedTime().asSeconds() > 5.0f) {
            for (int i = 0; i < 5; i++) {
                if (!projectiles[i] || !projectiles[i]->isActive()) {
                    delete projectiles[i];  // cleanup old
                    projectiles[i] = new Projectile(x, y, direction * 200, 0, "Data/Projectile.png");
                    shootClock.restart();
                    break;
                }
            }
        }


        // update projectiles
        for (int i = 0; i < projectileCount; ++i) {
            if (projectiles[i] && projectiles[i]->isActive()) {
                projectiles[i]->update(deltaTime);
                if (projectiles[i]->getBounds().intersects(playerBounds)) {
                    if (!player->isInvincible)
                        player->reduceHealth();
                    projectiles[i]->deactivate();
                }
            }
        }

        checkCollision(playerBounds, isPlayerJumpingOrRolling, player);
    }

    void drawProjectiles(RenderWindow& window) {
        for (int i = 0; i < projectileCount; ++i)
            if (projectiles[i] && projectiles[i]->isActive())
                projectiles[i]->draw(window);
    }


};

class MotoBug : public CrawlingEnemy {
public:
    void update(float deltaTime, FloatRect playerBounds, bool isPlayerJumpingOrRolling, Player* player) override {
        if (!alive)
            return;

        float targetX = playerBounds.left + playerBounds.width / 2;
        float dx = targetX - x;

        x += dx * 0.01f; // follows slowly

        checkCollision(playerBounds, isPlayerJumpingOrRolling, player);
    }

};


// --- Collectibles ---
class Collectible {

protected:
    Sprite sprite;
    Texture texture;
    bool collected;
    float x, y;
    const int cell_size = 64;

public:
    Collectible() : collected(false), x(0), y(0) {}
    virtual void load(const string& file, float px, float py) {
        texture.loadFromFile(file);
        sprite.setTexture(texture);
        x = px;
        y = py;
        sprite.setPosition(x, y);
    }
    virtual void update(float deltaTime) = 0;
    virtual void draw(RenderWindow& window) {
        if (!collected) {
            sprite.setPosition(x - cameraOffset, y);
            window.draw(sprite);
        }
    }
    virtual bool checkCollision(FloatRect playerBounds) = 0;
    virtual bool isCollected() const {
        return collected;
    }
    virtual ~Collectible() {}
};


class Ring : public Collectible {
private:
    int frameWidth = 16;
    int frameCount = 5;
    float frameTime = 0.1f;
    int currentFrame = 0;
    float timer = 0;




public:
    Ring() {}
    void load(const string& file, float px, float py) override {
        Collectible::load(file, px, py);
        sprite.setTextureRect(IntRect(0, 0, frameWidth, 16));
        sprite.setScale(2.5f, 2.5f);



    }

    void update(float deltaTime) override {
        if (collected)
            return;
        timer += deltaTime;
        if (timer >= frameTime) {
            currentFrame = (currentFrame + 1) % frameCount;
            sprite.setTextureRect(IntRect(currentFrame * frameWidth, 0, frameWidth, 16));  // animating the ring ... rotation 360
            timer = 0;
        }
    }
    bool checkCollision(FloatRect playerBounds) override {
        if (!collected && sprite.getGlobalBounds().intersects(playerBounds)) {
            collected = true;

            return true;
        }
        return false;
    }
};


class ExtraLife : public Collectible {

public:
    ExtraLife() {}
    void load(const string& file, float px, float py) override {
        bool success = texture.loadFromFile(file);
        if (!success) {
            cout << "Failed to load extra life image: " << file << endl;
            return;
        }
        sprite.setTexture(texture);
        sprite.setScale(1.0f, 1.0f);
        x = px;
        y = py;
        sprite.setPosition(x, y);
    }

    void update(float deltaTime) override {}

    void draw(RenderWindow& window) override {
        if (!collected) {
            sprite.setPosition(x - cameraOffset, y);
            window.draw(sprite);
        }
    }

    bool checkCollision(FloatRect playerBounds) override {
        if (collected) return false;

        FloatRect itemBounds = sprite.getGlobalBounds();
        itemBounds.width *= 0.6f;
        itemBounds.height *= 0.6f;
        itemBounds.left += itemBounds.width * 0.2f;
        itemBounds.top += itemBounds.height * 0.2f;

        if (itemBounds.intersects(playerBounds)) {
            collected = true;
            if (Player::sharedHealth < 5)  // max 5 extra lives per level
                Player::sharedHealth++;
            return true;
        }
        return false;
    }
};


class SpecialBoost : public Collectible {
public:
    SpecialBoost() {}

    void load(const string& file, float px, float py) override {
        if (!texture.loadFromFile(file)) {
            cout << "Failed to load boost image: " << file << endl;
            return;
        }
        sprite.setTexture(texture);
        sprite.setScale(2.0f, 2.0f);
        x = px;
        y = py;
        sprite.setPosition(x, y);
    }

    void update(float deltaTime) override {}

    bool checkCollision(FloatRect playerBounds, Player* player) {
        if (collected) return false;

        FloatRect itemBounds = sprite.getGlobalBounds();
        itemBounds.width *= 0.6f;
        itemBounds.height *= 0.6f;
        itemBounds.left += itemBounds.width * 0.2f;
        itemBounds.top += itemBounds.height * 0.2f;

        if (itemBounds.intersects(playerBounds)) {
            collected = true;
            player->isBoosted = true;
            player->boostClock.restart();

            if (Sonic* sonic = dynamic_cast<Sonic*>(player)) {   // if sonic gets the boost then speed increase by 4
                player->originalSpeed = sonic->getSpeed();
                sonic->setSpeed(player->originalSpeed + 4.0f);
            }
            else if (Tails* tails = dynamic_cast<Tails*>(player)) {    // dynamic castig used because we dont know ehich player picks the boost ... so it points to that specifc player
                tails->increaseFlyDuration(4.0f);   // flying duration increases by 4 sec
            }
            else if (Knuckles* knuckles = dynamic_cast<Knuckles*>(player)) {
                player->isInvincible = true;   // becomes invicible means that it doesnt get affected by any obstacle or eniemy during that time 
            }
            return true;
        }
        return false;
    }

    bool checkCollision(FloatRect playerBounds) override {
        return false;
    }
};

//========================================================================================level1=====================================================================


class Level {
protected:

    // level class is using all the other classes (aggregation)
    Sonic sonic;
    Tails tails;
    Knuckles knuckles;
    Player* selectedPlayer;

    Ring rings[500];
    int ringCount = 0, collectedRings = 0;

    ExtraLife extraLives[8];
    int extraLifeCount = 0;

    SpecialBoost boosts[5];
    int boostCount = 0;
    Obstacle* obstacles[100];
    int obstacleCount = 0;



    Enemy* enemies[10];
    int enemyCount = 0;
    char** lvl;
    int height, width;

    int score = 0;

    Texture wallTex, platformTex, breakTex, bgTex;
    Sprite wallSprite, platformSprite, breakWallSprite, background;  // common textures in all levels

    float gravity;
    float acceleration;
    float friction;
    float jumpStrength;
    const int cell_size = 64;


public:
    Level(int h, int w) : height(h), width(w), sonic(100, 50), tails(50, 100), knuckles(150, 100) {
        selectedPlayer = &sonic;
        lvl = new char* [height];
        for (int i = 0; i < height; i++)
            lvl[i] = new char[width] {'\0'};

        tails.setCarriedCharacters(&sonic, &knuckles);
    }

    void display_level(RenderWindow& window, const int height, const int width,
        char** lvl, Sprite& wallSprite1, Sprite& platformSprite, Sprite& breakWallSprite, const int cell_size)
    {
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                if (lvl[i][j] == 'w') {
                    wallSprite1.setPosition(j * cell_size - cameraOffset, i * cell_size);
                    window.draw(wallSprite1);
                }
                else if (lvl[i][j] == 'p') {
                    platformSprite.setPosition(j * cell_size - cameraOffset, i * cell_size);
                    window.draw(platformSprite);
                }
                else if (lvl[i][j] == 'b') {
                    breakWallSprite.setPosition(j * cell_size - cameraOffset, i * cell_size);
                    window.draw(breakWallSprite);
                }

            }
        }
    }


    virtual void loadAssets() {};
    virtual void generateLayout() = 0;

    Player* getSelectedPlayer() {
        return selectedPlayer;
    }
    char** getLevelGrid() {
        return lvl;
    }

    Sonic& getSonic() {
        return sonic;
    }
    Tails& getTails() {
        return tails;
    }
    Knuckles& getKnuckles() {
        return knuckles;
    }

    Ring* getRings() {
        return rings;
    }
    int getRingCount() const {
        return ringCount;
    }
    int& getCollectedRings() {
        return collectedRings;
    }

    ExtraLife* getExtraLives() {
        return extraLives;
    }
    int getExtraLifeCount() const {
        return extraLifeCount;
    }
    SpecialBoost* getBoosts() {
        return boosts;
    }
    int getBoostCount() const {
        return boostCount;
    }

    Obstacle** getObstacles() {
        return obstacles;
    }
    int getObstacleCount() const {
        return obstacleCount;
    }
    Enemy** getEnemies() {
        return enemies;
    }
    int getEnemyCount() const {
        return enemyCount;
    }

    Sprite& getWallSprite() {
        return wallSprite;
    }
    Sprite& getPlatformSprite() {
        return platformSprite;
    }
    Sprite& getBreakWallSprite() {
        return breakWallSprite;
    }
    Sprite& getBackgroundSprite() {
        return background;
    }
    int getWidth() const {
        return width;
    }
    int getHeight() const {
        return height;
    }

    float getGravity() const {
        return gravity;
    }
    float getAcceleration() const {
        return acceleration;
    }
    float getFriction() const {
        return friction;
    }
    float getJumpStrength() const {
        return jumpStrength;
    }
    virtual ~Level() {
        for (int i = 0; i < height; i++) delete[] lvl[i];
        delete[] lvl;
        for (int i = 0; i < obstacleCount; i++) delete obstacles[i];
        for (int i = 0; i < enemyCount; i++) delete enemies[i];
    }
};


//===========================================================================================================================================================================

class Level1 : Level {

private:
    Clock deltaClock;  //tracks frame time
    Clock spikeCooldown;
    Font font;
    Text ringText, scoreText, livesText;

public:
    Level1() : Level(14, 200) {  // screen size of level 1
        gravity = 1.0f;
        acceleration = 0.5f;
        friction = 0.6f;
        jumpStrength = -20.0f;
        selectedPlayer = &sonic;
        tails.setCarriedCharacters(&sonic, &knuckles);
        initializeLevelGrid();
        generateLayout();
        setupHUD();
    }

    ~Level1() {
        for (int i = 0; i < height; i++) delete[] lvl[i];
        delete[] lvl;
        for (int i = 0; i < obstacleCount; i++) delete obstacles[i];
        for (int i = 0; i < enemyCount; i++) delete enemies[i];
    }

    void initializeLevelGrid() {
        lvl = new char* [height];
        for (int i = 0; i < height; i++)
            lvl[i] = new char[width] { '\0' };     // inillizing grid size
    }

    void setupHUD() {    //setting up the highscore 
        font.loadFromFile("Data/arial.ttf");
        ringText.setFont(font);
        ringText.setCharacterSize(24);
        ringText.setFillColor(Color::Yellow);
        ringText.setPosition(20, 30);

        scoreText.setFont(font);
        scoreText.setCharacterSize(24);
        scoreText.setFillColor(Color::White);
        scoreText.setPosition(20, 60);

        livesText.setFont(font);
        livesText.setCharacterSize(24);
        livesText.setFillColor(Color::Cyan);
        livesText.setPosition(20, 90);
    }

    void generateLayout() {
        for (int j = 0; j < width;) {
            bool makePit = rand() % 6 == 0;
            int groupSize = rand() % 5 + 3;
            for (int k = 0; k < groupSize && (j + k) < width; k++) {
                for (int i = 11; i < height; i++)
                    lvl[i][j + k] = makePit ? '\0' : 'w';
                if (makePit) lvl[7 + rand() % 2][j + k] = 'p';
            }
            j += groupSize;
        }

        for (int i = 0; i <= 1; i++)
            for (int j = 0; j < width; j++)
                lvl[i][j] = 'w';

        int placedBreakWalls = 0;
        while (placedBreakWalls < 30) {
            int row = rand() % (height - 3) + 3;   // adding ground, platforms and breakable walls in rndom places
            int col = rand() % (width - 5);
            if (lvl[row][col] == 'w') {
                lvl[row][col] = 'b';
                placedBreakWalls++;
            }
        }

        // placing rings on random places
        for (int i = 0; i < 5; i++)
            rings[ringCount++].load("Data/ring.png", 1100 + i * 30, 400);

        for (int i = 0; i < 3; i++)
            rings[ringCount++].load("Data/ring.png", 1400 + i * 30, 480);

        for (int i = 0; i < 4; i++)
            rings[ringCount++].load("Data/ring.png", 1600 + i * 30, 300);

        for (int i = 0; i < 5; i++)
            rings[ringCount++].load("Data/ring.png", 1900 + i * 30, 250);

        for (int i = 0; i < 4; i++)
            rings[ringCount++].load("Data/ring.png", 2100, 400 - i * 40);

        for (int x = 2200; x < 60000; x += 600) {
            int clusterSize = rand() % 3 + 3;
            int y = rand() % 200 + 200;
            for (int i = 0; i < clusterSize && ringCount < 500; i++)
                rings[ringCount++].load("Data/ring.png", x + i * 30, y);
            if (rand() % 2 == 0 && ringCount + 3 < 500)
                for (int i = 0; i < 3; i++)
                    rings[ringCount++].load("Data/ring.png", x + clusterSize * 30, y - i * 40);
        }


        for (int i = 0; i < 8; i++) {
            int col = 25 + i * 15; // space them apart horizontally
            int row = 7 + rand() % 2; // row 7 or 8

            float x = col * cell_size;
            float y = row * cell_size;

            extraLives[extraLifeCount++].load("Data/heart.png", x, y);
            if (boostCount < 5)
                boosts[boostCount++].load("Data/boost.png", x + 30, y - 50);

            // place breakable walls around heart and boost 
            if (row > 0 && lvl[row - 1][col] == '\0') lvl[row - 1][col] = 'b';
            if (row < height - 1 && lvl[row + 1][col] == '\0') lvl[row + 1][col] = 'b';
            if (col > 0 && lvl[row][col - 1] == '\0') lvl[row][col - 1] = 'b';
            if (col < width - 1 && lvl[row][col + 1] == '\0') lvl[row][col + 1] = 'b';

        }

        Texture spikeTex;
        spikeTex.loadFromFile("Data/spike.png");
        for (int col = 10; col < width - 10; col += rand() % 20 + 10) {
            if (lvl[10][col] == 'w') {
                float spikeX = col * cell_size;
                float spikeY = 10 * cell_size - 32;
                obstacles[obstacleCount++] = new Spike(spikeTex, spikeX, spikeY);
            }
        }

        BatBrain* bat = new BatBrain();
        bat->load("Data/batbrain.png", 800, 300, 3, FloatRect(0, 0, width * cell_size, screen_y));
        enemies[enemyCount++] = bat;

        BeeBot* bee = new BeeBot();
        bee->load("Data/beebot.png", 2500, 250, 5, FloatRect(2000, 0, 1000, screen_y));
        enemies[enemyCount++] = bee;

        CrabMeat* crab = new CrabMeat();
        crab->load("Data/crabmeat.png", 3500, 600, 4, FloatRect(3400, 0, 800, screen_y));
        enemies[enemyCount++] = crab;

        MotoBug* moto = new MotoBug();
        moto->load("Data/motobug.png", 3000, 600, 3, FloatRect(2900, 0, 1000, screen_y));
        enemies[enemyCount++] = moto;
    }

    int getScore() { return score; }

    void update(RenderWindow& window) {

        Music bgMusic;
        if (!bgMusic.openFromFile("Data/labrynth.ogg")) {
            cout << "Could not load background music" << endl;
        }
        bgMusic.setLoop(true);
        bgMusic.play();

        score = 0;

        const int MAX_BREAKABLE_WALLS = 30;
        int placedBreakWalls = 0;

        //  generating ground and pits pattern
        for (int j = 0; j < width;) {
            bool makePit = rand() % 6 == 0;
            int groupSize = rand() % 5 + 3; // 3 to 7 columns per group

            for (int k = 0; k < groupSize && (j + k) < width; k++) {
                for (int i = 11; i < height; i++) {
                    lvl[i][j + k] = makePit ? '\0' : 'w';  // pit or ground
                }

                if (makePit) {
                    int platformRow = 7 + rand() % 2; // platform in row 7 or 8
                    lvl[platformRow][j + k] = 'p';
                }
            }
            j += groupSize;
        }


        // fill top wall layers
        for (int i = 0; i <= 1; i++) {
            for (int j = 0; j < width; j++) lvl[i][j] = 'w';
        }

        // random ground block groups
        for (int j = 2; j < width - 3;) {
            int groupSize = rand() % 5 + 1;
            for (int k = 0; k < groupSize && (j + k) < width; k++)
                if (lvl[10][j + k] == '\0') lvl[10][j + k] = 'w';
            j += groupSize + (rand() % 4 + 2);
        }

        // top ceiling block groups
        for (int j = 3; j < width - 3;) {
            int groupSize = rand() % 5 + 1;
            for (int k = 0; k < groupSize && (j + k) < width; k++)
                if (lvl[2][j + k] == '\0') lvl[2][j + k] = 'w';
            j += groupSize + (rand() % 4 + 2);
        }

        // mid-air platforms
        for (int j = 5; j < width - 10;) {
            int clusterSize = rand() % 4 + 1;
            int row = rand() % 6 + 3;
            for (int k = 0; k < clusterSize && (j + k) < width; k++)
                lvl[row][j + k] = 'p';
            j += clusterSize + (rand() % 6 + 2);
        }

        // vertical platform like an elevator shaft
        for (int row = 5; row <= 9; row++)
            lvl[row][50] = 'p';

        for (int row = 6; row <= 11; row++)
            lvl[row][80] = 'p';

        // Tall vertical wall players must go around or over
        for (int row = 4; row <= 11; row++)
            lvl[row][60] = 'w';

        for (int row = 5; row <= 11; row++)
            lvl[row][90] = 'w';

        // Staggered floating platforms
        lvl[6][55] = 'p';
        lvl[4][58] = 'p';
        lvl[7][62] = 'p';
        lvl[5][65] = 'p';

        // Small high ledge
        lvl[3][70] = 'p';
        lvl[3][71] = 'p';

        // random breable walls
        while (placedBreakWalls < MAX_BREAKABLE_WALLS) {
            int row = rand() % (height - 3) + 3; // avoid top rows (row 0–2)
            int col = rand() % (width - 5);      // avoid edge
            if (lvl[row][col] == 'w') {
                lvl[row][col] = 'b';
                placedBreakWalls++;
            }
        }
        //  manually adding breakable wall column under stuck ledge
        lvl[2][41] = 'b';
        lvl[8][57] = 'b';
        lvl[9][57] = 'b';

        //  loading textures
        Texture wallTex, platformTex, bgTex, spikeTex, breakTex;
        wallTex.loadFromFile("Data/brick1.png");
        platformTex.loadFromFile("Data/brick2.png");
        bgTex.loadFromFile("Data/bg2.png");
        spikeTex.loadFromFile("Data/spike.png");
        Sprite wallSprite(wallTex), platformSprite(platformTex), background(bgTex);
        breakTex.loadFromFile("Data/Break.png");

        Sprite breakWallSprite(breakTex);

        //  randomly adding spikes on solid ground in row 11
        for (int col = 10; col < width - 10; col += rand() % 20 + 10) {
            if (lvl[10][col] == 'w') {
                float spikeX = col * cell_size;
                float spikeY = 10 * cell_size - 32; // row 10
                obstacles[obstacleCount++] = new Spike(spikeTex, spikeX, spikeY);
            }
        }

        for (int i = 0; i < enemyCount; ++i) {
            if (enemies[i]->isAlive()) {
                enemies[i]->draw(window, cameraOffset, selectedPlayer->getX());

                // draw projectiles 
                CrabMeat* crab = dynamic_cast<CrabMeat*>(enemies[i]);
                if (crab) crab->drawProjectiles(window);

                BeeBot* bee = dynamic_cast<BeeBot*>(enemies[i]);
                if (bee) bee->drawProjectiles(window);
            }
        }

        float velocityY = 0, gravity = 1, terminal_Velocity = 20, jumpStrength = -20;
        int raw_img_x = 24, raw_img_y = 35;
        float scale_x = 2.5, scale_y = 2.5;
        int Pheight = raw_img_y * scale_y, Pwidth = raw_img_x * scale_x;
        int hit_box_factor_x = 8 * scale_x, hit_box_factor_y = 5 * scale_y;
        bool onGround = false;
        float offset_y = 0;

        Event ev;
        while (window.isOpen()) {
            while (window.pollEvent(ev)) {
                if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::Z) {
                    if (selectedPlayer == &sonic) {
                        if (tails.getActiveStatus()) selectedPlayer = &tails;
                        else if (knuckles.getActiveStatus()) selectedPlayer = &knuckles;
                    }
                    else if (selectedPlayer == &tails) {
                        if (knuckles.getActiveStatus()) selectedPlayer = &knuckles;
                        else if (sonic.getActiveStatus()) selectedPlayer = &sonic;
                    }
                    else if (selectedPlayer == &knuckles) {
                        if (sonic.getActiveStatus()) selectedPlayer = &sonic;
                        else if (tails.getActiveStatus()) selectedPlayer = &tails;
                    }

                }
            }

            if (Keyboard::isKeyPressed(Keyboard::Escape)) window.close();

            sonic.move(jumpStrength, acceleration, friction);
            tails.move(jumpStrength, acceleration, friction);
            knuckles.move(jumpStrength, acceleration, friction);

            if (selectedPlayer == &knuckles) {
                knuckles.punch(lvl);
            }


            cameraOffset = selectedPlayer->getX() - screen_x / 2;
            if (cameraOffset < 0) cameraOffset = 0;

            float maxOffset = width * cell_size - screen_x;
            if (cameraOffset > maxOffset) cameraOffset = maxOffset;

            float spacing = 50.0f;  // Distance between characters

            // the computr controlled follow user controlled player
            //ai
            if (selectedPlayer != &sonic) {
                float targetX = selectedPlayer->getX() - spacing;
                if (abs(targetX - sonic.getX()) > 0.5f)
                    sonic.getX() += (targetX - sonic.getX()) * 0.1f;
            }

            if (selectedPlayer != &tails) {
                float targetX = selectedPlayer->getX() - spacing;
                if (abs(targetX - tails.getX()) > 0.5f)
                    tails.getX() += (targetX - tails.getX()) * 0.1f;
            }

            if (selectedPlayer != &knuckles) {
                float targetX = selectedPlayer->getX() - spacing;
                if (abs(targetX - knuckles.getX()) > 0.5f)
                    knuckles.getX() += (targetX - knuckles.getX()) * 0.1f;
            }

            sonic.applyGravity(lvl, hit_box_factor_x, hit_box_factor_y, Pheight, Pwidth, gravity, 20.0f);
            tails.applyGravity(lvl, hit_box_factor_x, hit_box_factor_y, Pheight, Pwidth, gravity, 20.0f);
            knuckles.applyGravity(lvl, hit_box_factor_x, hit_box_factor_y, Pheight, Pwidth, gravity, 20.0f);

            FloatRect playerBounds = selectedPlayer->getSprite().getGlobalBounds();
            bool isJumpingOrRolling = Keyboard::isKeyPressed(Keyboard::Up) || Keyboard::isKeyPressed(Keyboard::Down);
            selectedPlayer->checkInvincibilityTimeout();

            float deltaTime = deltaClock.restart().asSeconds();

            for (int i = 0; i < enemyCount; i++) {
                if (enemies[i]->isAlive()) {
                    enemies[i]->update(deltaTime, playerBounds, isJumpingOrRolling, selectedPlayer);
                    // ai damge animation 
                    Player* players[] = { &sonic, &tails, &knuckles };
                    for (int j = 0; j < 3; j++) {
                        Player* p = players[j];
                        if (p != selectedPlayer && p->getActiveStatus()) {
                            if (enemies[i]->getBounds().intersects(p->getSprite().getGlobalBounds())) {
                                p->triggerDamageFlash();
                            }
                        }
                    }
                }
            }

            static Clock enemyDamageCooldown;
            if (enemyDamageCooldown.getElapsedTime().asSeconds() > 1.0f) {  // 1 second cooldown
                for (int i = 0; i < enemyCount; i++) {
                    if (enemies[i]->isAlive() &&
                        enemies[i]->getBounds().intersects(playerBounds) &&
                        !isJumpingOrRolling) {

                        if (!selectedPlayer->isInvincible) {
                            selectedPlayer->reduceHealth();
                            cout << "Player damaged by enemy!" << endl;
                            enemyDamageCooldown.restart();
                        }
                    }

                }
            }

            if (selectedPlayer->getHealth() <= 0) {
                cout << "Game Over! Player was defeated by enemy.\n";
                return;
            }

            if (isPlayerNearBreakableWall(sonic, lvl) || isPlayerNearBreakableWall(tails, lvl) || isPlayerNearBreakableWall(knuckles, lvl)) {
                int tileX = (int)(knuckles.getX()) / cell_size;
                int tileY = (int)(knuckles.getY()) / cell_size;

                for (int i = tileY - 1; i <= tileY + 1; i++) {
                    for (int j = tileX - 1; j <= tileX + 1; j++) {
                        if (i >= 0 && i < height && j >= 0 && j < width && lvl[i][j] == 'b') {
                            lvl[i][j] = '\0'; // break the wall

                        }
                    }
                }
            }

            // spike collision: only selected player loses hp
            static Clock damageCooldown;
            for (int i = 0; i < obstacleCount; i++) {
                Spike* spike = dynamic_cast<Spike*>(obstacles[i]);
                if (spike != nullptr && selectedPlayer->getActiveStatus()) {
                    if (spike->checkCollision(selectedPlayer->getX(), selectedPlayer->getY(), Pwidth, Pheight)) {
                        if (damageCooldown.getElapsedTime().asSeconds() > 2) {
                            if (!selectedPlayer->isInvincible) {
                                selectedPlayer->reduceHealth();
                                damageCooldown.restart();
                            }
                            if (selectedPlayer->getHealth() <= 0) {
                                cout << "Game Over! Player was defeated by obstacles.\n";
                                return;
                            }
                        }
                    }
                }
            }

            float fallThreshold = screen_y + 50;  // y-coordinate ... if y> fall threshold it means fallen into pit

            // check if the selected player  fell in pit
            if (selectedPlayer->getY() > fallThreshold) {
                cout << "Game Over! Player fell into a pit." << endl;
                return;
            }

            // respawn ai-controlled players if they fall
            if (&sonic != selectedPlayer && sonic.getY() > fallThreshold) {
                sonic.setX(selectedPlayer->getX() - 50);
                sonic.setY(selectedPlayer->getY() - 100);
            }
            if (&tails != selectedPlayer && tails.getY() > fallThreshold) {
                tails.setX(selectedPlayer->getX() + 50);
                tails.setY(selectedPlayer->getY() - 100);
            }
            if (&knuckles != selectedPlayer && knuckles.getY() > fallThreshold) {
                knuckles.setX(selectedPlayer->getX() + 100);
                knuckles.setY(selectedPlayer->getY() - 100);
            }

            playerBounds = selectedPlayer->getSprite().getGlobalBounds();

            for (int i = 0; i < ringCount; i++) {
                rings[i].update(deltaTime);
                if (rings[i].checkCollision(playerBounds)) {
                    collectedRings++;
                    score += 10; // +10 points per ring
                }
            }

            for (int i = 0; i < extraLifeCount; i++) {
                extraLives[i].update(deltaTime);
                extraLives[i].checkCollision(playerBounds);
            }
            for (int i = 0; i < boostCount; i++) {
                boosts[i].update(deltaTime);
                boosts[i].checkCollision(playerBounds, selectedPlayer);
            }

            window.clear();

            int bgWidth = bgTex.getSize().x;
            int bgRepeats = screen_x / bgWidth + 2;
            for (int i = 0; i < bgRepeats; ++i) {
                background.setPosition(i * bgWidth - (int)cameraOffset % bgWidth, 0);
                window.draw(background);
            }

            display_level(window, height, width, lvl, wallSprite, platformSprite, breakWallSprite, cell_size);

            for (int i = 0; i < obstacleCount; i++) {
                obstacles[i]->draw(window, cameraOffset);
            }

            for (int i = 0; i < extraLifeCount; i++) {
                extraLives[i].draw(window);
            }
            for (int i = 0; i < boostCount; i++) {
                boosts[i].draw(window);
            }
            sonic.draw(window);
            tails.draw(window);
            knuckles.draw(window);
            for (int i = 0; i < enemyCount; i++) {
                enemies[i]->draw(window, cameraOffset, selectedPlayer->getX());
            }


            CircleShape selector(10);
            selector.setFillColor(Color::Red);
            selector.setPosition(selectedPlayer->getX() - cameraOffset + 20, selectedPlayer->getY() - 20);
            window.draw(selector);


            for (int i = 0; i < obstacleCount; i++) {
                Spike* spike = dynamic_cast<Spike*>(obstacles[i]);
                if (spike != nullptr && spike->checkCollision(selectedPlayer->getX(), selectedPlayer->getY(), Pwidth, Pheight)) {
                    static Clock damageCooldown;
                    if (damageCooldown.getElapsedTime().asSeconds() > 2) { // 2 second cooldown
                        selectedPlayer->reduceHealth();
                        damageCooldown.restart();
                    }
                }
            }

            for (int i = 0; i < ringCount; i++) {
                rings[i].draw(window);
            }

            if (collectedRings == ringCount) {
                cout << "Level Complete!" << endl;
                return;
            }

            // draw HUD background box
            RectangleShape hudBox(Vector2f(250, 110));

            hudBox.setFillColor(Color(0, 0, 0, 100));  // semi transparent black
            hudBox.setPosition(10, 10);  // top left
            window.draw(hudBox);

            // draw HUD text on top
            ringText.setPosition(30, 30);
            ringText.setFillColor(Color::Yellow);
            ringText.setString("RINGS: " + to_string(collectedRings));

            scoreText.setPosition(30, 60);
            scoreText.setFillColor(Color::White);
            scoreText.setString("SCORE: " + to_string(score));
            livesText.setPosition(30, 90);
            livesText.setString("LIVES: " + to_string(selectedPlayer->getHealth()));


            window.draw(ringText);
            window.draw(scoreText);
            window.draw(livesText);

            if (selectedPlayer->isBoosted && selectedPlayer->boostClock.getElapsedTime().asSeconds() > 15) {
                selectedPlayer->isBoosted = false;
                if (Sonic* sonic = dynamic_cast<Sonic*>(selectedPlayer))
                    sonic->setSpeed(selectedPlayer->originalSpeed);
                else if (Knuckles* k = dynamic_cast<Knuckles*>(selectedPlayer))
                    selectedPlayer->isInvincible = false;
            }

            window.display();

        }
    }
    float getPlayerX() const { return selectedPlayer->getX(); }
    float getPlayerY() const { return selectedPlayer->getY(); }
    int getRingCount() const { return collectedRings; }
    int getEnemiesDefeated() const {
        int count = 0;
        for (int i = 0; i < enemyCount; i++)
            if (!enemies[i]->isAlive()) count++;
        return count;
    }
    string getSelectedCharacterName() const {
        if (selectedPlayer == &sonic) return "Sonic";
        if (selectedPlayer == &tails) return "Tails";
        return "Knuckles";
    }


    int run(RenderWindow& window) {
        update(window);  // one call only
        return score;
    }

};

//=========================================================================================================================================================================================
//
class Level2 : Level {
private:
    Clock deltaClock;
    Clock spikeCooldown;

    Font font;
    Text ringText, scoreText, livesText;



public:
    Level2() : Level(14, 250) {
        gravity = 1.2f;         // stronger pull
        acceleration = 0.6f;    // faster horizontal movement buildup
        friction = 0.1f;        // very slippery feel
        jumpStrength = -22.0f;

        selectedPlayer = &sonic;
        tails.setCarriedCharacters(&sonic, &knuckles);
        initializeLevelGrid();
        generateLayout();
        setupHUD();
    }

    ~Level2() {
        for (int i = 0; i < height; i++) delete[] lvl[i];
        delete[] lvl;
        for (int i = 0; i < obstacleCount; i++) delete obstacles[i];
        for (int i = 0; i < enemyCount; i++) delete enemies[i];
    }

    void initializeLevelGrid() {
        lvl = new char* [height];
        for (int i = 0; i < height; i++)
            lvl[i] = new char[width] { '\0' };
    }


    void setupHUD() {
        font.loadFromFile("Data/arial.ttf");
        ringText.setFont(font);
        ringText.setCharacterSize(24);
        ringText.setFillColor(Color::Yellow);
        ringText.setPosition(20, 30);

        scoreText.setFont(font);
        scoreText.setCharacterSize(24);
        scoreText.setFillColor(Color::White);
        scoreText.setPosition(20, 60);

        livesText.setFont(font);
        livesText.setCharacterSize(24);
        livesText.setFillColor(Color::Cyan);
        livesText.setPosition(20, 90);
    }

    void generateLayout() {
        for (int j = 0; j < width;) {
            bool makePit = rand() % 6 == 0;
            int groupSize = rand() % 5 + 3;
            for (int k = 0; k < groupSize && (j + k) < width; k++) {
                for (int i = 11; i < height; i++)
                    lvl[i][j + k] = makePit ? '\0' : 'w';
                if (makePit) lvl[7 + rand() % 2][j + k] = 'p';
            }
            j += groupSize;
        }

        for (int i = 0; i <= 1; i++)
            for (int j = 0; j < width; j++)
                lvl[i][j] = 'w';

        int placedBreakWalls = 0;
        while (placedBreakWalls < 30) {
            int row = rand() % (height - 3) + 3;
            int col = rand() % (width - 5);
            if (lvl[row][col] == 'w') {
                lvl[row][col] = 'b';
                placedBreakWalls++;
            }
        }

        for (int i = 0; i < 5; i++)
            rings[ringCount++].load("Data/ring.png", 1100 + i * 30, 400);

        for (int i = 0; i < 3; i++)
            rings[ringCount++].load("Data/ring.png", 1400 + i * 30, 480);

        for (int i = 0; i < 4; i++)
            rings[ringCount++].load("Data/ring.png", 1600 + i * 30, 300);

        for (int i = 0; i < 5; i++)
            rings[ringCount++].load("Data/ring.png", 1900 + i * 30, 250);

        for (int i = 0; i < 4; i++)
            rings[ringCount++].load("Data/ring.png", 2100, 400 - i * 40);

        for (int x = 2200; x < width * cell_size; x += 600) {
            int clusterSize = rand() % 3 + 3;
            int y = rand() % 200 + 300;
            for (int i = 0; i < clusterSize && ringCount < 500; i++)
                rings[ringCount++].load("Data/ring.png", x + i * 30, y);
            if (rand() % 2 == 0 && ringCount + 3 < 500)
                for (int i = 0; i < 3; i++)
                    rings[ringCount++].load("Data/ring.png", x + clusterSize * 30, y - i * 40);
        }

        for (int i = 0; i < 8; i++) {
            int col = 25 + i * 15;
            int row = 7 + rand() % 2;
            float x = col * cell_size;
            float y = row * cell_size;
            extraLives[extraLifeCount++].load("Data/heart.png", x, y);
            if (boostCount < 5)
                boosts[boostCount++].load("Data/boost.png", x + 30, y - 50);
        }

        Texture spikeTex;
        spikeTex.loadFromFile("Data/spike.png");
        for (int col = 10; col < width - 10; col += rand() % 20 + 10) {
            if (lvl[10][col] == 'w') {
                float spikeX = col * cell_size;
                float spikeY = 10 * cell_size - 32;
                obstacles[obstacleCount++] = new Spike(spikeTex, spikeX, spikeY);
            }
        }

        BatBrain* bat = new BatBrain();
        bat->load("Data/batbrain.png", 800, 300, 3, FloatRect(0, 0, width * cell_size, screen_y));
        enemies[enemyCount++] = bat;

        BeeBot* bee = new BeeBot();
        bee->load("Data/beebot.png", 2500, 250, 5, FloatRect(2000, 0, 1000, screen_y));
        enemies[enemyCount++] = bee;

        CrabMeat* crab = new CrabMeat();
        crab->load("Data/crabmeat.png", 3500, 600, 4, FloatRect(3400, 0, 800, screen_y));
        enemies[enemyCount++] = crab;

        MotoBug* moto = new MotoBug();
        moto->load("Data/motobug.png", 4500, 600, 3, FloatRect(4400, 0, 1000, screen_y));
        enemies[enemyCount++] = moto;
    }



    void update(RenderWindow& window) {

        Music bgMusic;
        if (!bgMusic.openFromFile("Data/magical_journey.ogg.opus")) {
            cout << "Could not load background music" << endl;
        }
        bgMusic.setLoop(true);
        bgMusic.play();




        score = 0;



        const int MAX_BREAKABLE_WALLS = 30;
        int placedBreakWalls = 0;


        const int MAX_LIVES = 8;
        ExtraLife extraLives[MAX_LIVES];
        int extraLifeCount = 0;

        const int MAX_BOOSTS = 5;
        SpecialBoost boosts[MAX_BOOSTS];
        int boostCount = 0;

        for (int i = 0; i < MAX_LIVES; i++) {
            float x = 1500 + i * 900;
            float y = 300 + rand() % 200;

        }

        for (int i = 0; i < 5; i++)
            rings[ringCount++].load("Data/ring.png", 1100 + i * 30, 400);

        for (int i = 0; i < 3; i++)
            rings[ringCount++].load("Data/ring.png", 1400 + i * 30, 480);

        for (int i = 0; i < 4; i++)
            rings[ringCount++].load("Data/ring.png", 1600 + i * 30, 300);

        for (int i = 0; i < 5; i++)
            rings[ringCount++].load("Data/ring.png", 1900 + i * 30, 250);

        for (int i = 0; i < 4; i++)
            rings[ringCount++].load("Data/ring.png", 2100, 400 - i * 40);

        for (int i = 0; i < 4; i++)
            rings[ringCount++].load("Data/ring.png", 2100, 400 - i * 40); // vertical column

        // --- Auto-generated ring clusters across the level ---
        for (int x = 2200; x < 60000; x += 600) {
            int clusterSize = rand() % 3 + 3;  // 3 to 5 rings
            int y = rand() % 200 + 300;        // Random vertical placement between 300 and 500

            for (int i = 0; i < clusterSize && ringCount < 500; i++) {
                rings[ringCount++].load("Data/ring.png", x + i * 30, y);
            }

            // Optional vertical column of rings
            if (rand() % 2 == 0 && ringCount + 3 < 500) {
                for (int i = 0; i < 3; i++) {
                    rings[ringCount++].load("Data/ring.png", x + clusterSize * 30, y - i * 40);
                }
            }
        }


        // Load Extra Life Hearts
        for (int i = 0; i < MAX_LIVES; i++) {
            int col = 25 + i * 15; // space them apart horizontally
            int row = 7 + rand() % 2; // row 7 or 8

            float x = col * cell_size;
            float y = row * cell_size;

            extraLives[extraLifeCount++].load("Data/heart.png", x, y);
            if (boostCount < MAX_BOOSTS)
                boosts[boostCount++].load("Data/boost.png", x + 30, y - 50);

            if (row > 0 && lvl[row - 1][col] == '\0')
                lvl[row - 1][col] = 'b';
            if (row < height - 1 && lvl[row + 1][col] == '\0')
                lvl[row + 1][col] = 'b';
            if (col > 0 && lvl[row][col - 1] == '\0') lvl[row][col - 1] = 'b';
            if (col < width - 1 && lvl[row][col + 1] == '\0')
                lvl[row][col + 1] = 'b';

        }

        for (int j = 0; j < width;) {
            bool makePit = rand() % 6 == 0;
            int groupSize = rand() % 5 + 3;

            for (int k = 0; k < groupSize && (j + k) < width; k++) {
                for (int i = 11; i < height; i++) {
                    lvl[i][j + k] = makePit ? '\0' : 'w';
                }

                if (makePit) {
                    int platformRow = 7 + rand() % 2;
                    lvl[platformRow][j + k] = 'p';
                }
            }
            j += groupSize;
        }

        for (int i = 0; i <= 1; i++) {
            for (int j = 0; j < width; j++) lvl[i][j] = 'w';
        }

        for (int j = 2; j < width - 3;) {
            int groupSize = rand() % 5 + 1;
            for (int k = 0; k < groupSize && (j + k) < width; k++)
                if (lvl[10][j + k] == '\0') lvl[10][j + k] = 'w';
            j += groupSize + (rand() % 4 + 2);
        }

        for (int j = 3; j < width - 3;) {
            int groupSize = rand() % 5 + 1;
            for (int k = 0; k < groupSize && (j + k) < width; k++)
                if (lvl[2][j + k] == '\0') lvl[2][j + k] = 'w';
            j += groupSize + (rand() % 4 + 2);
        }

        for (int j = 5; j < width - 10;) {
            int clusterSize = rand() % 4 + 1;
            int row = rand() % 6 + 3;
            for (int k = 0; k < clusterSize && (j + k) < width; k++)
                lvl[row][j + k] = 'p';
            j += clusterSize + (rand() % 6 + 2);
        }

        for (int row = 5; row <= 9; row++)
            lvl[row][50] = 'p';

        for (int row = 6; row <= 11; row++)
            lvl[row][80] = 'p';

        for (int row = 4; row <= 11; row++)
            lvl[row][60] = 'w';

        for (int row = 5; row <= 11; row++)
            lvl[row][90] = 'w';

        lvl[6][55] = 'p';
        lvl[4][58] = 'p';
        lvl[7][62] = 'p';
        lvl[5][65] = 'p';

        lvl[3][70] = 'p';
        lvl[3][71] = 'p';

        while (placedBreakWalls < MAX_BREAKABLE_WALLS) {
            int row = rand() % (height - 3) + 3;
            int col = rand() % (width - 5);

            if (lvl[row][col] == 'w') {
                lvl[row][col] = 'b';
                placedBreakWalls++;
            }
        }

        lvl[2][41] = 'b';
        lvl[8][57] = 'b';
        lvl[9][57] = 'b';

        Texture wallTex, platformTex, bgTex, spikeTex, breakTex;
        wallTex.loadFromFile("Data/Snow_1.png");
        platformTex.loadFromFile("Data/Snow_2.png");
        bgTex.loadFromFile("Data/snowbg.png");
        spikeTex.loadFromFile("Data/spike.png");

        Sprite wallSprite(wallTex), platformSprite(platformTex), background(bgTex);
        breakTex.loadFromFile("Data/BreakGlacier.png");

        Sprite breakWallSprite(breakTex);

        for (int col = 10; col < width - 10; col += rand() % 20 + 10) {
            if (lvl[10][col] == 'w') {
                float spikeX = col * cell_size;
                float spikeY = 10 * cell_size - 32; // row 10
                obstacles[obstacleCount++] = new Spike(spikeTex, spikeX, spikeY);
            }
        }
        float velocityY = 0, gravity = 1, terminal_Velocity = 20, jumpStrength = -20;
        int raw_img_x = 24, raw_img_y = 35;
        float scale_x = 2.5, scale_y = 2.5;


        int Pheight = raw_img_y * scale_y, Pwidth = raw_img_x * scale_x;
        int hit_box_factor_x = 8 * scale_x, hit_box_factor_y = 5 * scale_y;
        bool onGround = false;

        float offset_y = 0;

        Event ev;
        while (window.isOpen()) {
            while (window.pollEvent(ev)) {
                if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::Z) {
                    if (selectedPlayer == &sonic) {
                        if (tails.getActiveStatus()) selectedPlayer = &tails;
                        else if (knuckles.getActiveStatus()) selectedPlayer = &knuckles;
                    }
                    else if (selectedPlayer == &tails) {
                        if (knuckles.getActiveStatus()) selectedPlayer = &knuckles;
                        else if (sonic.getActiveStatus()) selectedPlayer = &sonic;
                    }
                    else if (selectedPlayer == &knuckles) {
                        if (sonic.getActiveStatus()) selectedPlayer = &sonic;
                        else if (tails.getActiveStatus()) selectedPlayer = &tails;
                    }

                }
            }


            if (Keyboard::isKeyPressed(Keyboard::Escape)) window.close();

            sonic.move(jumpStrength, acceleration, friction);
            tails.move(jumpStrength, acceleration, friction);
            knuckles.move(jumpStrength, acceleration, friction);



            if (selectedPlayer == &knuckles) {
                knuckles.punch(lvl);
            }


            cameraOffset = selectedPlayer->getX() - screen_x / 2;
            if (cameraOffset < 0) cameraOffset = 0;

            float maxOffset = width * cell_size - screen_x;
            if (cameraOffset > maxOffset) cameraOffset = maxOffset;

            float spacing = 50.0f;  // Distance between characters

            if (selectedPlayer != &sonic) {
                float targetX = selectedPlayer->getX() - spacing;
                if (abs(targetX - sonic.getX()) > 0.5f)
                    sonic.getX() += (targetX - sonic.getX()) * 0.1f;
            }


            if (selectedPlayer != &tails) {
                float targetX = selectedPlayer->getX() - spacing;
                if (abs(targetX - tails.getX()) > 0.5f)
                    tails.getX() += (targetX - tails.getX()) * 0.1f;
            }

            if (selectedPlayer != &knuckles) {
                float targetX = selectedPlayer->getX() - spacing;
                if (abs(targetX - knuckles.getX()) > 0.5f)
                    knuckles.getX() += (targetX - knuckles.getX()) * 0.1f;
            }


            sonic.applyGravity(lvl, hit_box_factor_x, hit_box_factor_y, Pheight, Pwidth, gravity, 20.0f);
            tails.applyGravity(lvl, hit_box_factor_x, hit_box_factor_y, Pheight, Pwidth, gravity, 20.0f);
            knuckles.applyGravity(lvl, hit_box_factor_x, hit_box_factor_y, Pheight, Pwidth, gravity, 20.0f);

            FloatRect playerBounds = selectedPlayer->getSprite().getGlobalBounds();
            bool isJumpingOrRolling = Keyboard::isKeyPressed(Keyboard::Up) || Keyboard::isKeyPressed(Keyboard::Down);  // Example logic
            selectedPlayer->checkInvincibilityTimeout();

            float deltaTime = deltaClock.restart().asSeconds();

            for (int i = 0; i < enemyCount; i++) {
                if (enemies[i]->isAlive()) {
                    enemies[i]->update(deltaTime, playerBounds, isJumpingOrRolling, selectedPlayer);

                    Player* players[] = { &sonic, &tails, &knuckles };
                    for (int j = 0; j < 3; j++) {
                        Player* p = players[j];
                        if (p != selectedPlayer && p->getActiveStatus()) {
                            if (enemies[i]->getBounds().intersects(p->getSprite().getGlobalBounds())) {
                                p->triggerDamageFlash();
                            }
                        }
                    }
                }
            }

            static Clock enemyDamageCooldown;
            if (enemyDamageCooldown.getElapsedTime().asSeconds() > 1.0f) {
                for (int i = 0; i < enemyCount; i++) {
                    if (enemies[i]->isAlive() &&
                        enemies[i]->getBounds().intersects(playerBounds) &&
                        !isJumpingOrRolling) {

                        selectedPlayer->reduceHealth();
                        cout << "Player damaged by enemy!" << endl;

                        enemyDamageCooldown.restart();
                        break;
                    }
                }
            }

            if (selectedPlayer->getHealth() <= 0) {
                cout << "Game Over! Player was defeated by enemy.\n";
                return;
            }


            if (isPlayerNearBreakableWall(sonic, lvl) || isPlayerNearBreakableWall(tails, lvl) || isPlayerNearBreakableWall(knuckles, lvl)) {
                int tileX = (int)(knuckles.getX()) / cell_size;
                int tileY = (int)(knuckles.getY()) / cell_size;

                for (int i = tileY - 1; i <= tileY + 1; i++) {
                    for (int j = tileX - 1; j <= tileX + 1; j++) {
                        if (i >= 0 && i < height && j >= 0 && j < width && lvl[i][j] == 'b') {
                            lvl[i][j] = '\0'; // break the wall

                        }
                    }
                }
            }

            static Clock damageCooldown;
            for (int i = 0; i < obstacleCount; i++) {
                Spike* spike = dynamic_cast<Spike*>(obstacles[i]);
                if (spike != nullptr && selectedPlayer->getActiveStatus()) {
                    if (spike->checkCollision(selectedPlayer->getX(), selectedPlayer->getY(), Pwidth, Pheight)) {
                        if (damageCooldown.getElapsedTime().asSeconds() > 2) {
                            selectedPlayer->reduceHealth();
                            damageCooldown.restart();

                            if (selectedPlayer->getHealth() <= 0) {
                                cout << "Game Over! Player was defeated by obstacles.\n";
                                return;
                            }
                        }
                    }
                }
            }
            float fallThreshold = screen_y + 50;
            if (selectedPlayer->getY() > fallThreshold) {
                cout << "Game Over! Player fell into a pit." << endl;
                return;
            }
            if (&sonic != selectedPlayer && sonic.getY() > fallThreshold) {
                sonic.setX(selectedPlayer->getX() - 50);
                sonic.setY(selectedPlayer->getY() - 100);
            }
            if (&tails != selectedPlayer && tails.getY() > fallThreshold) {
                tails.setX(selectedPlayer->getX() + 50);
                tails.setY(selectedPlayer->getY() - 100);
            }
            if (&knuckles != selectedPlayer && knuckles.getY() > fallThreshold) {
                knuckles.setX(selectedPlayer->getX() + 100);
                knuckles.setY(selectedPlayer->getY() - 100);
            }

            playerBounds = selectedPlayer->getSprite().getGlobalBounds();

            for (int i = 0; i < ringCount; i++) {
                rings[i].update(deltaTime);
                if (rings[i].checkCollision(playerBounds)) {
                    collectedRings++;
                    score += 10;
                }
            }

            for (int i = 0; i < extraLifeCount; i++) {
                extraLives[i].update(deltaTime);
                extraLives[i].checkCollision(playerBounds);
            }
            for (int i = 0; i < boostCount; i++) {
                boosts[i].update(deltaTime);
                boosts[i].checkCollision(playerBounds, selectedPlayer);
            }


            int bgWidth = bgTex.getSize().x;
            int bgRepeats = screen_x / bgWidth + 2;
            for (int i = 0; i < bgRepeats; ++i) {
                background.setPosition(i * bgWidth - (int)cameraOffset % bgWidth, 0);
                window.draw(background);
            }

            display_level(window, height, width, lvl, wallSprite, platformSprite, breakWallSprite, cell_size);


            for (int i = 0; i < obstacleCount; i++) {
                obstacles[i]->draw(window, cameraOffset);
            }

            for (int i = 0; i < extraLifeCount; i++) {
                extraLives[i].draw(window);
            }
            for (int i = 0; i < boostCount; i++) {
                boosts[i].draw(window);
            }
            sonic.draw(window);
            tails.draw(window);
            knuckles.draw(window);
            for (int i = 0; i < enemyCount; i++) {
                enemies[i]->draw(window, cameraOffset, selectedPlayer->getX());
            }

            CircleShape selector(10);
            selector.setFillColor(Color::Red);
            selector.setPosition(selectedPlayer->getX() - cameraOffset + 20, selectedPlayer->getY() - 20);
            window.draw(selector);


            for (int i = 0; i < obstacleCount; i++) {
                Spike* spike = dynamic_cast<Spike*>(obstacles[i]);
                if (spike != nullptr && spike->checkCollision(selectedPlayer->getX(), selectedPlayer->getY(), Pwidth, Pheight)) {
                    static Clock damageCooldown;
                    if (damageCooldown.getElapsedTime().asSeconds() > 2) {
                        selectedPlayer->reduceHealth();
                        damageCooldown.restart();
                    }
                }
            }

            for (int i = 0; i < ringCount; i++) {
                rings[i].draw(window);
            }

            if (collectedRings == ringCount) {
                cout << "Level Complete!" << endl;
                return;
            }

            RectangleShape hudBox(Vector2f(250, 110));

            hudBox.setPosition(10, 10);
            window.draw(hudBox);

            ringText.setPosition(30, 30);
            ringText.setFillColor(Color::Yellow);
            ringText.setString("RINGS: " + to_string(collectedRings));

            scoreText.setPosition(30, 60);
            scoreText.setFillColor(Color::White);
            scoreText.setString("SCORE: " + to_string(score));
            livesText.setPosition(30, 90);
            livesText.setString("LIVES: " + to_string(selectedPlayer->getHealth()));


            window.draw(ringText);
            window.draw(scoreText);
            window.draw(livesText);

            if (selectedPlayer->isBoosted && selectedPlayer->boostClock.getElapsedTime().asSeconds() > 15) {
                selectedPlayer->isBoosted = false;
                if (Sonic* sonic = dynamic_cast<Sonic*>(selectedPlayer))
                    sonic->setSpeed(selectedPlayer->originalSpeed);
                else if (Knuckles* k = dynamic_cast<Knuckles*>(selectedPlayer))
                    selectedPlayer->isInvincible = false;
            }


            window.display();

        }
    }

    float getPlayerX() const {
        return selectedPlayer->getX();
    }
    float getPlayerY() const {
        return selectedPlayer->getY();
    }
    int getRingCount() const {
        return collectedRings;
    }
    int getEnemiesDefeated() const {
        int count = 0;
        for (int i = 0; i < enemyCount; i++)
            if (!enemies[i]->isAlive()) count++;
        return count;
    }
    string getSelectedCharacterName() const {
        if (selectedPlayer == &sonic) return "Sonic";
        if (selectedPlayer == &tails) return "Tails";
        return "Knuckles";
    }


    int run(RenderWindow& window) {
        update(window);
        return score;
    }
};

class Level3 : Level {
private:
    Clock deltaClock;
    Clock spikeCooldown;

    Font font;
    Text ringText, scoreText, livesText;



public:
    Level3() : Level(14, 300) {
        gravity = 0.3f;         // lower gravity for floaty jumps in space
        acceleration = 0.2f;    // slower acceleration
        friction = 0.4f;        // moderate slipperiness
        jumpStrength = -12.0f;  // softer jumps

        selectedPlayer = &sonic;
        tails.setCarriedCharacters(&sonic, &knuckles);
        initializeLevelGrid();
        generateLayout();
        setupHUD();
    }

    ~Level3() {
        for (int i = 0; i < height; i++) delete[] lvl[i];
        delete[] lvl;
        for (int i = 0; i < obstacleCount; i++) delete obstacles[i];
        for (int i = 0; i < enemyCount; i++) delete enemies[i];
    }

    void initializeLevelGrid() {
        lvl = new char* [height];
        for (int i = 0; i < height; i++)
            lvl[i] = new char[width] { '\0' };
    }


    void setupHUD() {
        font.loadFromFile("Data/arial.ttf");
        ringText.setFont(font);
        ringText.setCharacterSize(24);
        ringText.setFillColor(Color::Yellow);
        ringText.setPosition(20, 30);

        scoreText.setFont(font);
        scoreText.setCharacterSize(24);
        scoreText.setFillColor(Color::White);
        scoreText.setPosition(20, 60);

        livesText.setFont(font);
        livesText.setCharacterSize(24);
        livesText.setFillColor(Color::Cyan);
        livesText.setPosition(20, 90);
    }

    void generateLayout() {
        for (int j = 0; j < width;) {
            bool makePit = rand() % 6 == 0;
            int groupSize = rand() % 5 + 3;
            for (int k = 0; k < groupSize && (j + k) < width; k++) {
                for (int i = 11; i < height; i++)
                    lvl[i][j + k] = makePit ? '\0' : 'w';
                if (makePit) lvl[7 + rand() % 2][j + k] = 'p';
            }
            j += groupSize;
        }

        for (int i = 0; i <= 1; i++)
            for (int j = 0; j < width; j++)
                lvl[i][j] = 'w';

        int placedBreakWalls = 0;
        while (placedBreakWalls < 30) {
            int row = rand() % (height - 3) + 3;
            int col = rand() % (width - 5);
            if (lvl[row][col] == 'w') {
                lvl[row][col] = 'b';
                placedBreakWalls++;
            }
        }

        for (int i = 0; i < 5; i++)
            rings[ringCount++].load("Data/ring.png", 1100 + i * 30, 400);

        for (int i = 0; i < 3; i++)
            rings[ringCount++].load("Data/ring.png", 1400 + i * 30, 480);

        for (int i = 0; i < 4; i++)
            rings[ringCount++].load("Data/ring.png", 1600 + i * 30, 300);

        for (int i = 0; i < 5; i++)
            rings[ringCount++].load("Data/ring.png", 1900 + i * 30, 250);

        for (int i = 0; i < 4; i++)
            rings[ringCount++].load("Data/ring.png", 2100, 400 - i * 40);

        for (int x = 2200; x < 60000; x += 600) {
            int clusterSize = rand() % 3 + 3;
            int y = rand() % 200 + 300;
            for (int i = 0; i < clusterSize && ringCount < 500; i++)
                rings[ringCount++].load("Data/ring.png", x + i * 30, y);
            if (rand() % 2 == 0 && ringCount + 3 < 500)
                for (int i = 0; i < 3; i++)
                    rings[ringCount++].load("Data/ring.png", x + clusterSize * 30, y - i * 40);
        }

        for (int i = 0; i < 8; i++) {
            int col = 25 + i * 15;
            int row = 7 + rand() % 2;
            float x = col * cell_size;
            float y = row * cell_size;
            extraLives[extraLifeCount++].load("Data/heart.png", x, y);
            if (boostCount < 5)
                boosts[boostCount++].load("Data/boost.png", x + 30, y - 50);
        }

        Texture spikeTex;
        spikeTex.loadFromFile("Data/spike.png");
        for (int col = 10; col < width - 10; col += rand() % 20 + 10) {
            if (lvl[10][col] == 'w') {
                float spikeX = col * cell_size;
                float spikeY = 10 * cell_size - 32;
                obstacles[obstacleCount++] = new Spike(spikeTex, spikeX, spikeY);
            }
        }

        BatBrain* bat = new BatBrain();
        bat->load("Data/batbrain.png", 800, 300, 3, FloatRect(0, 0, width * cell_size, screen_y));
        enemies[enemyCount++] = bat;

        BeeBot* bee = new BeeBot();
        bee->load("Data/beebot.png", 2500, 250, 5, FloatRect(2000, 0, 1000, screen_y));
        enemies[enemyCount++] = bee;

        CrabMeat* crab = new CrabMeat();
        crab->load("Data/crabmeat.png", 3500, 600, 4, FloatRect(3400, 0, 800, screen_y));
        enemies[enemyCount++] = crab;

        MotoBug* moto = new MotoBug();
        moto->load("Data/motobug.png", 4500, 600, 3, FloatRect(4400, 0, 1000, screen_y));
        enemies[enemyCount++] = moto;
    }



    void update(RenderWindow& window) {


        Music bgMusic;
        if (!bgMusic.openFromFile("Data/risk.ogg.opus")) {
            cout << "Could not load background music" << endl;
        }
        bgMusic.setLoop(true);
        bgMusic.play();



        const int MAX_RINGS = 200;
        Ring rings[MAX_RINGS];
        int ringCount = 0, collectedRings = 0;
        score = 0;
        Text scoreText;
        Text livesText;


        const int MAX_BREAKABLE_WALLS = 30;
        int placedBreakWalls = 0;

        const int MAX_LIVES = 8;
        ExtraLife extraLives[MAX_LIVES];
        int extraLifeCount = 0;

        const int MAX_BOOSTS = 5;
        SpecialBoost boosts[MAX_BOOSTS];
        int boostCount = 0;

        for (int i = 0; i < MAX_LIVES; i++) {
            float x = 1500 + i * 900;
            float y = 300 + rand() % 200;

        }

        for (int i = 0; i < 5; i++)
            rings[ringCount++].load("Data/ring.png", 1100 + i * 30, 400);

        for (int i = 0; i < 3; i++)
            rings[ringCount++].load("Data/ring.png", 1400 + i * 30, 480);

        for (int i = 0; i < 4; i++)
            rings[ringCount++].load("Data/ring.png", 1600 + i * 30, 300);

        for (int i = 0; i < 5; i++)
            rings[ringCount++].load("Data/ring.png", 1900 + i * 30, 250);


        for (int i = 0; i < 4; i++)
            rings[ringCount++].load("Data/ring.png", 2100, 400 - i * 40); // vertical column

        for (int i = 0; i < 4; i++)
            rings[ringCount++].load("Data/ring.png", 2100, 400 - i * 40); // vertical column

        for (int x = 2200; x < 60000; x += 600) {
            int clusterSize = rand() % 3 + 3;
            int y = rand() % 200 + 300;

            for (int i = 0; i < clusterSize && ringCount < MAX_RINGS; i++) {
                rings[ringCount++].load("Data/ring.png", x + i * 30, y);
            }

            if (rand() % 2 == 0 && ringCount + 3 < MAX_RINGS) {
                for (int i = 0; i < 3; i++) {
                    rings[ringCount++].load("Data/ring.png", x + clusterSize * 30, y - i * 40);
                }
            }
        }

        for (int i = 0; i < MAX_LIVES; i++) {
            int col = 25 + i * 15;
            int row = 7 + rand() % 2;

            float x = col * cell_size;
            float y = row * cell_size;

            extraLives[extraLifeCount++].load("Data/heart.png", x, y);
            if (boostCount < MAX_BOOSTS)
                boosts[boostCount++].load("Data/boost.png", x + 30, y - 50);

            // place breakable walls around them (left, right, top, bottom)
            if (row > 0 && lvl[row - 1][col] == '\0')
                lvl[row - 1][col] = 'b';
            if (row < height - 1 && lvl[row + 1][col] == '\0')
                lvl[row + 1][col] = 'b';
            if (col > 0 && lvl[row][col - 1] == '\0')
                lvl[row][col - 1] = 'b';
            if (col < width - 1 && lvl[row][col + 1] == '\0')
                lvl[row][col + 1] = 'b';

        }

        const int MAX_OBSTACLES = 100;
        Obstacle* obstacles[MAX_OBSTACLES];
        int obstacleCount = 0;

        for (int j = 0; j < width;) {
            bool makePit = rand() % 6 == 0;
            int groupSize = rand() % 5 + 3;

            for (int k = 0; k < groupSize && (j + k) < width; k++) {
                for (int i = 11; i < height; i++) {
                    lvl[i][j + k] = makePit ? '\0' : 'w';
                }

                if (makePit) {
                    int platformRow = 7 + rand() % 2;
                    lvl[platformRow][j + k] = 'p';
                }
            }
            j += groupSize;
        }

        for (int i = 0; i <= 1; i++) {
            for (int j = 0; j < width; j++) lvl[i][j] = 'w';
        }

        for (int j = 2; j < width - 3;) {
            int groupSize = rand() % 5 + 1;
            for (int k = 0; k < groupSize && (j + k) < width; k++)
                if (lvl[10][j + k] == '\0') lvl[10][j + k] = 'w';
            j += groupSize + (rand() % 4 + 2);
        }

        for (int j = 3; j < width - 3;) {
            int groupSize = rand() % 5 + 1;
            for (int k = 0; k < groupSize && (j + k) < width; k++)
                if (lvl[2][j + k] == '\0') lvl[2][j + k] = 'w';
            j += groupSize + (rand() % 4 + 2);
        }

        for (int j = 5; j < width - 10;) {
            int clusterSize = rand() % 4 + 1;
            int row = rand() % 6 + 3;
            for (int k = 0; k < clusterSize && (j + k) < width; k++)
                lvl[row][j + k] = 'p';
            j += clusterSize + (rand() % 6 + 2);
        }

        for (int row = 5; row <= 9; row++)
            lvl[row][50] = 'p';

        for (int row = 6; row <= 11; row++)
            lvl[row][80] = 'p';

        for (int row = 4; row <= 11; row++)
            lvl[row][60] = 'w';

        for (int row = 5; row <= 11; row++)
            lvl[row][90] = 'w';

        lvl[6][55] = 'p';
        lvl[4][58] = 'p';
        lvl[7][62] = 'p';
        lvl[5][65] = 'p';


        lvl[3][70] = 'p';
        lvl[3][71] = 'p';


        while (placedBreakWalls < MAX_BREAKABLE_WALLS) {
            int row = rand() % (height - 3) + 3;
            int col = rand() % (width - 5);

            if (lvl[row][col] == 'w') {
                lvl[row][col] = 'b';
                placedBreakWalls++;
            }
        }
        lvl[2][41] = 'b';
        lvl[8][57] = 'b';
        lvl[9][57] = 'b';

        Texture wallTex, platformTex, bgTex, spikeTex, breakTex;
        wallTex.loadFromFile("Data/Ground_2.png");
        platformTex.loadFromFile("Data/Danger_1.png");
        bgTex.loadFromFile("Data/bg.3.png");
        spikeTex.loadFromFile("Data/spike.png");
        Sprite wallSprite(wallTex), platformSprite(platformTex), background(bgTex);
        breakTex.loadFromFile("Data/Ground_1.png");

        Sprite breakWallSprite(breakTex);


        for (int col = 10; col < width - 10; col += rand() % 20 + 10) {
            if (lvl[10][col] == 'w') {
                float spikeX = col * cell_size;
                float spikeY = 10 * cell_size - 32; // row 10
                obstacles[obstacleCount++] = new Spike(spikeTex, spikeX, spikeY);
            }
        }

        BatBrain* bat = new BatBrain();
        bat->load("Data/batbrain.png", 800, 300, 3, FloatRect(0, 0, width * cell_size, screen_y));
        enemies[enemyCount++] = bat;

        BeeBot* bee = new BeeBot();
        bee->load("Data/beebot.png", 2500, 250, 5, FloatRect(2000, 0, 1000, screen_y));
        enemies[enemyCount++] = bee;
        CrabMeat* crab = new CrabMeat();
        crab->load("Data/crabmeat.png", 3500, 600, 4, FloatRect(3400, 0, 800, screen_y));
        enemies[enemyCount++] = crab;

        MotoBug* moto = new MotoBug();
        moto->load("Data/motobug.png", 4500, 600, 3, FloatRect(4400, 0, 1000, screen_y));
        enemies[enemyCount++] = moto;


        float velocityY = 0, gravity = 1, terminal_Velocity = 20, jumpStrength = -20;
        int raw_img_x = 24, raw_img_y = 35;
        float scale_x = 2.5, scale_y = 2.5;


        int Pheight = raw_img_y * scale_y, Pwidth = raw_img_x * scale_x;
        int hit_box_factor_x = 8 * scale_x, hit_box_factor_y = 5 * scale_y;


        bool onGround = false;
        float offset_y = 0;

        Event ev;
        while (window.isOpen()) {
            while (window.pollEvent(ev)) {
                if (ev.type == Event::KeyPressed && ev.key.code == Keyboard::Z) {
                    if (selectedPlayer == &sonic) {
                        if (tails.getActiveStatus()) selectedPlayer = &tails;
                        else if (knuckles.getActiveStatus()) selectedPlayer = &knuckles;
                    }
                    else if (selectedPlayer == &tails) {
                        if (knuckles.getActiveStatus()) selectedPlayer = &knuckles;
                        else if (sonic.getActiveStatus()) selectedPlayer = &sonic;
                    }
                    else if (selectedPlayer == &knuckles) {
                        if (sonic.getActiveStatus()) selectedPlayer = &sonic;
                        else if (tails.getActiveStatus()) selectedPlayer = &tails;
                    }

                }
            }

            if (Keyboard::isKeyPressed(Keyboard::Escape)) window.close();

            sonic.move(jumpStrength, acceleration, friction);
            tails.move(jumpStrength, acceleration, friction);
            knuckles.move(jumpStrength, acceleration, friction);

            if (selectedPlayer == &knuckles) {
                knuckles.punch(lvl);
            }


            cameraOffset = selectedPlayer->getX() - screen_x / 2;
            if (cameraOffset < 0) cameraOffset = 0;
            float maxOffset = width * cell_size - screen_x;
            if (cameraOffset > maxOffset) cameraOffset = maxOffset;


            float spacing = 50.0f;  // Distance between characters

            if (selectedPlayer != &sonic) {
                float targetX = selectedPlayer->getX() - spacing;
                if (abs(targetX - sonic.getX()) > 0.5f)
                    sonic.getX() += (targetX - sonic.getX()) * 0.1f;
            }


            if (selectedPlayer != &tails) {
                float targetX = selectedPlayer->getX() - spacing;
                if (abs(targetX - tails.getX()) > 0.5f)
                    tails.getX() += (targetX - tails.getX()) * 0.1f;
            }

            if (selectedPlayer != &knuckles) {
                float targetX = selectedPlayer->getX() - spacing;
                if (abs(targetX - knuckles.getX()) > 0.5f)
                    knuckles.getX() += (targetX - knuckles.getX()) * 0.1f;
            }



            sonic.applyGravity(lvl, hit_box_factor_x, hit_box_factor_y, Pheight, Pwidth, gravity, 20.0f);
            tails.applyGravity(lvl, hit_box_factor_x, hit_box_factor_y, Pheight, Pwidth, gravity, 20.0f);
            knuckles.applyGravity(lvl, hit_box_factor_x, hit_box_factor_y, Pheight, Pwidth, gravity, 20.0f);

            FloatRect playerBounds = selectedPlayer->getSprite().getGlobalBounds();
            bool isJumpingOrRolling = Keyboard::isKeyPressed(Keyboard::Up) || Keyboard::isKeyPressed(Keyboard::Down);  // Example logic
            selectedPlayer->checkInvincibilityTimeout();

            float deltaTime = deltaClock.restart().asSeconds();

            for (int i = 0; i < enemyCount; i++) {
                if (enemies[i]->isAlive()) {
                    enemies[i]->update(deltaTime, playerBounds, isJumpingOrRolling, selectedPlayer);


                    Player* players[] = { &sonic, &tails, &knuckles };
                    for (int j = 0; j < 3; j++) {
                        Player* p = players[j];
                        if (p != selectedPlayer && p->getActiveStatus()) {
                            if (enemies[i]->getBounds().intersects(p->getSprite().getGlobalBounds())) {
                                p->triggerDamageFlash();
                            }
                        }
                    }
                }
            }

            static Clock enemyDamageCooldown;
            if (enemyDamageCooldown.getElapsedTime().asSeconds() > 1.0f) {
                for (int i = 0; i < enemyCount; i++) {
                    if (enemies[i]->isAlive() &&
                        enemies[i]->getBounds().intersects(playerBounds) &&
                        !isJumpingOrRolling) {

                        selectedPlayer->reduceHealth();
                        cout << "Player damaged by enemy!" << endl;

                        enemyDamageCooldown.restart();
                        break;

                    }
                }
            }

            if (selectedPlayer->getHealth() <= 0) {
                cout << "Game Over! Player was defeated by enemy.\n";
                return;
            }


            if (isPlayerNearBreakableWall(sonic, lvl) || isPlayerNearBreakableWall(tails, lvl) || isPlayerNearBreakableWall(knuckles, lvl)) {
                int tileX = (int)(knuckles.getX()) / cell_size;
                int tileY = (int)(knuckles.getY()) / cell_size;

                for (int i = tileY - 1; i <= tileY + 1; i++) {
                    for (int j = tileX - 1; j <= tileX + 1; j++) {
                        if (i >= 0 && i < height && j >= 0 && j < width && lvl[i][j] == 'b') {
                            lvl[i][j] = '\0';

                        }
                    }
                }
            }
            static Clock damageCooldown;
            for (int i = 0; i < obstacleCount; i++) {
                Spike* spike = dynamic_cast<Spike*>(obstacles[i]);
                if (spike != nullptr && selectedPlayer->getActiveStatus()) {
                    if (spike->checkCollision(selectedPlayer->getX(), selectedPlayer->getY(), Pwidth, Pheight)) {
                        if (damageCooldown.getElapsedTime().asSeconds() > 2) {
                            selectedPlayer->reduceHealth();
                            damageCooldown.restart();

                            if (selectedPlayer->getHealth() <= 0) {
                                cout << "Game Over! Player was defeated by obstacles.\n";
                                return;
                            }
                        }
                    }
                }
            }



            float fallThreshold = screen_y + 50;

            if (selectedPlayer->getY() > fallThreshold) {
                cout << "Game Over! Player fell into a pit." << endl;
                return;
            }


            if (&sonic != selectedPlayer && sonic.getY() > fallThreshold) {
                sonic.setX(selectedPlayer->getX() - 50);
                sonic.setY(selectedPlayer->getY() - 100);
            }
            if (&tails != selectedPlayer && tails.getY() > fallThreshold) {
                tails.setX(selectedPlayer->getX() + 50);
                tails.setY(selectedPlayer->getY() - 100);
            }
            if (&knuckles != selectedPlayer && knuckles.getY() > fallThreshold) {
                knuckles.setX(selectedPlayer->getX() + 100);
                knuckles.setY(selectedPlayer->getY() - 100);
            }



            playerBounds = selectedPlayer->getSprite().getGlobalBounds();

            for (int i = 0; i < ringCount; i++) {
                rings[i].update(deltaTime);
                if (rings[i].checkCollision(playerBounds)) {
                    collectedRings++;
                    score += 10;
                }
            }

            for (int i = 0; i < extraLifeCount; i++) {
                extraLives[i].update(deltaTime);
                extraLives[i].checkCollision(playerBounds);
            }
            for (int i = 0; i < boostCount; i++) {
                boosts[i].update(deltaTime);
                boosts[i].checkCollision(playerBounds, selectedPlayer);
            }



            window.clear();


            int bgWidth = bgTex.getSize().x;
            int bgRepeats = screen_x / bgWidth + 2;
            for (int i = 0; i < bgRepeats; ++i) {
                background.setPosition(i * bgWidth - (int)cameraOffset % bgWidth, 0);
                window.draw(background);
            }

            display_level(window, height, width, lvl, wallSprite, platformSprite, breakWallSprite, cell_size);


            for (int i = 0; i < obstacleCount; i++) {
                obstacles[i]->draw(window, cameraOffset);
            }

            for (int i = 0; i < extraLifeCount; i++) {
                extraLives[i].draw(window);
            }
            for (int i = 0; i < boostCount; i++) {
                boosts[i].draw(window);
            }
            sonic.draw(window);
            tails.draw(window);
            knuckles.draw(window);
            for (int i = 0; i < enemyCount; i++) {
                enemies[i]->draw(window, cameraOffset, selectedPlayer->getX());
            }


            CircleShape selector(10);
            selector.setFillColor(Color::Red);
            selector.setPosition(selectedPlayer->getX() - cameraOffset + 20, selectedPlayer->getY() - 20);
            window.draw(selector);


            for (int i = 0; i < obstacleCount; i++) {
                Spike* spike = dynamic_cast<Spike*>(obstacles[i]);
                if (spike != nullptr && spike->checkCollision(selectedPlayer->getX(), selectedPlayer->getY(), Pwidth, Pheight)) {
                    static Clock damageCooldown;
                    if (damageCooldown.getElapsedTime().asSeconds() > 2) {
                        selectedPlayer->reduceHealth();
                        damageCooldown.restart();
                    }
                }
            }

            for (int i = 0; i < ringCount; i++) {
                rings[i].draw(window);
            }

            if (collectedRings == ringCount) {
                cout << "Level Complete!" << endl;
                return;
            }

            RectangleShape hudBox(Vector2f(250, 110));

            hudBox.setFillColor(Color(0, 0, 0, 100));
            hudBox.setPosition(10, 10);
            window.draw(hudBox);

            ringText.setPosition(30, 30);
            ringText.setFillColor(Color::Yellow);
            ringText.setString("RINGS: " + to_string(collectedRings));

            scoreText.setPosition(30, 60);
            scoreText.setFillColor(Color::White);
            scoreText.setString("SCORE: " + to_string(score));
            livesText.setPosition(30, 90);
            livesText.setString("LIVES: " + to_string(selectedPlayer->getHealth()));


            window.draw(ringText);
            window.draw(scoreText);
            window.draw(livesText);

            if (selectedPlayer->isBoosted && selectedPlayer->boostClock.getElapsedTime().asSeconds() > 15) {
                selectedPlayer->isBoosted = false;
                if (Sonic* sonic = dynamic_cast<Sonic*>(selectedPlayer))
                    sonic->setSpeed(selectedPlayer->originalSpeed);
                else if (Knuckles* k = dynamic_cast<Knuckles*>(selectedPlayer))
                    selectedPlayer->isInvincible = false;
            }


            window.display();

        }
    }
    float getPlayerX() const { return selectedPlayer->getX(); }
    float getPlayerY() const { return selectedPlayer->getY(); }
    int getRingCount() const { return collectedRings; }
    int getEnemiesDefeated() const {
        int count = 0;
        for (int i = 0; i < enemyCount; i++)
            if (!enemies[i]->isAlive()) count++;
        return count;
    }
    string getSelectedCharacterName() const {
        if (selectedPlayer == &sonic) return "Sonic";
        if (selectedPlayer == &tails) return "Tails";
        return "Knuckles";
    }


    int run(RenderWindow& window) {
        update(window);
        return score;
    }
};




////////////////////////bosslevel implementations:////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Improved EggStinger and BossLevel

class EggStinger : public FlyingEnemy {
private:
    Clock attackTimer, retreatTimer;
    bool attacking = false, retreating = false;
    float attackSpeedY = 5.0f;
    float retreatSpeedY = 4.0f;
    int platformRow = 11;
    char** lvl;
    float initialY;
    int leftBoundary, rightBoundary;
    float lockedAttackX = 0.0f;




public:
    void load(const string& file, float px, float py, int health, FloatRect section, char** levelGrid) {
        FlyingEnemy::load(file, px, py, health, section);
        lvl = levelGrid;
        leftBoundary = 5 * cell_size;
        rightBoundary = 15 * cell_size;
        speedX = 3.0f; // increase horizontal speed
    }


    void update(float deltaTime, FloatRect playerBounds, bool isPlayerJumpingOrRolling, Player* player) override {
        if (!alive) return;

        float playerCenterX = playerBounds.left + playerBounds.width / 2;
        float targetY = platformRow * cell_size - sprite.getGlobalBounds().height;

        if (!attacking && !retreating) {

            x += direction * speedX;
            if (x < leftBoundary || x + sprite.getGlobalBounds().width > rightBoundary) {
                direction *= -1;
            }

            if (attackTimer.getElapsedTime().asSeconds() >= 10.0f) {

                float playerCenterX = playerBounds.left + playerBounds.width / 2;
                lockedAttackX = playerCenterX - sprite.getGlobalBounds().width / 2;

                attacking = true;
                attackTimer.restart();
            }
        }
        else if (attacking) {

            if (abs(x - lockedAttackX) > 3.0f) {
                float dir = (lockedAttackX > x) ? 1.0f : -1.0f;
                x += dir * speedX;
            }
            else if (y < platformRow * cell_size - sprite.getGlobalBounds().height) {
                y += attackSpeedY;
            }
            else {
                int col = static_cast<int>(x / cell_size);
                if (col >= 0 && col < 20 && lvl[platformRow][col] == 'w') {
                    lvl[platformRow][col] = '\0';
                }

                retreating = true;
                attacking = false;
            }
        }
        else if (retreating) {
            if (y > initialY) {
                y -= retreatSpeedY;
            }
            else {
                y = initialY;
                retreating = false;
            }
        }


        if (checkCollision(playerBounds, isPlayerJumpingOrRolling, player)) {
            if (!alive) {
                cout << "Victory! Egg Stinger defeated!" << endl;
                exit(0);
            }
        }

    }
    bool checkCollision(FloatRect playerBounds, bool isPlayerJumpingOrRolling, Player* player) override {
        if (!alive || !sprite.getGlobalBounds().intersects(playerBounds))
            return false;

        if (isPlayerJumpingOrRolling) {
            hp--;
            if (hp <= 0) alive = false;
            return true;
        }
        else {
            static Clock damageCooldown;
            if (damageCooldown.getElapsedTime().asSeconds() > 2) {
                player->reduceHealth();
                damageCooldown.restart();
            }
        }
        return false;
    }

    int getHP() const {
        return hp;
    }
};

class BossLevel : public Level {
private:
    EggStinger* boss;
    Clock deltaClock;
    Clock gameTimer;
    Font font;
    Text infoText;

public:
    BossLevel() : Level(14, 20) {
        selectedPlayer = &sonic;
        sonic.setX(400);
        sonic.setY(600);
        gravity = 1.0f;
        acceleration = 0.5f;
        friction = 0.5f;
        jumpStrength = -20.0f;
        initializeLevelGrid();
        loadAssets();
        generateLayout();
    }

    void initializeLevelGrid() {
        for (int i = 0; i < height; i++)
            for (int j = 0; j < width; j++)
                lvl[i][j] = '\0';

        for (int j = 5; j < 15; j++)
            lvl[11][j] = 'w';
    }

    void loadAssets() {
        wallTex.loadFromFile("Data/brick1.png");
        wallSprite.setTexture(wallTex);

        bgTex.loadFromFile("Data/bossbg2.png");
        background.setTexture(bgTex);

        font.loadFromFile("Data/arial.ttf");
        infoText.setFont(font);
        infoText.setCharacterSize(24);
        infoText.setFillColor(Color::White);
        infoText.setPosition(20, 20);
    }

    void generateLayout() {
        boss = new EggStinger();
        boss->load("Data/EggStinger.png", 400, 100, 15, FloatRect(0, 0, screen_x, screen_y), lvl);
        enemies[enemyCount++] = boss;
    }

    void update(RenderWindow& window) {

        Music bgMusic;
        if (!bgMusic.openFromFile("Data/Music3.ogg.opus")) {
            cout << "Could not load background music" << endl;
        }
        bgMusic.setLoop(true);
        bgMusic.play();


        window.setFramerateLimit(60);
        Event ev;

        float terminal_Velocity = 20;
        int hit_box_factor_x = 10, hit_box_factor_y = 10;
        int Pheight = 80, Pwidth = 60;

        gameTimer.restart();

        while (window.isOpen()) {
            while (window.pollEvent(ev)) {
                if (ev.type == Event::Closed)
                    window.close();
            }

            float elapsedTime = gameTimer.getElapsedTime().asSeconds();
            if (elapsedTime >= 300) {
                cout << "Time's up! Game Over." << endl;
                return;
            }

            sonic.move(jumpStrength, acceleration, friction);
            sonic.applyGravity(lvl, hit_box_factor_x, hit_box_factor_y, Pheight, Pwidth, gravity, terminal_Velocity);

            FloatRect playerBounds = sonic.getSprite().getGlobalBounds();
            bool isJumping = Keyboard::isKeyPressed(Keyboard::Up);
            float deltaTime = deltaClock.restart().asSeconds();

            if (boss->isAlive())
                boss->update(deltaTime, playerBounds, isJumping, &sonic);

            if (sonic.getY() > screen_y + 50 || sonic.getHealth() <= 0) {
                cout << "Game Over! You lost." << endl;
                return;
            }

            if (!boss->isAlive()) {
                cout << "Victory! Egg Stinger defeated!" << endl;
                return;
            }

            window.clear();

            background.setPosition(0, 0);
            window.draw(background);

            display_level(window, height, width, lvl, wallSprite, platformSprite, breakWallSprite, cell_size);
            sonic.draw(window);
            if (boss->isAlive()) boss->draw(window, 0, 0);

            infoText.setString("HP: " + to_string(sonic.getHealth()) + "   Boss HP: " + to_string(boss->getHP()) +
                "   Time Left: " + to_string(300 - static_cast<int>(elapsedTime)) + "s");
            window.draw(infoText);

            window.display();
        }
    }

    ~BossLevel() {
        for (int i = 0; i < enemyCount; i++) delete enemies[i];
    }

    void run(RenderWindow& window) {
        update(window);
    }

};


///////////////eggstinger finish///////////////////
// 
class Menu {
private:
    Font font;
    Text title;
    Text options[6];
    int selectedIndex;

public:
    Menu() {
        font.loadFromFile("Data/arial.ttf");

        title.setFont(font);
        title.setString("SONIC GAME");
        title.setCharacterSize(60);
        title.setFillColor(Color::Yellow);
        title.setPosition(400, 100);

        string labels[6] = {
            "1. Start Level 1",
            "2. Start Level 2",
            "3. Start Level 3",
            "4. Start Boss Level",
            "5. View High Scores",
            "6. Save Game Summary"
        };

        for (int i = 0; i < 6; i++) {
            options[i].setFont(font);
            options[i].setString(labels[i]);
            options[i].setCharacterSize(36);
            options[i].setPosition(420, 220 + i * 60);
            options[i].setFillColor(Color::White);
        }

        selectedIndex = 0;
    }



    int run(RenderWindow& window) {
        while (window.isOpen()) {
            Event event;
            while (window.pollEvent(event)) {
                if (event.type == Event::Closed)
                    window.close();

                if (event.type == Event::KeyPressed) {
                    if (event.key.code == Keyboard::Num1) return 1;
                    if (event.key.code == Keyboard::Num2) return 2;
                    if (event.key.code == Keyboard::Num3) return 3;
                    if (event.key.code == Keyboard::Num4) return 4;
                    if (event.key.code == Keyboard::Num5) return 5;
                    if (event.key.code == Keyboard::Num6) return 6;

                }
            }

            window.clear(Color::Black);
            window.draw(title);
            for (int i = 0; i < 6; i++) window.draw(options[i]);
            window.display();
        }
        return 0;
    }
};

class Highscore {
private:
    static const int MAX_HIGH_SCORES = 10;
    static const int NAME_LENGTH = 20;
    char highScoreNames[MAX_HIGH_SCORES][NAME_LENGTH];
    int highScoreScores[MAX_HIGH_SCORES];

public:
    void loadHighScores() {
        ifstream file("highscores.txt");
        if (!file.is_open()) {
            for (int i = 0; i < MAX_HIGH_SCORES; i++) {
                highScoreScores[i] = 0;
                for (int j = 0; j < NAME_LENGTH; j++)
                    highScoreNames[i][j] = '\0';
            }
            return;
        }

        int index = 0;
        while (index < MAX_HIGH_SCORES && file >> highScoreNames[index] >> highScoreScores[index])
            index++;

        file.close();
    }

    void saveHighScores() {
        ofstream file("highscores.txt");
        if (!file.is_open()) {
            return;
        }

        for (int i = 0; i < MAX_HIGH_SCORES; i++) {
            if (highScoreScores[i] > 0) {
                file << highScoreNames[i] << " " << highScoreScores[i] << endl;

            }
        }
    }

    void updateHighScores(const char playerName[], int playerScore) {
        highScoreScores[MAX_HIGH_SCORES - 1] = playerScore;

        for (int i = 0; i < NAME_LENGTH - 1; ++i) {
            highScoreNames[MAX_HIGH_SCORES - 1][i] = playerName[i];
            if (playerName[i] == '\0')
                break;
        }
        highScoreNames[MAX_HIGH_SCORES - 1][NAME_LENGTH - 1] = '\0';

        for (int i = 0; i < MAX_HIGH_SCORES - 1; ++i) {
            for (int j = i + 1; j < MAX_HIGH_SCORES; ++j) {
                if (highScoreScores[i] < highScoreScores[j]) {
                    int tempScore = highScoreScores[i];
                    highScoreScores[i] = highScoreScores[j];
                    highScoreScores[j] = tempScore;

                    char temp[NAME_LENGTH];
                    for (int k = 0; k < NAME_LENGTH - 1; ++k) {
                        temp[k] = highScoreNames[i][k];
                        if (highScoreNames[i][k] == '\0') break;
                    }
                    temp[NAME_LENGTH - 1] = '\0';

                    for (int k = 0; k < NAME_LENGTH - 1; ++k) {
                        highScoreNames[i][k] = highScoreNames[j][k];
                        if (highScoreNames[j][k] == '\0') break;
                    }
                    highScoreNames[i][NAME_LENGTH - 1] = '\0';

                    for (int k = 0; k < NAME_LENGTH - 1; ++k) {
                        highScoreNames[j][k] = temp[k];
                        if (temp[k] == '\0') break;
                    }
                    highScoreNames[j][NAME_LENGTH - 1] = '\0';
                }
            }
        }

        highScoreScores[MAX_HIGH_SCORES - 1] = 0;
        highScoreNames[MAX_HIGH_SCORES - 1][0] = '\0';
    }





    void getPlayerName(char* playerName, RenderWindow& window, Font& font) {
        window.clear();
        Text prompt("Enter your name:", font, 30);
        prompt.setPosition(300, 200);
        prompt.setFillColor(Color::White);
        window.draw(prompt);
        window.display();

        char input[NAME_LENGTH] = { 0 };
        int length = 0;
        Event event;

        while (true) {
            while (window.pollEvent(event)) {
                if (event.type == Event::TextEntered && length < NAME_LENGTH - 1) {
                    if (event.text.unicode < 128 && isprint(event.text.unicode)) {
                        input[length++] = static_cast<char>(event.text.unicode);
                        input[length] = '\0';
                    }
                }
                if (event.type == Event::KeyPressed) {
                    if (event.key.code == Keyboard::Enter && length > 0) {
                        for (int i = 0; i < NAME_LENGTH - 1; ++i) {
                            playerName[i] = input[i];
                            if (input[i] == '\0') break;
                        }
                        playerName[NAME_LENGTH - 1] = '\0';
                        return;
                    }
                    if (event.key.code == Keyboard::BackSpace && length > 0) {
                        input[--length] = '\0';
                    }
                }
            }

            window.clear();
            window.draw(prompt);
            Text name(input, font, 30);
            name.setPosition(300, 250);
            name.setFillColor(Color::White);
            window.draw(name);
            window.display();
        }
    }




    void displayHighScores(RenderWindow& window, Font& font, const char playerName[], int playerScore) {
        window.clear();
        Text title("High Scores", font, 40);
        title.setPosition(300, 50);
        title.setFillColor(Color::Yellow);
        window.draw(title);

        for (int i = 0; i < MAX_HIGH_SCORES; ++i) {
            if (highScoreScores[i] == 0)
                continue;

            string line = to_string(i + 1) + ". " + highScoreNames[i] + " - " + to_string(highScoreScores[i]);
            Text entry(line, font, 30);
            entry.setPosition(200, 150 + i * 40);

            if (highScoreScores[i] == playerScore && string(highScoreNames[i]) == playerName)
                entry.setFillColor(Color::Green);
            else
                entry.setFillColor(Color::White);

            window.draw(entry);
        }

        Text back("Press any key to return to the menu", font, 20);
        back.setPosition(200, 600);
        back.setFillColor(Color::White);
        window.draw(back);

        window.display();

        Event event;
        while (true) {
            while (window.pollEvent(event)) {
                if (event.type == Event::KeyPressed) return;
            }
        }
    }
};



void saveGameSummary(const string& name, const string& character, float x, float y, int hp, int score, int rings, int enemies) {
    ofstream file("GameSummary.txt", ios::app); // append mode
    if (file.is_open()) {
        file << "Player Name: " << name << endl;
        file << "Character: " << character << endl;
        file << "Position: " << (int)x << " " << (int)y << endl;
        file << "HP: " << hp << endl;
        file << "Score: " << score << endl;
        file << "Rings Collected: " << rings << endl;
        file << "Enemies Defeated: " << enemies << endl;
        file << "------------------------------" << endl;
        file.close();
        cout << "Game Summary Saved.\n";
    }
}
void displayGameSummary(RenderWindow& window, Font& font) {
    ifstream file("GameSummary.txt");
    if (!file.is_open()) return;

    const int MAX_LINES = 100; // limit number of lines to display
    Text lines[MAX_LINES];
    string line;
    int lineCount = 0;
    int y = 100;

    while (getline(file, line) && lineCount < MAX_LINES) {
        lines[lineCount].setFont(font);
        lines[lineCount].setCharacterSize(24);
        lines[lineCount].setFillColor(Color::White);
        lines[lineCount].setString(line);
        lines[lineCount].setPosition(100, y);
        y += 30;
        lineCount++;
    }

    file.close();

    while (window.isOpen()) {
        Event event;
        while (window.pollEvent(event)) {
            if (event.type == Event::Closed ||
                (event.type == Event::KeyPressed && event.key.code == Keyboard::Escape)) {
                return;
            }
        }

        window.clear(Color::Black);
        for (int i = 0; i < lineCount; i++) {
            window.draw(lines[i]);
        }
        window.display();
    }
}




//======================================================================================================================================================
//=============================================== MAINN=================================================================================================




int main() {
    RenderWindow window(VideoMode(screen_x, screen_y), "Sonic Game");
    window.setFramerateLimit(60);

    Font font;
    font.loadFromFile("Data/arial.ttf");

    Highscore highscoreManager;
    highscoreManager.loadHighScores();

    while (window.isOpen()) {
        Menu menu;
        int option = menu.run(window);

        if (option == 1) {
            char playerName[20];
            highscoreManager.getPlayerName(playerName, window, font);
            Level1 level1;
            int score = level1.run(window);

            highscoreManager.updateHighScores(playerName, score);
            highscoreManager.saveHighScores();
            highscoreManager.displayHighScores(window, font, playerName, score);

            saveGameSummary(playerName, level1.getSelectedCharacterName(),
                level1.getPlayerX(), level1.getPlayerY(),
                Player::sharedHealth, score,
                level1.getRingCount(), level1.getEnemiesDefeated());
        }

        else if (option == 2) {


             char playerName[20];
             highscoreManager.getPlayerName(playerName, window, font);
             Level2 level2;
             int score = level2.run(window);

             highscoreManager.updateHighScores(playerName, score);
             highscoreManager.saveHighScores();
             highscoreManager.displayHighScores(window, font, playerName, score);

             saveGameSummary(playerName, level2.getSelectedCharacterName(),
                 level2.getPlayerX(), level2.getPlayerY(),
                 Player::sharedHealth, score,
                 level2.getRingCount(), level2.getEnemiesDefeated());

        }

        else if (option == 3) {
          /*  char playerName[20];
            highscoreManager.getPlayerName(playerName, window, font);
            Level3 level3;
            int score = level3.run(window);

            highscoreManager.updateHighScores(playerName, score);
            highscoreManager.saveHighScores();
            highscoreManager.displayHighScores(window, font, playerName, score);

            saveGameSummary(playerName, level3.getSelectedCharacterName(),
                level3.getPlayerX(), level3.getPlayerY(),
                Player::sharedHealth, score,
                level3.getRingCount(), level3.getEnemiesDefeated());*/
        }

        else if (option == 4) {
            char playerName[20];
            highscoreManager.getPlayerName(playerName, window, font);
            BossLevel boss;
            boss.run(window);
        }

        else if (option == 5) {
            highscoreManager.displayHighScores(window, font, "", -1);
        }

        else if (option == 6) {
            displayGameSummary(window, font);
        }
    }

    return 0;
}
