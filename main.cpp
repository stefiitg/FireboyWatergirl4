
#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable : 4996) // MSVC: disable warnings for deprecated funcs
#endif
#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <sstream>
#include <cassert>

#if defined(__clang__)
    #pragma clang diagnostic pop
#elif defined(_MSC_VER)
    #pragma warning(pop)
#endif

using std::string;
using std::vector;
using std::cout;
using std::endl;


enum class TileType { Empty, Solid, Fire, Water, ExitFire, ExitWater };

static string toString(TileType t) {
    switch (t) {
        case TileType::Empty: return "Empty";
        case TileType::Solid: return "Solid";
        case TileType::Fire: return "Fire";
        case TileType::Water: return "Water";
        case TileType::ExitFire: return "ExitFire";
        case TileType::ExitWater: return "ExitWater";
    }
    return "Unknown";
}

class Tile {
private:
    TileType type;
    sf::RectangleShape rect; // visual representation (fallback)
    int gx, gy; // grid coordinates (col,row)
    static constexpr float TILE_SIZE = 48.f;

    void initShape() {
        rect.setSize({TILE_SIZE, TILE_SIZE});
        rect.setPosition(gx * TILE_SIZE, gy * TILE_SIZE);
        switch (type) {
            case TileType::Empty: rect.setFillColor(sf::Color(80,80,80)); break;
            case TileType::Solid: rect.setFillColor(sf::Color(120,120,120)); break;
            case TileType::Fire: rect.setFillColor(sf::Color::Red); break;
            case TileType::Water: rect.setFillColor(sf::Color::Blue); break;
            case TileType::ExitFire: rect.setFillColor(sf::Color(255,140,0)); break;
            case TileType::ExitWater: rect.setFillColor(sf::Color(0,200,100)); break;
        }
    }

public:
    // constructor parametric
    explicit Tile(TileType t , int col , int row )
        : type(t), gx(col), gy(row) {
        initShape();
    }

    // copy / assign default are fine
    Tile(const Tile& other) = default;
    Tile& operator=(const Tile& other) = default;
    ~Tile() = default;

    TileType getType() const { return type; }
    int getCol() const { return gx; }
    int getRow() const { return gy; }
    static float getSize() { return TILE_SIZE; }

    // draw (const)
    void draw(sf::RenderTarget& target) const {
        if (type != TileType::Empty) target.draw(rect);
        else {
            // draw a faint empty tile if you want
            // target.draw(rect);
        }
    }

    // get bounding box in world coords
    sf::FloatRect bounds() const {
        return rect.getGlobalBounds();
    }

    friend std::ostream& operator<<(std::ostream& os, const Tile& t) {
        os << "[" << toString(t.type) << " @" << t.gx << "," << t.gy << "]";
        return os;
    }
};

// -------------------------------
// Character
// -------------------------------
class Character {
private:
    string name;
    sf::Texture texture;
    sf::Sprite sprite;
    sf::RectangleShape fallbackShape;
    bool usingTexture;
    sf::Vector2f position; // world coordinates (top-left)
    sf::Vector2f velocity;
    int lives;
    bool onGround;

    float speed = 160.f; // px/s
    float jumpImpulse = 360.f; // initial jump velocity
    static constexpr float GRAVITY = 900.f;

    void initFallbackShape(const sf::Color& c, const sf::Vector2f& size) {
        fallbackShape.setSize(size);
        fallbackShape.setFillColor(c);
        fallbackShape.setPosition(position);
    }

public:

    // constructor parametric
    Character(const string& nm, const string& texturePath,
          const sf::Vector2f& pos = {0.f,0.f}, int lifeCount = 3,
          const sf::Color& fallbackColor = sf::Color::White)
    : name(nm), usingTexture(false), position(pos), velocity(0.f, 0.f),
      lives(lifeCount), onGround(false)
    {
        // try load texture (if not found, use fallback rectangle)
        if (!texturePath.empty() && texture.loadFromFile(texturePath)) {
            usingTexture = true;
            sprite.setTexture(texture);
            // scale sprite to tile size
            float factor = Tile::getSize() / texture.getSize().y;
            sprite.setScale(factor, factor);
            sprite.setPosition(position);
        } else {
            usingTexture = false;
            initFallbackShape(fallbackColor, {Tile::getSize(), Tile::getSize()});
        }
    }

    //get on ground state
    void setOnGround(bool state) {
         onGround=state;
    }
    // copy constructor (default is ok but implement explicitly to satisfy assignment)
    Character (const Character& other)
        : name(other.name),
          texture(other.texture),
          sprite(other.sprite),
          fallbackShape(other.fallbackShape),
          usingTexture(other.usingTexture),
          position(other.position),
          velocity(other.velocity),
          lives(other.lives),
          onGround(other.onGround),
          speed(other.speed),
          jumpImpulse(other.jumpImpulse)
    {
        if (usingTexture) sprite.setTexture(texture);
    }

    Character& operator=(const Character& other) {
        if (this == &other) return *this;
        name = other.name;
        texture = other.texture;
        sprite = other.sprite;
        fallbackShape = other.fallbackShape;
        usingTexture = other.usingTexture;
        position = other.position;
        velocity = other.velocity;
        lives = other.lives;
        onGround = other.onGround;
        speed = other.speed;
        jumpImpulse = other.jumpImpulse;
        if (usingTexture) sprite.setTexture(texture);
        return *this;
    }

    ~Character() = default;

    // getters
    const string& getName() const { return name; }
    int getLives() const { return lives; }
    sf::Vector2f getPosition() const { return position; }

    // bounding box for collisions
    sf::FloatRect bounds() const {
        if (usingTexture) return sprite.getGlobalBounds();
        return fallbackShape.getGlobalBounds();
    }

    // set position (useful for respawn)
    void setPosition(const sf::Vector2f& p) {
        position = p;
        if (usingTexture) sprite.setPosition(position);
        else fallbackShape.setPosition(position);
    }

    // A public complex function: update physics, apply gravity, integrate velocity
    // returns true if something significant changed (for debugging)
    bool update(float dt, const sf::FloatRect& worldBounds) {
        // gravity
        velocity.y += GRAVITY * dt;
        // integrate
        position += velocity * dt;

        // simple floor collision with world bounds bottom
        if (position.y + Tile::getSize() > worldBounds.top + worldBounds.height) {
            position.y = worldBounds.top + worldBounds.height - Tile::getSize();
            velocity.y = 0.f;
            onGround = true;
        } else {
            onGround = false;
        }

        // prevent leaving world horizontally
        if (position.x < worldBounds.left) position.x = worldBounds.left;
        if (position.x + Tile::getSize() > worldBounds.left + worldBounds.width)
            position.x = worldBounds.left + worldBounds.width - Tile::getSize();

        // update visual
        if (usingTexture) sprite.setPosition(position);
        else fallbackShape.setPosition(position);

        return true;
    }

    // move left/right - uses dt multiplier outside or inside; we provide velocity-based per-call
    void moveLeft(float dt) { position.x -= speed * dt; if (usingTexture) sprite.setPosition(position); else fallbackShape.setPosition(position); }
    void moveRight(float dt) { position.x += speed * dt; if (usingTexture) sprite.setPosition(position); else fallbackShape.setPosition(position); }

    // jump: complex (only if on ground)
    void jump() {
        if (onGround) {
            velocity.y = -jumpImpulse;
            onGround = false;
        }
    }

    // apply damage & respawn
    void takeDamageAndRespawn(const sf::Vector2f& respawnPos) {
        if (lives > 0) --lives;
        setPosition(respawnPos);
        velocity = {0.f, 0.f};
        onGround = false;
    }

    // draw
    void draw(sf::RenderTarget& target) const {
        if (usingTexture) target.draw(sprite);
        else target.draw(fallbackShape);
    }

    friend std::ostream& operator<<(std::ostream& os, const Character& c) {
        os << c.name << " pos=(" << (int)c.position.x << "," << (int)c.position.y << ") lives=" << c.lives;
        return os;
    }

    // helper to set fallback color size (used at construction or later)
    void setFallbackAppearance(const sf::Color& c) {
        fallbackShape.setFillColor(c);
        fallbackShape.setSize({Tile::getSize(), Tile::getSize()});
        fallbackShape.setPosition(position);
    }
};

// -------------------------------
// Map (contains Tiles) - implement copy ctor/operator=/destructor explicitly
// -------------------------------
class Map {
private:
    vector<vector<Tile>> grid;
    int width, height;

    // helper to create grid
    void allocateGrid(int w, int h, TileType defaultType = TileType::Empty) {
        width = w; height = h;
        grid.assign(height, vector<Tile>(width, Tile(defaultType, 0, 0)));
        for (int r = 0; r < height; ++r)
            for (int c = 0; c < width; ++c)
                grid[r][c] = Tile(defaultType, c, r);
    }

public:
    // constructor parametric
    Map(int w = 12, int h = 8, TileType defaultType = TileType::Empty) {
        allocateGrid(w,h,defaultType);
    }

    // copy constructor (explicit)
    Map(const Map& other) {
        width = other.width;
        height = other.height;
        grid.reserve(height);
        for (int r = 0; r < height; ++r) {
            vector<Tile> row;
            row.reserve(width);
            for (int c = 0; c < width; ++c) row.push_back(other.grid[r][c]);
            grid.push_back(std::move(row));
        }
    }

    // copy assignment
    Map& operator=(const Map& other) {
        if (this == &other) return *this;
        width = other.width;
        height = other.height;
        grid.clear();
        grid.reserve(height);
        for (int r = 0; r < height; ++r) {
            vector<Tile> row;
            row.reserve(width);
            for (int c = 0; c < width; ++c) row.push_back(other.grid[r][c]);
            grid.push_back(std::move(row));
        }
        return *this;
    }

    // destructor
    ~Map() {
        // No raw pointers, but destructor explicitly present to satisfy requirement.
    }

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    // complex public function: generate ascending platforms randomly
    // "ascending" -> platforms generally going upward from left to right
    void generateAscendingPlatforms(unsigned seed = 0) {
        // clear first
        for (int r=0;r<height;++r)
            for (int c=0;c<width;++c)
                grid[r][c] = Tile(TileType::Empty, c, r);

        std::mt19937 rng((seed==0)? std::random_device{}() : seed);
        std::uniform_int_distribution<int> gapDist(1,3);
        std::uniform_int_distribution<int> lengthDist(1,3);

        int currentRow = height - 2; // start near bottom
        int c = 1;
        while (c < width-1 && currentRow > 0) {
            int len = lengthDist(rng);
            for (int k = 0; k < len && c < width-1; ++k) {
                grid[currentRow][c] = Tile(TileType::Solid, c, currentRow);
                ++c;
            }
            int gap = gapDist(rng);
            c += gap;
            // ascend sometimes
            if (currentRow > 1) currentRow -= 1;
        }

        // create some special tiles: fire patch and water patch in different places
        // ensure they are on top of solids or on their own row
        grid[height-2][2] = Tile(TileType::Fire, 2, height-2);
        grid[height-3][width-3] = Tile(TileType::Water, width-3, height-3);

        // exits: place exit for Fireboy (ExitFire) and for Watergirl (ExitWater)
        // place Fire exit on top-right, Water exit on top-left (if available)
        grid[1][width-2] = Tile(TileType::ExitFire, width-2, 1);
        grid[1][1] = Tile(TileType::ExitWater, 1, 1);
    }
    /*void generateCustomPlatforms() {
        // Clear entire grid to Empty first
        for (int r = 0; r < height; ++r)
            for (int c = 0; c < width; ++c)
                grid[r][c] = Tile(TileType::Empty, c, r);

        // === PLASEAZĂ MANUAL TILE-URILE SOLID ===
        // Exemplu: platformă orizontală la baza hărții
        for (int c = 0; c < width; ++c) {
            grid[height-1][c] = Tile(TileType::Solid, c, height-1); // Platformă de jos
        }

        // Platforme verticale sau orizontale la poziții specifice
        grid[6][3] = Tile(TileType::Solid, 3, 6);
        grid[6][4] = Tile(TileType::Solid, 4, 6);
        grid[6][5] = Tile(TileType::Solid, 5, 6);

        grid[4][7] = Tile(TileType::Solid, 7, 4);
        grid[4][8] = Tile(TileType::Solid, 8, 4);
        grid[4][9] = Tile(TileType::Solid, 9, 4);

        grid[2][5] = Tile(TileType::Solid, 5, 2);
        grid[2][6] = Tile(TileType::Solid, 6, 2);

        // === ELEMENTE SPECIALE (păstrează-le sau modifică-le) ===
        grid[height-2][2] = Tile(TileType::Fire, 2, height-2);
        grid[height-3][width-3] = Tile(TileType::Water, width-3, height-3);
        grid[1][width-2] = Tile(TileType::ExitFire, width-2, 1);
        grid[1][1] = Tile(TileType::ExitWater, 1, 1);
    }*/

    // get tile type at world coords (x,y in pixels) OR by grid coords
    TileType getTileTypeAtGrid(int col, int row) const {
        if (col < 0 || col >= width || row < 0 || row >= height) return TileType::Solid;
        return grid[row][col].getType();
    }

    // get tile type by world position
    TileType getTileTypeAtWorld(float x, float y) const {
        int col = static_cast<int>(x / Tile::getSize());
        int row = static_cast<int>(y / Tile::getSize());
        return getTileTypeAtGrid(col, row);
    }

    // draw map
    void draw(sf::RenderTarget& target) const {
        for (int r = 0; r < height; ++r)
            for (int c = 0; c < width; ++c)
                grid[r][c].draw(target);
    }

    friend std::ostream& operator<<(std::ostream& os, const Map& m) {
        os << "Map " << m.width << "x" << m.height << "\n";
        for (int r = 0; r < m.height; ++r) {
            for (int c = 0; c < m.width; ++c) {
                os << m.grid[r][c];
            }
            os << "\n";
        }
        return os;
    }

    // get world bounds as rectangle
    sf::FloatRect worldBounds() const {
        return sf::FloatRect(0.f, 0.f, width * Tile::getSize(), height * Tile::getSize());
    }

    // find respawn positions for characters (grid col,row) -> world pos
    sf::Vector2f respawnWorldPosForFire() const {
        // left-bottom corner safe spot
        return sf::Vector2f(Tile::getSize() * 1.f, Tile::getSize() * (height - 2));
    }
    sf::Vector2f respawnWorldPosForWater() const {
        return sf::Vector2f(Tile::getSize() * 5.f, Tile::getSize() * (height - 2));
    }
};

// -------------------------------
// Utility collision helpers
// -------------------------------
static bool intersects(const sf::FloatRect& a, const sf::FloatRect& b) {
    return a.intersects(b);
}

// -------------------------------
// Game (manages everything)
// -------------------------------
class Game {
private:
    sf::RenderWindow window;
    Map map;
    Character fireboy;
    Character watergirl;
    bool fireboyAtExit = false;
    bool watergirlAtExit = false;
    bool won = false;
    sf::Font font;
    sf::Text winText;

    // private helpers
    void processInput(float dt) {
        // Fireboy controls: A D W
        if (!won) {
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) fireboy.moveLeft(dt);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) fireboy.moveRight(dt);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) fireboy.jump();

            // Watergirl controls: Left Right Up (arrows)
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::F)) watergirl.moveLeft(dt);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::H)) watergirl.moveRight(dt);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::T)) watergirl.jump();
        }
    }

    // collision handling with tiles: simple AABB sampling tiles around character
    void handleCollisions(Character& ch, const sf::Vector2f& respawnPos, bool& reachedExitForCharacter, TileType whatExit) {
        (void) whatExit;
        sf::FloatRect cb = ch.bounds();
        // sample nearby tiles (grid cells overlapping bounds)
        int leftCol = static_cast<int>(cb.left / Tile::getSize());
        int rightCol = static_cast<int>((cb.left + cb.width) / Tile::getSize());
        int topRow = static_cast<int>(cb.top / Tile::getSize());
        int bottomRow = static_cast<int>((cb.top + cb.height) / Tile::getSize());

        // clamp to map bounds
        leftCol = std::max(leftCol, 0);
        rightCol = std::min(rightCol, map.getWidth()-1);
        topRow = std::max(topRow, 0);
        bottomRow = std::min(bottomRow, map.getHeight()-1);
//haha
        // detect if standing on solid: if any tile directly below intersects
       // bool onSolid = false;
        for (int r = topRow; r <= bottomRow; ++r) {
            for (int c = leftCol; c <= rightCol; ++c) {
                TileType tt = map.getTileTypeAtGrid(c, r);
                if (tt == TileType::Solid) {
                    // compute tile bounds
                    sf::FloatRect tileRect(c * Tile::getSize(), r * Tile::getSize(), Tile::getSize(), Tile::getSize());
                    if (intersects(cb, tileRect)) {
                        // if intersecting vertically, push character above the tile
                        // simple resolution: if center of character is above center of tile -> put on top
                        float charCenterY = cb.top + cb.height * 0.5f;
                        float tileCenterY = tileRect.top + tileRect.height * 0.5f;
                        ch.setOnGround(true);
                        if (charCenterY < tileCenterY) {
                            // place character on top
                            ch.setPosition({cb.left, tileRect.top - cb.height});
                        } else {
                            // push below (rare)
                            ch.setPosition({cb.left, tileRect.top + tileRect.height});
                        }
                     //   onSolid = true;
                    }
                }
                // check hazardous tiles
                if (tt == TileType::Fire) {
                    // If character is Watergirl -> damage
                    if (ch.getName() == "Watergirl") {
                        ch.takeDamageAndRespawn(respawnPos);
                        reachedExitForCharacter = false;
                        return;
                    }
                } else if (tt == TileType::Water) {
                    if (ch.getName() == "Fireboy") {
                        ch.takeDamageAndRespawn(respawnPos);
                        reachedExitForCharacter = false;
                        return;
                    }
                } else if (tt == TileType::ExitFire && ch.getName() == "Fireboy") {
                    // check overlap => at exit
                    sf::FloatRect tileRect(c * Tile::getSize(), r * Tile::getSize(), Tile::getSize(), Tile::getSize());
                    if (intersects(cb, tileRect)) {
                        reachedExitForCharacter = true;
                    }
                } else if (tt == TileType::ExitWater && ch.getName() == "Watergirl") {
                    sf::FloatRect tileRect(c * Tile::getSize(), r * Tile::getSize(), Tile::getSize(), Tile::getSize());
                    if (intersects(cb, tileRect)) {
                        reachedExitForCharacter = true;
                    }
                }
            }
        }
        // if standing on solid, ensure vertical velocity reset (handled in Character::update by world bottom test, but here we do extra)
        // nothing more to do here
    }

    // update function that orchestrates characters + collisions
    void update(float dt) {
        if (won) return;

        sf::FloatRect world = map.worldBounds();
        // apply physics
        fireboy.update(dt, world);
        watergirl.update(dt, world);

        // collisions and interactions
        handleCollisions(fireboy, map.respawnWorldPosForFire(), fireboyAtExit, TileType::ExitFire);
        handleCollisions(watergirl, map.respawnWorldPosForWater(), watergirlAtExit, TileType::ExitWater);

        // check win
        if (fireboyAtExit && watergirlAtExit) {
            won = true;
            winText.setString("WIN");
            // center text
            sf::FloatRect tb = winText.getLocalBounds();
            winText.setOrigin(tb.left + tb.width/2.f, tb.top + tb.height/2.f);
            winText.setPosition(window.getSize().x/2.f, window.getSize().y/2.f - 20.f);
        }
    }

    void render() {
        window.clear(sf::Color(40,40,40));
        map.draw(window);
        fireboy.draw(window);
        watergirl.draw(window);
        if (won) window.draw(winText);
        window.display();
    }

public:
    // constructor parametric (loads map, characters)
    Game(int mapW = 14, int mapH = 9)
     : map(mapW, mapH),
       fireboy("Fireboy", "Desktop/fireboy.png", {Tile::getSize()*1.f, Tile::getSize()*(mapH-2.f)}, 3, sf::Color::Red),
       watergirl("Watergirl", "Desktop/watergirl.png", {Tile::getSize()*5.f, Tile::getSize()*(mapH-2.f)}, 3, sf::Color::Blue)
    {
        // === Detect if we're running in headless CI environment ===
        const char* disp = std::getenv("DISPLAY");
        bool headless = (disp == nullptr || std::string(disp).empty());
        if (headless) {
            cout << "[Info] Headless environment detected (no DISPLAY). SFML window will be skipped.\n";
        } else {
            window.create(sf::VideoMode(
                (unsigned)(mapW * Tile::getSize()),
                (unsigned)(mapH * Tile::getSize())),
                "Fireboy & Watergirl"
            );
        }

        // generate platforms
        map.generateAscendingPlatforms(/*seed*/ 12345);

        // load font
        if (!font.loadFromFile("Desktop/arial.ttf")) {
            cout << "Warning: font 'Desktop/arial.ttf' not found. Win text may not display correctly.\n";
        }
        winText.setFont(font);
        winText.setCharacterSize(48);
        winText.setFillColor(sf::Color::Yellow);
        winText.setStyle(sf::Text::Bold);

        // ensure fallback visuals
        fireboy.setFallbackAppearance(sf::Color::Red);
        watergirl.setFallbackAppearance(sf::Color::Blue);
    }

    // operator<< for Game (compositional)
    friend std::ostream& operator<<(std::ostream& os, const Game& g) {
        os << "Game state:\n";
        os << "Map: " << g.map.getWidth() << "x" << g.map.getHeight() << "\n";
        os << "Fireboy: " << g.fireboy << "\n";
        os << "Watergirl: " << g.watergirl << "\n";
        os << "Won: " << (g.won ? "true" : "false") << "\n";
        return os;
    }

    // public complex method: run the game loop
    void run() {
        sf::Clock clock;
        while (window.isOpen()) {
            sf::Event ev;
            while (window.pollEvent(ev)) {
                if (ev.type == sf::Event::Closed) window.close();
            }

            float dt = clock.restart().asSeconds();
            processInput(dt);
            update(dt);
            render();
        }
    }
};


int main() {
    // build a game with map dimensions (width, height)
    Game game(14, 9);

    // print initial state using operator<< (scenario of use)
    cout << game << endl;

    // run the game (this will open SFML window)
    game.run();

    return 0;
}
