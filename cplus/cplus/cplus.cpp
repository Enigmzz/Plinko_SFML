#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <string>

using namespace std;

constexpr int WIDTH = 800;
constexpr int HEIGHT = 750;
constexpr float GRAVITY = 1200.f;
constexpr float PEG_RADIUS = 7.f;
constexpr float BALL_RADIUS = 10.0f; // Rozmiar 10 zgodnie z prośbą

float length(sf::Vector2f v) { return sqrt(v.x * v.x + v.y * v.y); }

struct Peg {
    sf::CircleShape shape;
    float hitTimer = 0.f;
};

struct Ball {
    sf::CircleShape shape;
    sf::Vector2f vel = { 0.f, 0.f };
    float betAmount = 0.f;
    bool alive = true;
};

void updateViewport(sf::RenderWindow& window, sf::View& view) {
    float windowWidth = static_cast<float>(window.getSize().x);
    float windowHeight = static_cast<float>(window.getSize().y);
    float windowRatio = windowWidth / windowHeight;
    float viewRatio = static_cast<float>(WIDTH) / static_cast<float>(HEIGHT);
    float sizeX = 1.f, sizeY = 1.f, posX = 0.f, posY = 0.f;
    if (windowRatio > viewRatio) { sizeX = viewRatio / windowRatio; posX = (1.f - sizeX) / 2.f; }
    else { sizeY = windowRatio / viewRatio; posY = (1.f - sizeY) / 2.f; }
    view.setViewport(sf::FloatRect(sf::Vector2f(posX, posY), sf::Vector2f(sizeX, sizeY)));
}

int main() {
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(WIDTH, HEIGHT)), "Plinko", sf::Style::Default);
    window.setFramerateLimit(60);
    srand(static_cast<unsigned>(time(nullptr)));

    sf::View gameView(sf::FloatRect(sf::Vector2f(0.f, 0.f), sf::Vector2f((float)WIDTH, (float)HEIGHT)));

    sf::Font font;
    if (!font.openFromFile("arial.ttf")) {}

    float wallet = 1000.f;
    float currentBet = 10.f;
    float shotTimer = 0.f;
    const float SHOT_DELAY = 0.2f;

    // --- GEOMETRIA ---
    float spacingX = 55.f;
    float spacingY = spacingX * 0.866f;
    int rows = 12;
    float startY = 120.f;
    float tan30 = 0.577f;

    // PRZESUNIĘCIE ŚCIAN: -2.0f przybliża ściany do centrum
    float wallTopW = spacingX - 2.0f;

    float wallTopY = startY;
    float wallBottomY = startY + (float)(rows - 1) * spacingY;

    auto getWidthAtY = [&](float y) -> float {
        if (y < wallTopY) return wallTopW;
        return wallTopW + (y - wallTopY) * tan30;
        };

    float wallBotW = getWidthAtY(wallBottomY);

    // GENEROWANIE KOŁKÓW (Bez tych w ścianach)
    vector<Peg> pegs;
    for (int r = 0; r < rows; r++) {
        float rowY = startY + (float)r * spacingY;
        int n = r + 3;
        float rowStartX = (WIDTH / 2.f) - ((float)n - 1.f) * spacingX / 2.f;
        float currentWallW = getWidthAtY(rowY);

        for (int i = 0; i < n; i++) {
            float x = rowStartX + (float)i * spacingX;
            float distFromCenter = abs(x - WIDTH / 2.f);

            // Warunek usuwający kołki "wbite" w ściany
            if (distFromCenter < currentWallW - (BALL_RADIUS + 1.0f)) {
                Peg p;
                p.shape.setRadius(PEG_RADIUS);
                p.shape.setOrigin(sf::Vector2f(PEG_RADIUS, PEG_RADIUS));
                p.shape.setPosition(sf::Vector2f(x, rowY));
                p.shape.setFillColor(sf::Color(130, 130, 150));
                pegs.push_back(p);
            }
        }
    }

    // ŚCIANY
    sf::ConvexShape lWall(4), rWall(4);
    sf::Color wallCol(60, 60, 80);
    lWall.setFillColor(wallCol); rWall.setFillColor(wallCol);
    lWall.setPoint(0, sf::Vector2f(WIDTH / 2.f - wallTopW, wallTopY));
    lWall.setPoint(1, sf::Vector2f(WIDTH / 2.f - wallBotW, wallBottomY));
    lWall.setPoint(2, sf::Vector2f(WIDTH / 2.f - wallBotW - 15.f, wallBottomY));
    lWall.setPoint(3, sf::Vector2f(WIDTH / 2.f - wallTopW - 15.f, wallTopY));
    rWall.setPoint(0, sf::Vector2f(WIDTH / 2.f + wallTopW, wallTopY));
    rWall.setPoint(1, sf::Vector2f(WIDTH / 2.f + wallBotW, wallBottomY));
    rWall.setPoint(2, sf::Vector2f(WIDTH / 2.f + wallBotW + 15.f, wallBottomY));
    rWall.setPoint(3, sf::Vector2f(WIDTH / 2.f + wallTopW + 15.f, wallTopY));

    // MNOŻNIKI I BRAMKI
    vector<float> mults = { 40.f, 10.f, 2.f, 0.5f, 0.2f, 0.1f, 0.0f, 0.1f, 0.2f, 0.5f, 2.f, 10.f, 40.f };
    vector<sf::RectangleShape> sBoxes;
    vector<sf::Text> sTexts;
    float leftX = WIDTH / 2.f - wallBotW;
    float rightX = WIDTH / 2.f + wallBotW;
    float totalW = rightX - leftX;
    float bWidth = totalW / (float)mults.size();

    for (size_t i = 0; i < mults.size(); i++) {
        sf::RectangleShape r(sf::Vector2f(bWidth - 2.f, 50.f));
        r.setPosition(sf::Vector2f(leftX + (float)i * bWidth + 1.f, wallBottomY + 15.f));
        if (mults[i] >= 10.f) r.setFillColor(sf::Color(200, 20, 20));
        else if (mults[i] >= 1.f) r.setFillColor(sf::Color(100, 100, 30));
        else r.setFillColor(sf::Color(40, 40, 60));
        sBoxes.push_back(r);
        string label = "x" + (mults[i] >= 1.f ? to_string((int)mults[i]) : to_string(mults[i]).substr(0, 3));
        sf::Text t(font, label, 13);
        sf::FloatRect tb = t.getLocalBounds();
        t.setOrigin(sf::Vector2f(tb.position.x + tb.size.x / 2.f, tb.position.y + tb.size.y / 2.f));
        t.setPosition(sf::Vector2f(leftX + (float)i * bWidth + bWidth / 2.f, wallBottomY + 40.f));
        sTexts.push_back(t);
    }

    sf::Text uiWallet(font), uiBet(font);
    uiWallet.setCharacterSize(22); uiWallet.setPosition(sf::Vector2f(20, 20)); uiWallet.setFillColor(sf::Color::Green);
    uiBet.setCharacterSize(22); uiBet.setPosition(sf::Vector2f(WIDTH - 160, 20)); uiBet.setFillColor(sf::Color::Cyan);

    vector<Ball> balls;
    sf::Clock clock;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        if (shotTimer > 0.f) shotTimer -= dt;

        while (const optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            if (const auto* resize = event->getIf<sf::Event::Resized>()) updateViewport(window, gameView);
            if (const auto* kp = event->getIf<sf::Event::KeyPressed>()) {
                if (kp->code == sf::Keyboard::Key::Up) currentBet += 10.f;
                if (kp->code == sf::Keyboard::Key::Down && currentBet > 10.f) currentBet -= 10.f;
            }
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space) && shotTimer <= 0.f && wallet >= currentBet) {
            wallet -= currentBet; shotTimer = SHOT_DELAY;
            Ball b; b.shape.setRadius(BALL_RADIUS); b.shape.setOrigin(sf::Vector2f(BALL_RADIUS, BALL_RADIUS));
            b.shape.setFillColor(sf::Color::Yellow);
            float startJitter = ((rand() % 400) / 100.f - 2.0f) * 4.f;
            b.shape.setPosition(sf::Vector2f(WIDTH / 2.f + startJitter, 20.f));
            float initialSidePush = ((rand() % 200) / 100.f - 1.0f) * 35.f;
            b.vel = sf::Vector2f(initialSidePush, 450.f);
            b.betAmount = currentBet;
            balls.push_back(b);
        }

        // --- FIZYKA ---
        for (int step = 0; step < 12; step++) {
            float sdt = dt / 12.f;
            for (auto& b : balls) {
                if (!b.alive) continue;
                b.vel.x *= 0.999f;
                b.vel.y += GRAVITY * sdt;
                b.shape.move(b.vel * sdt);

                for (auto& p : pegs) {
                    sf::Vector2f d = b.shape.getPosition() - p.shape.getPosition();
                    float dist = length(d);
                    if (dist < BALL_RADIUS + PEG_RADIUS) {
                        sf::Vector2f n = d / dist;
                        float overlap = (BALL_RADIUS + PEG_RADIUS) - dist;
                        b.shape.setPosition(b.shape.getPosition() + n * (overlap + 0.15f));
                        float dot = b.vel.x * n.x + b.vel.y * n.y;
                        if (dot < 0) { b.vel -= 1.85f * dot * n; b.vel *= 0.65f; p.hitTimer = 0.1f; }
                    }
                }

                // Kolizje ze ścianami
                float bY = b.shape.getPosition().y, bX = b.shape.getPosition().x, curW = getWidthAtY(bY);
                float lWallX = (WIDTH / 2.f - curW);
                float rWallX = (WIDTH / 2.f + curW);

                if (bX < lWallX + BALL_RADIUS) {
                    sf::Vector2f n(0.866f, -0.5f);
                    b.shape.setPosition(sf::Vector2f(lWallX + BALL_RADIUS + 0.2f, bY));
                    float dot = b.vel.x * n.x + b.vel.y * n.y;
                    if (dot < 0) { b.vel -= 1.6f * dot * n; b.vel *= 0.35f; }
                }
                if (bX > rWallX - BALL_RADIUS) {
                    sf::Vector2f n(-0.866f, -0.5f);
                    b.shape.setPosition(sf::Vector2f(rWallX - BALL_RADIUS - 0.2f, bY));
                    float dot = b.vel.x * n.x + b.vel.y * n.y;
                    if (dot < 0) { b.vel -= 1.6f * dot * n; b.vel *= 0.35f; }
                }
            }
        }
        window.clear(sf::Color(20, 20, 25));
        window.setView(gameView);
        for (auto& box : sBoxes) window.draw(box);
        for (auto& txt : sTexts) window.draw(txt);
        window.draw(lWall); window.draw(rWall);
        for (auto& p : pegs) {
            if (p.hitTimer > 0) { p.hitTimer -= dt; p.shape.setFillColor(sf::Color::White); }
            else p.shape.setFillColor(sf::Color(100, 100, 115));
            window.draw(p.shape);
        }
        for (auto& b : balls) {
            if (!b.alive) continue;
            window.draw(b.shape);
            if (b.shape.getPosition().y > wallBottomY + 10.f) {
                float bx = b.shape.getPosition().x;
                if (bx >= leftX && bx <= rightX) {
                    int idx = static_cast<int>((bx - leftX) / bWidth);
                    if (idx >= 0 && idx < (int)mults.size()) wallet += b.betAmount * mults[(size_t)idx];
                }
                b.alive = false;
            }
        }
        erase_if(balls, [](const Ball& b) { return !b.alive; });
        uiWallet.setString("Wallet: $" + to_string((int)wallet));
        uiBet.setString("Bet: $" + to_string((int)currentBet));
        window.draw(uiWallet); window.draw(uiBet);
        window.display();
    }
    return 0;
}