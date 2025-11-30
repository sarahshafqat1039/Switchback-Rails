#include "app.h"
#include "../core/simulation_state.h"
#include "../core/simulation.h"
#include "../core/grid.h"
#include "../core/switches.h"
#include "../core/io.h"
#include <SFML/Graphics.hpp>
#include <cmath>
#include <cstdio>

// ============================================================================
// APP.CPP - Implementation of SFML application (NO CLASSES)
// ============================================================================

// ----------------------------------------------------------------------------
// GLOBAL VARIABLES FOR APP STATE
// ----------------------------------------------------------------------------
static sf::RenderWindow* g_window = nullptr;
static sf::Font g_font;

// View for camera (panning/zoom)
static sf::View g_camera;

// Simulation state
static bool g_isPaused = false;
static bool g_isStepMode = false;

// Mouse state
static bool g_isDragging = false;
static int g_lastMouseX = 0;
static int g_lastMouseY = 0;

// Grid rendering parameters
static const int TILE_SIZE = 48; // Choose 32 or 48
static float g_cellSize = TILE_SIZE;
static float g_gridOffsetX = 50.0f;
static float g_gridOffsetY = 50.0f;

// Textures for sprites
static sf::Texture g_textures[10]; // Assume 5 sprites: 1.png to 5.png

// Sprites
static sf::Sprite g_sprites[10];

// ----------------------------------------------------------------------------
// INITIALIZATION
// ----------------------------------------------------------------------------
// This function will initialize the SFML application window and resources.
// It creates a render window with a specified size and title, sets the
// framerate limit, attempts to load a font file for text rendering, and
// initializes the camera view. Returns true on success, false on failure.
// This should be called once at the start of the application before entering
// the main loop.
// ----------------------------------------------------------------------------
bool initializeApp() {
    // Create window
    g_window = new sf::RenderWindow(sf::VideoMode(1200, 800), "Switchback Rails");
    if (!g_window) return false;
    g_window->setFramerateLimit(60);

    // Load font
    if (!g_font.loadFromFile("arial.ttf")) { // Assume arial.ttf exists or use default
        // Use default font if available
    }

    // Initialize camera
    g_camera = g_window->getDefaultView();

    // Adjust camera to fit the grid. Use setSize to avoid confusing zoom multipliers
    float gridWidth = COLS * g_cellSize + 2 * g_gridOffsetX;
    float gridHeight = ROWS * g_cellSize + 2 * g_gridOffsetY;
    float winW = (float)g_window->getSize().x;
    float winH = (float)g_window->getSize().y;
    // scale so whole grid fits into the view while preserving aspect
    float scaleX = gridWidth / winW;
    float scaleY = gridHeight / winH;
    float scale = (scaleX > scaleY) ? scaleX : scaleY;
    // Set the view size to window size * scale so the view covers full grid
    g_camera.setSize(winW * scale, winH * scale);
    g_camera.setCenter(gridWidth / 2.0f, gridHeight / 2.0f);
    g_window->setView(g_camera);

    // Load textures
    for (int i = 0; i < 5; ++i) {
        char filename[20];
        sprintf(filename, "Sprites/%d.png", i + 1);
        if (!g_textures[i].loadFromFile(filename)) {
            // Failed to load, use placeholder
        }
        g_sprites[i].setTexture(g_textures[i]);
    }

    // Custom sprite mapping
    // 1.png (g_textures[0]) contains signal lights: three indicators left-to-right at 128px each
    // 5.png (g_textures[4]) contains the track pieces at various IntRect positions (128px tiles)
// Custom sprite mapping based on actual sprite sheets
    // Sprites use 128px tiles
    const int TILE_RECT = 128;

    // === IMAGE 1: Traffic Signals (1.png = g_textures[0]) ===
    // Green, Yellow, Red indicators arranged horizontally
    if (g_textures[0].getSize().x >= 384) {
        // Green Signal -> sprite slot 5
        g_sprites[5].setTexture(g_textures[0]);
        g_sprites[5].setTextureRect(sf::IntRect(0, 0, TILE_RECT, TILE_RECT));
        g_sprites[5].setOrigin(TILE_RECT/2.0f, TILE_RECT/2.0f);
        g_sprites[5].setScale(g_cellSize / (float)TILE_RECT, g_cellSize / (float)TILE_RECT);
        
        // Yellow Signal -> sprite slot 6
        g_sprites[6].setTexture(g_textures[0]);
        g_sprites[6].setTextureRect(sf::IntRect(TILE_RECT, 0, TILE_RECT, TILE_RECT));
        g_sprites[6].setOrigin(TILE_RECT/2.0f, TILE_RECT/2.0f);
        g_sprites[6].setScale(g_cellSize / (float)TILE_RECT, g_cellSize / (float)TILE_RECT);
        
        // Red Signal -> sprite slot 7
        g_sprites[7].setTexture(g_textures[0]);
        g_sprites[7].setTextureRect(sf::IntRect(TILE_RECT*2, 0, TILE_RECT, TILE_RECT));
        g_sprites[7].setOrigin(TILE_RECT/2.0f, TILE_RECT/2.0f);
        g_sprites[7].setScale(g_cellSize / (float)TILE_RECT, g_cellSize / (float)TILE_RECT);
    }

    // === IMAGE 5: Track Pieces (5.png = g_textures[4]) ===
    // Layout: Row 0: [Horizontal]
    //         Row 1: [Vertical]
    //         Row 2: [Diagonal /, Diagonal \]
    //         Row 3: [Crossing]
    if (g_textures[4].getSize().x > 0) {
        // Horizontal Track -> sprite 0 (top-left)
        g_sprites[0].setTexture(g_textures[4]);
        g_sprites[0].setTextureRect(sf::IntRect(0, 0, TILE_RECT, TILE_RECT));
        g_sprites[0].setOrigin(TILE_RECT/2.0f, TILE_RECT/2.0f);
        g_sprites[0].setScale(g_cellSize / (float)TILE_RECT, g_cellSize / (float)TILE_RECT);

        // Vertical Track -> sprite 1 (middle-right)
        g_sprites[1].setTexture(g_textures[4]);
        g_sprites[1].setTextureRect(sf::IntRect(TILE_RECT, 0, TILE_RECT, TILE_RECT));
        g_sprites[1].setOrigin(TILE_RECT/2.0f, TILE_RECT/2.0f);
        g_sprites[1].setScale(g_cellSize / (float)TILE_RECT, g_cellSize / (float)TILE_RECT);

        // Diagonal / (BL->TR) -> sprite 2 (bottom-left)
        g_sprites[2].setTexture(g_textures[4]);
        g_sprites[2].setTextureRect(sf::IntRect(0, TILE_RECT, TILE_RECT, TILE_RECT));
        g_sprites[2].setOrigin(TILE_RECT/2.0f, TILE_RECT/2.0f);
        g_sprites[2].setScale(g_cellSize / (float)TILE_RECT, g_cellSize / (float)TILE_RECT);

        // Diagonal \ (TL->BR) -> sprite 3 (bottom-right)
        g_sprites[3].setTexture(g_textures[4]);
        g_sprites[3].setTextureRect(sf::IntRect(TILE_RECT, TILE_RECT, TILE_RECT, TILE_RECT));
        g_sprites[3].setOrigin(TILE_RECT/2.0f, TILE_RECT/2.0f);
        g_sprites[3].setScale(g_cellSize / (float)TILE_RECT, g_cellSize / (float)TILE_RECT);

        // Crossing -> sprite 4 (center-bottom)
        g_sprites[4].setTexture(g_textures[4]);
        g_sprites[4].setTextureRect(sf::IntRect(TILE_RECT/2, TILE_RECT*2, TILE_RECT, TILE_RECT));
        g_sprites[4].setOrigin(TILE_RECT/2.0f, TILE_RECT/2.0f);
        g_sprites[4].setScale(g_cellSize / (float)TILE_RECT, g_cellSize / (float)TILE_RECT);
    }

    // === IMAGE 2: Train Sprites (2.png = g_textures[1]) ===
    // Store train sprite for directional rendering
    // Layout: Row 0: [Up, Down], Row 1: [Left, Right]
    if (g_textures[1].getSize().x > 0) {
        // We'll use sprite slot 8 for trains and apply rotation in render
        g_sprites[8].setTexture(g_textures[1]);
        g_sprites[8].setTextureRect(sf::IntRect(0, 0, TILE_RECT*2, TILE_RECT)); // Use UP direction as base
        g_sprites[8].setOrigin(TILE_RECT, TILE_RECT/2.0f);
        g_sprites[8].setScale(g_cellSize / (float)(TILE_RECT*2), g_cellSize / (float)TILE_RECT);
    }

    return true;
}

// Helper to get sprite index from tile
int getSpriteIndex(char tile) {
    if (tile == '-') return 0; // Horizontal track
    if (tile == '|') return 1; // Vertical track
    if (tile == '/' || tile == '\\') return 2; // Curves
    if (tile == '+') return 3; // Crossing
    if (tile >= 'A' && tile <= 'Z') return 4; // Switch (use same as train for now)
    if (tile == 'S' || tile == 'D' || tile == '=') return 0; // Default to track
    return 0; // Default
}

// Helper to render the grid
void renderGrid() {
    // Background for grid
    sf::RectangleShape bg(sf::Vector2f(COLS * g_cellSize, ROWS * g_cellSize));
    bg.setPosition(g_gridOffsetX, g_gridOffsetY);
    bg.setFillColor(sf::Color(24, 24, 24));
    g_window->draw(bg);

    // Lightweight gridlines for readability
    sf::VertexArray lines(sf::Lines);
    for (int y = 0; y <= ROWS; ++y) {
        float yy = g_gridOffsetY + y * g_cellSize;
        lines.append(sf::Vertex(sf::Vector2f(g_gridOffsetX, yy), sf::Color(40,40,40)));
        lines.append(sf::Vertex(sf::Vector2f(g_gridOffsetX + COLS * g_cellSize, yy), sf::Color(40,40,40)));
    }
    for (int x = 0; x <= COLS; ++x) {
        float xx = g_gridOffsetX + x * g_cellSize;
        lines.append(sf::Vertex(sf::Vector2f(xx, g_gridOffsetY), sf::Color(40,40,40)));
        lines.append(sf::Vertex(sf::Vector2f(xx, g_gridOffsetY + ROWS * g_cellSize), sf::Color(40,40,40)));
    }
    g_window->draw(lines);

    for (int y = 0; y < ROWS; ++y) {
        for (int x = 0; x < COLS; ++x) {
            char tile = grid[y][x];
            if (tile == ' ') continue; // Don't draw anything for empty tiles

            // Fill color by tile type to make it clearer
            sf::Color fill = sf::Color(80, 80, 80); // default track
            if (tile == 'S') fill = sf::Color(64, 160, 64); // source green
            else if (tile == 'D') fill = sf::Color(64, 64, 160); // destination blue
            else if (tile == '=') fill = sf::Color(180, 180, 180);
            else if (tile >= 'A' && tile <= 'Z') fill = sf::Color(200, 140, 60); // switch color

            sf::RectangleShape rect(sf::Vector2f(g_cellSize - 2.0f, g_cellSize - 2.0f));
            rect.setPosition(g_gridOffsetX + x * g_cellSize + 1.0f, g_gridOffsetY + y * g_cellSize + 1.0f);
            rect.setFillColor(fill);
            g_window->draw(rect);

            // Draw sprite overlay if available
            int spriteIndex = getSpriteIndex(tile);
            if (g_textures[spriteIndex].getSize().x > 0) {
                float px = g_gridOffsetX + x * g_cellSize + g_cellSize / 2.0f;
                float py = g_gridOffsetY + y * g_cellSize + g_cellSize / 2.0f;
                g_sprites[spriteIndex].setPosition(px, py);
                g_window->draw(g_sprites[spriteIndex]);
            }
        }
    }
    // Draw trains on top using a direct loop; include rotation and color
    for (int t = 0; t < num_trains; ++t) {
        if (trains[t].state == TRAIN_CRASHED || trains[t].state == TRAIN_DELIVERED) continue;
        float px = g_gridOffsetX + trains[t].x * g_cellSize + g_cellSize / 2.0f;
        float py = g_gridOffsetY + trains[t].y * g_cellSize + g_cellSize / 2.0f;
        // Rotation based on direction
        float rotation = 0.0f;
        if (trains[t].direction == DIR_UP) rotation = -90.0f;
        else if (trains[t].direction == DIR_RIGHT) rotation = 0.0f;
        else if (trains[t].direction == DIR_DOWN) rotation = 90.0f;
        else if (trains[t].direction == DIR_LEFT) rotation = 180.0f;

        // Map color index to a visible sf::Color
        sf::Color tcol;
        switch (trains[t].color) {
            case 0: tcol = sf::Color::Red; break;
            case 1: tcol = sf::Color::Blue; break;
            case 2: tcol = sf::Color::Green; break;
            case 3: tcol = sf::Color::Yellow; break;
            case 4: tcol = sf::Color::Magenta; break;
            case 5: tcol = sf::Color::Cyan; break;
            default: tcol = sf::Color(200,200,200); break;
        }

        if (g_textures[4].getSize().x > 0) {
            g_sprites[4].setPosition(px, py);
            g_sprites[4].setRotation(rotation);
            g_sprites[4].setColor(tcol);
            g_window->draw(g_sprites[4]);
        } else {
            // Fallback: colored rectangle for trains
            sf::RectangleShape trainRect(sf::Vector2f(g_cellSize * 0.7f, g_cellSize * 0.7f));
            trainRect.setOrigin(g_cellSize * 0.35f, g_cellSize * 0.35f);
            trainRect.setPosition(px, py);
            trainRect.setFillColor(tcol);
            g_window->draw(trainRect);
        }
    }
}

// ----------------------------------------------------------------------------
// MAIN RUN LOOP
// ----------------------------------------------------------------------------
// This function will run the main application loop. It handles event processing,
// simulation updates, and rendering. The loop continues while the window is open.
// It processes SFML events (window close, keyboard input, mouse input), updates
// the simulation at a fixed interval (2 ticks per second) when not paused,
// checks if the simulation is complete, and renders the current frame. Keyboard
// controls: SPACE to pause/resume, PERIOD to step one tick, ESC to exit. The
// loop exits when the window is closed or ESC is pressed.
// ----------------------------------------------------------------------------
void runApp() {
    sf::Clock clock;
    float tickTimer = 0.0f;
    const float tickInterval = 0.5f; // 2 ticks per second

    while (g_window->isOpen()) {
        sf::Event event;
        while (g_window->pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                g_window->close();
            } else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Space) {
                    g_isPaused = !g_isPaused;
                } else if (event.key.code == sf::Keyboard::Period) {
                    g_isStepMode = true;
                } else if (event.key.code == sf::Keyboard::Escape) {
                    g_window->close();
                }
            } else if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    // Place/remove safety tile
                    int gridX = (event.mouseButton.x - g_gridOffsetX) / g_cellSize;
                    int gridY = (event.mouseButton.y - g_gridOffsetY) / g_cellSize;
                    toggleSafetyTile(gridX, gridY);
                } else if (event.mouseButton.button == sf::Mouse::Right) {
                    // Toggle switch
                    int gridX = (event.mouseButton.x - g_gridOffsetX) / g_cellSize;
                    int gridY = (event.mouseButton.y - g_gridOffsetY) / g_cellSize;
                    if (isSwitchTile(gridX, gridY)) {
                        int idx = getSwitchIndex(grid[gridY][gridX]);
                        toggleSwitchState(idx);
                    }
                } else if (event.mouseButton.button == sf::Mouse::Middle) {
                    g_isDragging = true;
                    g_lastMouseX = event.mouseButton.x;
                    g_lastMouseY = event.mouseButton.y;
                }
            } else if (event.type == sf::Event::MouseButtonReleased) {
                if (event.mouseButton.button == sf::Mouse::Middle) {
                    g_isDragging = false;
                }
            } else if (event.type == sf::Event::MouseMoved) {
                if (g_isDragging) {
                    int dx = event.mouseMove.x - g_lastMouseX;
                    int dy = event.mouseMove.y - g_lastMouseY;
                    g_camera.move(-dx, -dy);
                    g_lastMouseX = event.mouseMove.x;
                    g_lastMouseY = event.mouseMove.y;
                }
            } else if (event.type == sf::Event::MouseWheelScrolled) {
                if (event.mouseWheelScroll.delta > 0) {
                    g_camera.zoom(0.9f);
                } else {
                    g_camera.zoom(1.1f);
                }
            }
        }

        // Update simulation
        float deltaTime = clock.restart().asSeconds();
        tickTimer += deltaTime;
        if (tickTimer >= tickInterval && (!g_isPaused || g_isStepMode)) {
            if (!isSimulationComplete()) {
                simulateOneTick();
            }
            tickTimer = 0.0f;
            g_isStepMode = false;
        }

        // Render
        g_window->clear(sf::Color::Black);
        g_window->setView(g_camera);
        renderGrid();
        g_window->display();
    }
}

// ----------------------------------------------------------------------------
// CLEANUP
// ----------------------------------------------------------------------------
// This function will clean up all resources and close the application window.
// It deletes the render window object and sets the pointer to nullptr. This
// should be called once at the end of the application before exiting to ensure
// proper resource cleanup.
// ----------------------------------------------------------------------------
void cleanupApp() {
    if (g_window) {
        delete g_window;
        g_window = nullptr;
    }
    writeMetrics();
}
