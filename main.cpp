#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <fstream>
#include <algorithm>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int BACKGROUND_START_HEIGHT = 600;
const int GROUND_HEIGHT = 500;
const int GRAVITY = 2;
const int PLAYER_SPEED = 5;
int spawnTimer = 0;
int BLOCK_FALL_SPEED = 5;
const int BLOCK_WIDTH = 50;
const int BLOCK_HEIGHT = 50;
int blocksPerSecond = 4;
int timeSinceStart = 0;
int lastSpeedUpTime = 0;
int speedUpCount = 0;
Uint32 lastBlockSpawnTime = 0;
const Uint32 blockSpawnIntervalMs = 1000;  // mỗi giây
int frameCounter = 0;
int speedIncreaseCount = 0;
int survivalTime = 0;
int bestSurvivalTime = 0;
bool isNewRecord = false;
Uint32 recordTimerStart = 0;
const Uint32 recordAnimDuration = 3000;
bool lavaActive = false;
int lavaY = SCREEN_HEIGHT;
int lavaTargetY = 0;
Uint32 lavaStartTime = 0;
Uint32 lastLavaMeltTime = 0;
const int LAVA_RISE_SPEED = 1; // pixel mỗi frame
const int LAVA_MELT_INTERVAL = 3000; // ms
SDL_Rect camera = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
bool showPlayer = true;
bool isPaused = false;
SDL_Texture* gameOverTexture = nullptr;
SDL_Texture* menuTexture = nullptr;
SDL_Texture* pauseGameTexture = nullptr;
SDL_Texture* pauseButtonTexture = nullptr;
SDL_Texture* settingsTexture = nullptr;
enum GameState { MENU, PLAYING, PAUSED, GAME_OVER, SETTINGS, TUTORIAL };
GameState gameState = MENU;
TTF_Font* font = nullptr;
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* player_stand_left = nullptr;
SDL_Texture* player_stand_right = nullptr;
SDL_Texture* player_jump_left = nullptr;
SDL_Texture* player_jump_right = nullptr;
SDL_Texture* player_run_left_fast = nullptr;
SDL_Texture* player_run_right_fast = nullptr;
SDL_Texture* playerTexture = nullptr;
SDL_Texture* backgroundStart = nullptr;
SDL_Texture* tutorialTexture = nullptr;
SDL_Texture* blockTextures[5] = { nullptr, nullptr, nullptr , nullptr, nullptr};
SDL_Texture* lavaTexture = nullptr;
SDL_Texture* heartTexture = nullptr;
SDL_Texture* heartEmptyTexture = nullptr;
SDL_Texture* currentPlayerTexture = nullptr;
SDL_Texture* renderText(const std::string& message, SDL_Color color);
SDL_Rect pauseButtonRect = {375, 10, 50, 50};
Mix_Chunk* jumpSound = nullptr;
Mix_Chunk* hitSound = nullptr;
Mix_Chunk* gameOverSound = nullptr;
Mix_Music* backgroundMusic = nullptr;
int musicVolume = 64;
int sfxVolume = 64;

Uint32 levelUpStartTime = 0;
bool showLevelUp = false;
int frame = 0;
int frameDelay = 4;
int currentRunFrame = 0;
int maxPlayerHeight = SCREEN_HEIGHT;
bool isPausedForHit = false;
Uint32 pauseStartTime = 0;
int countdownNumber = 3;

enum ButtonAction {
    BUTTON_PLAY,
    BUTTON_TUTORIAL,
    BUTTON_SETTINGS,
    BUTTON_REPLAY,
    BUTTON_MENU,
    BUTTON_RESUME,
    BUTTON_MUSIC_UP,
    BUTTON_MUSIC_DOWN,
    BUTTON_SFX_UP,
    BUTTON_SFX_DOWN,
    BUTTON_BACK,
    BUTTON_TUTORIAL_BACK,
};

struct Player {
    int x, y, w, h;
    int velocityY;
    int velocityX;
    bool isJumping;
    int lives;
    bool wasOnBlock;
    bool wasCrushed;
    bool facingLeft;       // Hướng đang di chuyển: true nếu sang trái
    bool lastFacingLeft;  // Hướng cuối cùng (dù đứng im): dùng để biết đứng im nhìn hướng nào
    int playerLives = 3;
    bool pauseAfterHit = false;
    Uint32 pauseStartTime = 0;
    int PAUSE_DURATION = 3000;
} player;

struct Block {
    int x, y, w, h;
    bool isLanded;
    int type;
    Uint32 lavaContactTime = 0;
};

struct TextButton {
    std::string label;
    SDL_Texture* texture;
    SDL_Rect rect;
    ButtonAction action;
};

TextButton createTextButton(const std::string& label, SDL_Rect rect, ButtonAction action) {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Texture* tex = renderText(label, white);
    return {label, tex, rect, action};
}
std::vector<TextButton> menuButtons;
std::vector<TextButton> pauseButtons;
std::vector<TextButton> gameOverButtons;
std::vector<TextButton> settingsButtons;
std::vector<TextButton> tutorialButtons;
std::vector<std::string> tutorialLines = {
    "Use LEFT and RIGHT arrows to move.",
    "Press UP or SPACE to jump.",
    "Avoid falling blocks and lava.",
    "Survive as long as you can!"
};

void initTextButtons() {
     menuButtons = {
        createTextButton("PLAY", {230, 340, 340, 60}, BUTTON_PLAY),
        createTextButton("TUTORIAL", {230, 440, 340, 60}, BUTTON_TUTORIAL),
        createTextButton("SETTINGS", {230, 540, 340, 60}, BUTTON_SETTINGS),
    };

    pauseButtons = {
        createTextButton("RESUME", { 320, 250, 175, 40}, BUTTON_RESUME),
        createTextButton("REPLAY", {320, 370, 175, 40}, BUTTON_REPLAY),
        createTextButton("MENU", {320, 490,175,40}, BUTTON_MENU),
    };

    gameOverButtons = {
        createTextButton("REPLAY", {320, 310,170,40}, BUTTON_REPLAY),
        createTextButton("MENU", {320, 470,165,40}, BUTTON_MENU),
    };
    settingsButtons = {
        createTextButton("-",  {130, 300, 60, 40}, BUTTON_MUSIC_DOWN),
        createTextButton("+",  {615, 300, 60, 40}, BUTTON_MUSIC_UP),
        createTextButton("-",  {130, 470, 60, 40}, BUTTON_SFX_DOWN),
        createTextButton("+",  {615, 470, 60, 40}, BUTTON_SFX_UP),
        createTextButton("BACK", {80, 80, 30, 50}, BUTTON_MENU),
    };

    tutorialButtons = {
        createTextButton("BACK", {90, 65, 30, 50}, BUTTON_TUTORIAL_BACK),
    };
}

void renderTextButtons(const std::vector<TextButton>& buttons) {
    int mx, my;
    SDL_GetMouseState(&mx, &my);

    // Để tính vị trí phần trăm âm lượng
    SDL_Rect musicMinusBtn = {}, musicPlusBtn = {};
    SDL_Rect sfxMinusBtn = {}, sfxPlusBtn = {};

    for (const auto& btn : buttons) {
        SDL_Point mousePoint = {mx, my};
        bool hovered = SDL_PointInRect(&mousePoint, &btn.rect);

        // Đổi màu khi hover
        if (hovered) {
            SDL_SetTextureColorMod(btn.texture, 255, 255, 150); // Phát sáng
        } else {
            SDL_SetTextureColorMod(btn.texture, 255, 255, 255); // Bình thường
        }

        // Lấy kích thước gốc của chữ
        int textW, textH;
        SDL_QueryTexture(btn.texture, nullptr, nullptr, &textW, &textH);

        // Tăng kích cỡ khi hover (scale 1.2 lần)
        float scale = hovered ? 1.2f : 1.0f;
        int scaledW = static_cast<int>(textW * scale);
        int scaledH = static_cast<int>(textH * scale);

        SDL_Rect textRect = {
            btn.rect.x + (btn.rect.w - scaledW) / 2,
            btn.rect.y + (btn.rect.h - scaledH) / 2,
            scaledW,
            scaledH
        };

        SDL_RenderCopy(renderer, btn.texture, nullptr, &textRect);

        // Ghi lại vị trí nút - / +
        if (btn.action == BUTTON_MUSIC_DOWN) musicMinusBtn = btn.rect;
        if (btn.action == BUTTON_MUSIC_UP)   musicPlusBtn  = btn.rect;
        if (btn.action == BUTTON_SFX_DOWN)   sfxMinusBtn   = btn.rect;
        if (btn.action == BUTTON_SFX_UP)     sfxPlusBtn    = btn.rect;
    }

    // Hiển thị phần trăm MUSIC ở giữa nút - và +
    if (musicMinusBtn.w > 0 && musicPlusBtn.w > 0) {
        std::string volText = std::to_string((musicVolume * 100) / 128) + "%";
        SDL_Texture* volTex = renderText(volText, {255, 255, 255, 255});
        int w, h;
        SDL_QueryTexture(volTex, nullptr, nullptr, &w, &h);
        int centerX = (musicMinusBtn.x + musicMinusBtn.w + musicPlusBtn.x) / 2 - w / 2;
        int centerY = musicMinusBtn.y + (musicMinusBtn.h - h) / 2;
        SDL_Rect textRect = {centerX, centerY, w, h};
        SDL_RenderCopy(renderer, volTex, nullptr, &textRect);
        SDL_DestroyTexture(volTex);
    }

    // Hiển thị phần trăm SFX ở giữa nút - và +
    if (sfxMinusBtn.w > 0 && sfxPlusBtn.w > 0) {
        std::string sfxText = std::to_string((sfxVolume * 100) / 128) + "%";
        SDL_Texture* sfxTex = renderText(sfxText, {255, 255, 255, 255});
        int w, h;
        SDL_QueryTexture(sfxTex, nullptr, nullptr, &w, &h);
        int centerX = (sfxMinusBtn.x + sfxMinusBtn.w + sfxPlusBtn.x) / 2 - w / 2;
        int centerY = sfxMinusBtn.y + (sfxMinusBtn.h - h) / 2;
        SDL_Rect textRect = {centerX, centerY, w, h};
        SDL_RenderCopy(renderer, sfxTex, nullptr, &textRect);
        SDL_DestroyTexture(sfxTex);
    }
}



void resetGame();


void handleTextButtonClick(int mouseX, int mouseY, const std::vector<TextButton>& buttons) {
    for (const auto& btn : buttons) {
      SDL_Point pt = {mouseX, mouseY};
        if (SDL_PointInRect(&pt, &btn.rect)) {
            switch (btn.action) {
                case BUTTON_PLAY: gameState = PLAYING; resetGame(); break;
                case BUTTON_TUTORIAL_BACK: gameState = MENU; break;
                case BUTTON_SETTINGS: gameState = SETTINGS; break;
                case BUTTON_REPLAY: resetGame(); gameState = PLAYING; break;
                case BUTTON_MENU: gameState = MENU; break;
                case BUTTON_RESUME: gameState = PLAYING; break;
                case BUTTON_MUSIC_UP:
                        musicVolume = std::min(128, musicVolume + 13);
                        Mix_VolumeMusic(musicVolume);
                        break;
                case BUTTON_MUSIC_DOWN:
                        musicVolume = std::max(0, musicVolume - 13);
                        Mix_VolumeMusic(musicVolume);
                        break;
                case BUTTON_SFX_UP:
                        sfxVolume = std::min(128, sfxVolume + 13);
                        Mix_VolumeChunk(jumpSound,    sfxVolume);
                        Mix_VolumeChunk(hitSound,     sfxVolume);
                        Mix_VolumeChunk(gameOverSound,sfxVolume);
                        break;
                case BUTTON_SFX_DOWN:
                        sfxVolume = std::max(0, sfxVolume - 13);
                        Mix_VolumeChunk(jumpSound,    sfxVolume);
                        Mix_VolumeChunk(hitSound,     sfxVolume);
                        Mix_VolumeChunk(gameOverSound,sfxVolume);
                        break;
                case BUTTON_BACK:
                        gameState = MENU;
                        break;
                case BUTTON_TUTORIAL:
                        gameState = TUTORIAL;
                        break;
                default: break;
            }
        }
    }
}

std::vector<Block> blocks;
bool gameOver = false;
int score = 0;
bool isGameStarted = false;

bool initSDL() {
    if (TTF_Init() == -1) {
    std::cout << "Failed to initialize SDL_ttf: " << TTF_GetError() << std::endl;
    return false;
    }
    font = TTF_OpenFont("arial.ttf", 36);
    if (!font) {
    std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
    return false;
    }
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) return false;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) return false;
    window = SDL_CreateWindow("Falling Blocks Survival", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) return false;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) return false;
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) return false;
    return true;
}

SDL_Texture* loadTexture(const char* path) {
    SDL_Surface* loadedSurface = IMG_Load(path);
    if (!loadedSurface) return nullptr;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
    SDL_FreeSurface(loadedSurface);
    return texture;
}

SDL_Texture* renderText(const std::string& message, SDL_Color color) {
    SDL_Surface* textSurface = TTF_RenderText_Blended(font, message.c_str(), color);
    if (!textSurface) {
        std::cout << "TTF_RenderText Error: " << TTF_GetError() << std::endl;
        return nullptr;
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);
    return textTexture;
}


void renderSurvivalTimes() {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color currentColor = white;

    if (isNewRecord) {
        Uint32 elapsed = SDL_GetTicks() - recordTimerStart;
        int alpha = (SDL_GetTicks() / 300) % 2 == 0 ? 255 : 100;
        currentColor = {255, 255, 0, (Uint8)alpha}; // vàng nhấp nháy
    }

    std::string survivalStr = "Survival Time: " + std::to_string(survivalTime / 60) + "s";
    std::string bestStr = "Best Time: " + std::to_string(bestSurvivalTime / 60) + "s";

    SDL_Texture* survivalTexture = renderText(survivalStr, currentColor);
    SDL_Texture* bestTexture = renderText(bestStr, currentColor);

    if (survivalTexture && bestTexture) {
        SDL_Rect survivalRect = {10, 10, 0, 0};
        SDL_Rect bestRect = {10, 40, 0, 0};

        SDL_QueryTexture(survivalTexture, nullptr, nullptr, &survivalRect.w, &survivalRect.h);
        SDL_QueryTexture(bestTexture, nullptr, nullptr, &bestRect.w, &bestRect.h);

        SDL_RenderCopy(renderer, survivalTexture, nullptr, &survivalRect);
        SDL_RenderCopy(renderer, bestTexture, nullptr, &bestRect);

        SDL_DestroyTexture(survivalTexture);
        SDL_DestroyTexture(bestTexture);
    }
}

void saveBestTime(int time) {
    std::ofstream out("record.txt");
    if (out) {
        out << time;
        out.close();
    }
}

int loadBestTime() {
    std::ifstream in("record.txt");
    int time = 0;
    if (in) {
        in >> time;
        in.close();
    }
    return time;
}


Mix_Chunk* loadSound(const char* path) {
    return Mix_LoadWAV(path);
}

Mix_Music* loadMusic(const char* path) {
    return Mix_LoadMUS(path);
}

void loadMedia() {
    menuTexture = loadTexture("menu.png");
    pauseButtonTexture = loadTexture( "pause_button.png");
    gameOverTexture = loadTexture("gameover.png");
    pauseGameTexture = loadTexture("pause.png");
    settingsTexture = loadTexture("settings.png");
    tutorialTexture = loadTexture("tutorial.png");
    player_stand_left      = loadTexture("player_stand_left.png");
    player_stand_right     = loadTexture("player_stand_right.png");
    player_jump_left       = loadTexture("player_jump_left.png");
    player_jump_right      = loadTexture("player_jump_right.png");
    player_run_left_fast    = loadTexture("player_run_left_fast.png");;
    player_run_right_fast   = loadTexture("player_run_right_fast.png");
    backgroundStart = loadTexture("backgroundStart.png");
    blockTextures[0] = loadTexture("block1.png");
    blockTextures[1] = loadTexture("block2.png");
    blockTextures[2] = loadTexture("block3.png");
    blockTextures[3] = loadTexture("block4.png");
    blockTextures[4] = loadTexture("block5.png");
    heartTexture = loadTexture("heart.png");
    heartEmptyTexture = loadTexture("heart_empty.png");
    lavaTexture = loadTexture("lava.png");
    jumpSound = loadSound("jump.wav");
    hitSound = loadSound("hit.wav");
    gameOverSound = loadSound("gameover.wav");
    backgroundMusic = loadMusic("background.mp3");
    Mix_PlayMusic(backgroundMusic, -1);
    Mix_VolumeMusic(musicVolume);
    Mix_VolumeChunk(jumpSound, sfxVolume);
    Mix_VolumeChunk(hitSound, sfxVolume);
    Mix_VolumeChunk(gameOverSound, sfxVolume);
    Mix_VolumeMusic(musicVolume);
    Mix_Volume(-1, sfxVolume);
}

void spawnBlock() {
    Block b;
    b.w = BLOCK_WIDTH;
    b.h = BLOCK_HEIGHT;
    b.x = (rand() % (SCREEN_WIDTH / BLOCK_WIDTH)) * BLOCK_WIDTH;
    b.y = -BLOCK_HEIGHT;
    b.isLanded = false;
    b.type = rand() % 5;
    blocks.push_back(b);
}

void resetGame() {
    player.w = 50;
    player.h = 50;
    // Đặt lại vi trí và kích thuoc player
    player.x = SCREEN_WIDTH / 2 - player.w / 2;
    player.y = SCREEN_HEIGHT - player.h - 100;
    maxPlayerHeight = player.y;
    player.velocityX = 0;
    player.velocityY = 0;
    player.isJumping = false;
    player.lives = 3;
    player.wasOnBlock = false;
    player.wasCrushed = false;

    // Xóa tất cả block hiện có
    blocks.clear();

    // Reset các biến game
    gameOver = false;
    survivalTime = 0;
    spawnTimer = 0;
    BLOCK_FALL_SPEED = 3;
    speedUpCount = 0;
    lastSpeedUpTime = SDL_GetTicks();
    lastBlockSpawnTime = SDL_GetTicks();
    lavaActive = false;
    lavaStartTime = SDL_GetTicks();

    if (backgroundMusic) {
        Mix_PlayMusic(backgroundMusic, -1);
    }
}

void handleEvents(bool& running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            running = false;
        } else if (e.type == SDL_KEYDOWN) {
            if (gameState == MENU) {
                if (e.key.keysym.sym == SDLK_SPACE) {
                    resetGame();
                    gameState = PLAYING;
                }
            } else if (gameState == PLAYING) {
                switch (e.key.keysym.sym) {
                    case SDLK_LEFT:
                        player.velocityX = -PLAYER_SPEED;
                        break;
                    case SDLK_RIGHT:
                        player.velocityX = PLAYER_SPEED;
                        break;
                    case SDLK_UP:
                        if (!player.isJumping) {
                            player.velocityY = -20;
                            player.isJumping = true;
                            Mix_PlayChannel(-1, jumpSound, 0);
                        }
                        break;
                    case SDLK_SPACE:
                        if (!player.isJumping) {
                            player.velocityY = -20;
                            player.isJumping = true;
                            Mix_PlayChannel(-1, jumpSound, 0);
                        } else {
                            // Bám mép khi đang nhảy
                            const int grabRangeX = 10;
                            const int grabRangeY = 15;

                            for (const auto& block : blocks) {
                                bool nearLeftEdge =
                                    abs((player.x + player.w) - block.x) <= grabRangeX &&
                                    player.y + player.h > block.y &&
                                    player.y < block.y + grabRangeY;

                                bool nearRightEdge =
                                    abs(player.x - (block.x + block.w)) <= grabRangeX &&
                                    player.y + player.h > block.y &&
                                    player.y < block.y + grabRangeY;

                                if (nearLeftEdge || nearRightEdge) {
                                    player.y = block.y - player.h;
                                    player.velocityY = 0;
                                    player.isJumping = false;
                                    Mix_PlayChannel(-1, jumpSound, 0);
                                    break;
                                }
                            }
                        }
                        break;
                    case SDLK_p:
                        gameState = PAUSED;
                        break;
                    case SDLK_ESCAPE:
                        running = false;
                        break;
                }
            } else if (gameState == GAME_OVER) {
                if (e.key.keysym.sym == SDLK_r) {
                    resetGame();
                    gameState = PLAYING;
                } else if (e.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                }
            } else if (gameState == PAUSED) {
                if (e.key.keysym.sym == SDLK_p) {
                    gameState = PLAYING;
                } else if (e.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                }
            }
        } else if (e.type == SDL_KEYUP) {
            if (gameState == PLAYING) {
                if (e.key.keysym.sym == SDLK_LEFT || e.key.keysym.sym == SDLK_RIGHT) {
                    player.velocityX = 0;
                }
            }
        } else if (e.type == SDL_MOUSEBUTTONDOWN) {
            int mx, my;
            SDL_GetMouseState(&mx, &my);

            if (gameState == MENU) {
                handleTextButtonClick(mx, my, menuButtons);
            } else if (gameState == PAUSED) {
                // Khi đang paused, xử lý click nút trong pauseButtons (Resume, Replay, Menu)
                handleTextButtonClick(mx, my, pauseButtons);
            } else if (gameState == GAME_OVER) {
                handleTextButtonClick(mx, my, gameOverButtons);
            } else if (gameState == PLAYING) {
                // Xử lý bấm nút pause ở trên cùng màn hình khi chơi
                if (mx >= pauseButtonRect.x && mx <= pauseButtonRect.x + pauseButtonRect.w &&
                    my >= pauseButtonRect.y && my <= pauseButtonRect.y + pauseButtonRect.h) {
                    gameState = PAUSED;
                }
            } else if (gameState == SETTINGS) {
                handleTextButtonClick(mx, my, settingsButtons);
            }
                else if (gameState == TUTORIAL) {
                    handleTextButtonClick(mx, my, tutorialButtons);
                }
        }
    }
}

void update() {
    if (player.y < maxPlayerHeight) {
    int delta = maxPlayerHeight - player.y;

    // Kéo toàn bộ block xuống nếu camera dịch lên
    for (auto& block : blocks) {
        block.y += delta;
    }
    maxPlayerHeight = player.y;
}


if (isPausedForHit) {
        Uint32 currentTime = SDL_GetTicks();
        Uint32 elapsed = currentTime - pauseStartTime;

        countdownNumber = 3 - elapsed / 1000;

        if (elapsed >= 3000) {
            isPausedForHit = false;
            countdownNumber = 0;
        }
        return;
    }

      if (player.isJumping) {
        if (player.velocityX < 0)
            currentPlayerTexture = player_jump_left;
        else if (player.velocityX > 0)
            currentPlayerTexture = player_jump_right;
        else
            currentPlayerTexture = (player.lastFacingLeft) ? player_jump_left : player_jump_right;
    } else if (player.velocityX < 0) {
        frame++;
        if (frame >= frameDelay * 2) frame = 0;
        currentRunFrame = frame / frameDelay;
        currentPlayerTexture = (currentRunFrame == 0) ? player_run_left_fast : player_run_left_fast;
        player.lastFacingLeft = true;
    } else if (player.velocityX > 0) {
        frame++;
        if (frame >= frameDelay * 2) frame = 0;
        currentRunFrame = frame / frameDelay;
        currentPlayerTexture = (currentRunFrame == 0) ? player_run_right_fast : player_run_right_fast;
        player.lastFacingLeft = false;
    } else {
        currentPlayerTexture = (player.lastFacingLeft) ? player_stand_left : player_stand_right;
    }
    Uint32 currentTime = SDL_GetTicks();
    if (gameOver || player.pauseAfterHit) {
        if (player.pauseAfterHit && currentTime - player.pauseStartTime >= 3000) {
            player.pauseAfterHit = false;
            // Xóa block chưa chạm đất
            blocks.erase(std::remove_if(blocks.begin(), blocks.end(), [](const Block& b) {
                return !b.isLanded;
            }), blocks.end());
        } else {
            return; // đang pause hoặc gameOver
        }
    }


    player.velocityY += GRAVITY;
    player.y += player.velocityY;
    player.x += player.velocityX;
    if (player.y < maxPlayerHeight) {
    maxPlayerHeight = player.y;
}

    if (player.y >= SCREEN_HEIGHT - player.h - 100) {
        player.y = SCREEN_HEIGHT - player.h - 100;
        player.isJumping = false;
    }
    if (player.x < 0) player.x = 0;
    if (player.x + player.w > SCREEN_WIDTH) player.x = SCREEN_WIDTH - player.w;

    bool currentlyOnBlock = false;
    bool crushed = false;

    for (auto it = blocks.begin(); it != blocks.end(); ) {
    Block& block = *it;

    const int BLOCK_DELETE_BUFFER = 800;
    if (block.y > maxPlayerHeight + BLOCK_DELETE_BUFFER) {
        it = blocks.erase(it);
        continue;
    }

    if (!block.isLanded) {
        block.y += BLOCK_FALL_SPEED;
        for (auto& other : blocks) {
            if (&block != &other && block.y + block.h >= other.y && block.y < other.y && block.x == other.x) {
                block.y = other.y - block.h;
                block.isLanded = true;
                break;
            }
        }
        if (block.y + block.h >= SCREEN_HEIGHT - 100) {
            block.y = SCREEN_HEIGHT - 100 - block.h;
            block.isLanded = true;
        }
    }

    // va chạm platform
    if (player.y + player.h > block.y && player.y + player.h - player.velocityY <= block.y &&
        player.x + player.w > block.x && player.x < block.x + block.w) {
        player.y = block.y - player.h;
        player.velocityY = 0;
        player.isJumping = false;
        currentlyOnBlock = true;
    }

    // va chạm bên trái/phải
    if (player.x + player.w > block.x && player.x < block.x + block.w &&
        player.y < block.y + block.h && player.y + player.h > block.y) {
        if (player.velocityX > 0) {
            player.x = block.x - player.w;
        } else if (player.velocityX < 0) {
            player.x = block.x + block.w;
        }
    }

    // bị đè nếu block đang rơi trúng đầu
    if (!block.isLanded &&
        player.y + player.h > block.y && player.y < block.y + block.h &&
        player.x + player.w > block.x && player.x < block.x + block.w) {
        crushed = true;
        isPausedForHit = true;
        pauseStartTime = SDL_GetTicks();
        countdownNumber = 3;
    }

    ++it;
}

    if (crushed && !player.wasCrushed) {
        player.lives--;
        Mix_PlayChannel(-1, hitSound, 0);
        player.wasCrushed = true;
        if (player.lives <= 0) {
        Mix_PlayChannel(-1, gameOverSound, 0);
        Mix_HaltMusic();
        gameOver = true;
        gameState = GAME_OVER;
        return;
        } else {
            player.pauseAfterHit = true;
            player.pauseStartTime = SDL_GetTicks();
        }
    } else if (!crushed) {
        player.wasCrushed = false;
    }

    player.wasOnBlock = currentlyOnBlock;



   Uint32 now = SDL_GetTicks();

if (!lavaActive && survivalTime >= 1200) {
    lavaActive = true;
    // Tìm đáy block thấp nhất
  int lowestBottom = 0;
for (auto& b : blocks) {
    lowestBottom = std::max(lowestBottom, b.y + b.h);
}
lavaY = std::min(lowestBottom, SCREEN_HEIGHT - 1);

}

for (auto& block : blocks) {
    if (lavaActive && block.y + block.h >= lavaY) {
        if (block.lavaContactTime == 0) {
            block.lavaContactTime = now;  // bắt đầu tính thời gian chạm lava
        }
    } else {
        block.lavaContactTime = 0;  // không còn chạm lava nữa
    }
}

// Xóa block nếu đã chạm lava > 3s
blocks.erase(std::remove_if(blocks.begin(), blocks.end(), [&](const Block& b) {
    return b.lavaContactTime > 0 && now - b.lavaContactTime >= 3000;
}), blocks.end());

for (auto& b : blocks) {
    if (b.isLanded) {
        // Tìm xem có block nào đỡ bên dưới không?
        bool supported = false;
        for (const auto& other : blocks) {
            if (&b == &other) continue;

            bool sameColumn = b.x == other.x;
            bool directlyOnTop = b.y + b.h == other.y;

            if (sameColumn && directlyOnTop) {
                supported = true;
                break;
            }
        }

        // Nếu không có gì đỡ bên dưới -> rơi tiếp
        if (!supported && b.y + b.h < SCREEN_HEIGHT - 100) {
            b.isLanded = false;
        }
    }
}



    if (now - lastBlockSpawnTime >= blockSpawnIntervalMs) {
        // spawn 5 block mỗi giây
        for (int i = 0; i < 4; ++i) {
            spawnBlock();
        }
        lastBlockSpawnTime = now;
    }


    if (survivalTime % 1200 == 0 && survivalTime != 0 && blocksPerSecond < 10) {
    blocksPerSecond++; // mỗi 30s tăng thêm 1 block/giây
}

 survivalTime++;
if (lavaActive && player.y + player.h >= lavaY && gameState == PLAYING) {
    Mix_PlayChannel(-1, gameOverSound, 0);
    Mix_HaltMusic();
    gameOver = true;
    gameState = GAME_OVER;
    return;
}

if (survivalTime > bestSurvivalTime) {
    if (!isNewRecord) {
        isNewRecord = true;
        recordTimerStart = SDL_GetTicks();
    }
    bestSurvivalTime = survivalTime;
    saveBestTime(bestSurvivalTime); // Ghi kỷ lục mới vào file
} else if (isNewRecord) {
    // Kiểm tra xem animation đã hết chưa
    if (SDL_GetTicks() - recordTimerStart > recordAnimDuration) {
        isNewRecord = false;  // hết animation
                        }
                    }
                }

void render() {
    SDL_RenderClear(renderer);

    if (gameState == MENU) {
        renderTextButtons(menuButtons);
    } else if (gameState == PAUSED) {
        SDL_RenderCopy(renderer, pauseGameTexture, nullptr, nullptr);
        renderTextButtons(pauseButtons);
    } else if (gameState == GAME_OVER) {
        SDL_RenderCopy(renderer, gameOverTexture, nullptr, nullptr);
        renderTextButtons(gameOverButtons);
    } else if (gameState == SETTINGS) {
        SDL_RenderCopy(renderer, settingsTexture, nullptr, nullptr);
        renderTextButtons(settingsButtons);
        SDL_RenderPresent(renderer);
    }  else if (gameState == TUTORIAL) {
    SDL_RenderClear(renderer);
    if (tutorialTexture) SDL_RenderCopy(renderer, tutorialTexture, nullptr, nullptr);

    // Vẽ từng dòng chữ hướng dẫn
    SDL_Color white = {255, 255, 255, 255};
    int y = 240;
    for (const auto& line : tutorialLines) {
        SDL_Texture* txt = renderText(line, white);
        if (txt) {
            int w, h;
            SDL_QueryTexture(txt, nullptr, nullptr, &w, &h);
            SDL_Rect dst = { (SCREEN_WIDTH - w) / 2, y, w, h };
            SDL_RenderCopy(renderer, txt, nullptr, &dst);
            SDL_DestroyTexture(txt);
        }
        y += 60;
    }

    renderTextButtons(tutorialButtons);
    SDL_RenderPresent(renderer);
        }   else {
        // Vẽ background
        if (backgroundStart) SDL_RenderCopy(renderer, backgroundStart, nullptr, nullptr);

        // Vẽ lava nếu cần
        if (lavaActive && lavaTexture && lavaY < SCREEN_HEIGHT) {
            int h = SCREEN_HEIGHT - lavaY;
            if (h > 0) {
                SDL_Rect dst = {0, lavaY, SCREEN_WIDTH, h};
                SDL_RenderCopy(renderer, lavaTexture, nullptr, &dst);
            }
        }

        // Vẽ các block
        for (auto& b : blocks) {
            SDL_Rect dst = {b.x, b.y, b.w, b.h};
            SDL_RenderCopy(renderer, blockTextures[b.type], nullptr, &dst);
        }


        // Vẽ nhân vật
        if (showPlayer && currentPlayerTexture) {
            SDL_Rect playerRect = {
                player.x - camera.x,
                player.y - camera.y,
                player.w = 64,
                player.h = 64,
            };
            SDL_RenderCopy(renderer, currentPlayerTexture, nullptr, &playerRect);
        }


    if (isPausedForHit && countdownNumber > 0) {
    std::string text = std::to_string(countdownNumber);
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, text.c_str(), white);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    int textW, textH;
    SDL_QueryTexture(textTexture, nullptr, nullptr, &textW, &textH);
    SDL_Rect dstRect;
    dstRect.w = textW * 3; // phóng to 3 lần cho lớn
    dstRect.h = textH * 3;
    dstRect.x = (SCREEN_WIDTH - dstRect.w) / 2;
    dstRect.y = (SCREEN_HEIGHT - dstRect.h) / 2;
    SDL_RenderCopy(renderer, textTexture, nullptr, &dstRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
}

        // Vẽ trái tim mạng sống
        const int MAX_LIVES = 3;
        int heartSize = 32;
        int padding = 10;

        for (int i = 0; i < MAX_LIVES; ++i) {
            SDL_Texture* currentHeart = (i < player.lives) ? heartTexture : heartEmptyTexture;

            SDL_Rect heartRect = {
                SCREEN_WIDTH - (MAX_LIVES - i) * (heartSize + padding),
                10,
                heartSize,
                heartSize
            };

            SDL_RenderCopy(renderer, currentHeart, nullptr, &heartRect);
        }

        //  VẼ NÚT PAUSE Ở GIỮA TRÊN CÙNG MÀN HÌNH
        if (pauseButtonTexture) {
            SDL_RenderCopy(renderer, pauseButtonTexture, nullptr, &pauseButtonRect);
        }

        // Hiển thị thời gian sinh tồn, kỷ lục...
        renderSurvivalTimes();
    }

    SDL_RenderPresent(renderer);
}



void cleanUp() {
    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(backgroundStart);
    for (int i = 0; i < 3; ++i) {
    if (blockTextures[i]) {
        SDL_DestroyTexture(blockTextures[i]);
        blockTextures[i] = nullptr;
        }
    }
    SDL_DestroyTexture(lavaTexture);
    Mix_FreeChunk(jumpSound);
    Mix_FreeChunk(hitSound);
    Mix_FreeChunk(gameOverSound);
    Mix_FreeMusic(backgroundMusic);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_Quit();
    SDL_Quit();
}


int main(int argc, char* argv[]) {
    srand(static_cast<unsigned int>(time(0)));
    if (!initSDL()) return -1;
    loadMedia();
    resetGame();
    initTextButtons();
    bestSurvivalTime = loadBestTime();
    bool running = true;
    while (running) {
        handleEvents(running);
        switch (gameState) {
            case MENU:
                SDL_RenderClear(renderer);
                if (menuTexture) {
                SDL_RenderCopy(renderer, menuTexture, nullptr, nullptr);
                }
                renderTextButtons(menuButtons);
                SDL_RenderPresent(renderer);
                break;
            case SETTINGS:
                render();
                break;
            case TUTORIAL:
                render();
                break;
            case PLAYING:
                update();
                render();
                break;
            case PAUSED:
                render();
                break;
            case GAME_OVER:
                render();
                break;
        }

        SDL_Delay(16); // ~60 FPS
    }
    cleanUp();
    return 0;
}
