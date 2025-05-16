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
const int GRAVITY = 2;
const int PLAYER_SPEED = 5;
int spawnTimer = 0;
int BLOCK_FALL_SPEED = 3;
const int BLOCK_WIDTH = 50;
const int BLOCK_HEIGHT = 50;
int MAX_BLOCKS = 20;
int timeSinceStart = 0;
int lastSpeedUpTime = 0;
int speedUpCount = 0;
int survivalTime = 0;
int bestSurvivalTime = 0;
bool isNewRecord = false;
Uint32 recordTimerStart = 0;
const Uint32 recordAnimDuration = 3000;

bool showPlayer = true;
SDL_Texture* gameOverTexture = nullptr;
SDL_Texture* menuTexture = nullptr;
enum GameState { MENU, PLAYING, PAUSED, GAME_OVER };
GameState gameState = MENU;

TTF_Font* font = nullptr;
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* player_stand_left = nullptr;
SDL_Texture* player_stand_right = nullptr;
SDL_Texture* player_jump_left = nullptr;
SDL_Texture* player_jump_right = nullptr;
SDL_Texture* player_run_left_slow = nullptr;
SDL_Texture* player_run_left_fast = nullptr;
SDL_Texture* player_run_right_slow = nullptr;
SDL_Texture* player_run_right_fast = nullptr;
SDL_Texture* playerTexture = nullptr;
SDL_Texture* background = nullptr;
SDL_Texture* blockTexture = nullptr;
SDL_Texture* heartTexture = nullptr;
SDL_Texture* heartEmptyTexture = nullptr;
SDL_Texture* levelUpBanner = nullptr;
SDL_Texture* currentPlayerTexture = nullptr;
Mix_Chunk* jumpSound = nullptr;
Mix_Chunk* hitSound = nullptr;
Mix_Chunk* gameOverSound = nullptr;
Mix_Chunk* levelUpSound = nullptr;
Mix_Music* backgroundMusic = nullptr;


Uint32 levelUpStartTime = 0;
bool showLevelUp = false;
int frame = 0;
int frameDelay = 4;
int currentRunFrame = 0;
int maxPlayerHeight = SCREEN_HEIGHT;
int cameraY = 0;


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
    SDL_Texture* countdownTextures[3] = {nullptr, nullptr, nullptr};
} player;

struct Block {
    int x, y, w, h;
    bool isLanded;
};

std::vector<Block> blocks;
bool gameOver = false;
int score = 0;
bool isGameStarted = false;

bool initSDL() {

    if (TTF_Init() == -1) {
    std::cout << "Failed to initialize SDL_ttf: " << TTF_GetError() << std::endl;
    return false;
    }
    font = TTF_OpenFont("arial.ttf", 24);
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

Mix_Chunk* loadSound(const char* path) {
    return Mix_LoadWAV(path);
}

Mix_Music* loadMusic(const char* path) {
    return Mix_LoadMUS(path);
}

void loadMedia() {
    menuTexture = loadTexture("menu.png");
    gameOverTexture = loadTexture("gameover.png");
    player_stand_left      = loadTexture("player_stand_left.png");
    player_stand_right     = loadTexture("player_stand_right.png");
    player_jump_left       = loadTexture("player_jump_left.png");
    player_jump_right      = loadTexture("player_jump_right.png");
    player_run_left_slow    = loadTexture("player_run_left_slow.png");
    player_run_left_fast    = loadTexture("player_run_left_fast.png");
    player_run_right_slow   = loadTexture("player_run_right_slow.png");
    player_run_right_fast   = loadTexture("player_run_right_fast.png");
    background = loadTexture("background.png");
    blockTexture = loadTexture("block.png");
    heartTexture = loadTexture("heart.png");
    heartEmptyTexture = loadTexture("heart_empty.png");
    levelUpBanner = loadTexture("levelup.png");
    jumpSound = loadSound("jump.wav");
    hitSound = loadSound("hit.wav");
    gameOverSound = loadSound("gameover.wav");
    levelUpSound = loadSound("levelup.wav");
    backgroundMusic = loadMusic("background.mp3");
    Mix_PlayMusic(backgroundMusic, -1);
}

void spawnBlock() {
      int numColumns = SCREEN_WIDTH / BLOCK_WIDTH;

    // Chọn vị trí cột ngẫu nhiên
    int col = rand() % numColumns;

    Block newBlock;
    newBlock.w = BLOCK_WIDTH;
    newBlock.h = BLOCK_HEIGHT;
    newBlock.x = col * BLOCK_WIDTH;
    newBlock.y = -BLOCK_HEIGHT; // bắt đầu rơi từ trên màn hình
    newBlock.isLanded = false;
    blocks.push_back(newBlock);
}
void resetGame() {
    // Đặt lại vị trí và kích thước player
    player.x = SCREEN_WIDTH / 2 - player.w / 2;
    player.y = SCREEN_HEIGHT - player.h - 100;  // đứng trên mặt đất
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
    score = 0;
    survivalTime = 0;
    spawnTimer = 0;
    BLOCK_FALL_SPEED = 3;
    MAX_BLOCKS = 20;

    // Reset camera hoặc các biến liên quan khác (nếu có)
    // cameraY = 0;  // nếu có biến này

    // Nếu có hàm cập nhật player (updatePlayer), gọi ở đây
    // updatePlayer(player);

    // Phát lại nhạc nền nếu cần
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
                if (e.key.keysym.sym == SDLK_LEFT) player.velocityX = -PLAYER_SPEED;
                if (e.key.keysym.sym == SDLK_RIGHT) player.velocityX = PLAYER_SPEED;

                if (e.key.keysym.sym == SDLK_UP) {
                    // Nhảy khi chưa nhảy
                    if (!player.isJumping) {
                        player.velocityY = -20;
                        player.isJumping = true;
                        Mix_PlayChannel(-1, jumpSound, 0);
                    }
                }

                if (e.key.keysym.sym == SDLK_SPACE) {
                    // Bám mép khi đang nhảy
                    if (player.isJumping) {
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
                }

                if (e.key.keysym.sym == SDLK_p) {
                    gameState = PAUSED;
                }
            } else if (gameState == GAME_OVER) {
                if (e.key.keysym.sym == SDLK_r) {
                    resetGame();
                    gameState = PLAYING;
                }
            } else if (gameState == PAUSED) {
                if (e.key.keysym.sym == SDLK_p) {
                    gameState = PLAYING;
                }
            }

            if (e.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
        } else if (e.type == SDL_KEYUP) {
            if (gameState == PLAYING) {
                if (e.key.keysym.sym == SDLK_LEFT || e.key.keysym.sym == SDLK_RIGHT)
                    player.velocityX = 0;
            }
        }
    }
}

void update() {
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
        currentPlayerTexture = (currentRunFrame == 0) ? player_run_left_slow : player_run_left_fast;
        player.lastFacingLeft = true;
    } else if (player.velocityX > 0) {
        frame++;
        if (frame >= frameDelay * 2) frame = 0;
        currentRunFrame = frame / frameDelay;
        currentPlayerTexture = (currentRunFrame == 0) ? player_run_right_slow : player_run_right_fast;
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

        } else {
            player.pauseAfterHit = true;
            player.pauseStartTime = SDL_GetTicks();
        }
    } else if (!crushed) {
        player.wasCrushed = false;
    }

    player.wasOnBlock = currentlyOnBlock;

    if (survivalTime % 1800 == 0 && survivalTime != 0) {
        BLOCK_FALL_SPEED = static_cast<int>(BLOCK_FALL_SPEED * 1.5);
        static int increaseCount = 0;
        if (increaseCount < 5) {
            MAX_BLOCKS += 5;
            increaseCount++;
        }
    }

    if (rand() % 100 < 3) spawnBlock();

const int cameraThreshold = BLOCK_HEIGHT * 4; // 4 block

if (player.y < cameraY + cameraThreshold) {
    cameraY = player.y - cameraThreshold;

    // Không để cameraY âm
    if (cameraY < 0) cameraY = 0;
}

    survivalTime++;

if (survivalTime > bestSurvivalTime) {
    if (!isNewRecord) {
        isNewRecord = true;
        recordTimerStart = SDL_GetTicks();
    }
    bestSurvivalTime = survivalTime;
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
        if (menuTexture) {
            SDL_RenderCopy(renderer, menuTexture, nullptr, nullptr);
        }
    } else if (gameState == GAME_OVER) {
        if (gameOverTexture) {
            SDL_RenderCopy(renderer, gameOverTexture, nullptr, nullptr);
        }
    } else {
        // Vẽ background
        if (background) SDL_RenderCopy(renderer, background, nullptr, nullptr);

        // Vẽ các block (phải trừ cameraY để cuộn theo nhân vật)
        for (const auto& block : blocks) {
            SDL_Rect rect = {
                block.x,
                block.y - cameraY, // trừ cameraY để cuộn
                block.w,
                block.h
            };
            SDL_RenderCopy(renderer, blockTexture, nullptr, &rect);
        }

        // Hiệu ứng đếm ngược sau khi bị đè
        if (player.pauseAfterHit) {
            Uint32 elapsed = SDL_GetTicks() - player.pauseStartTime;
            int second = 3 - elapsed / 1000;
            if (second >= 1 && second <= 3 && player.countdownTextures[second - 1]) {
                SDL_Rect dst = {SCREEN_WIDTH / 2 - 64, SCREEN_HEIGHT / 2 - 64, 128, 128};
                SDL_RenderCopy(renderer, player.countdownTextures[second - 1], nullptr, &dst);
            }
        }

        // Vẽ nhân vật
        if (showPlayer && currentPlayerTexture) {
            SDL_Rect playerRect = {
                player.x,
                player.y - cameraY, // trừ cameraY để cuộn
                player.w,
                player.h
            };
            SDL_RenderCopy(renderer, currentPlayerTexture, nullptr, &playerRect);
        }

        // Hiển thị trái tim mạng sống (HUD — không trừ cameraY)
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

        // Hiển thị chữ: thời gian sinh tồn, kỷ lục,...
        renderSurvivalTimes();
    }

    SDL_RenderPresent(renderer);
}


void cleanUp() {
    SDL_DestroyTexture(playerTexture);
    SDL_DestroyTexture(background);
    SDL_DestroyTexture(blockTexture);
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
    player = {100, SCREEN_HEIGHT - 150, 50, 50, 0, 0, false, 3, false, 0, false, false};
    bool running = true;
    while (running) {
        handleEvents(running);
        update();
        render();
        SDL_Delay(16);
    }
    cleanUp();
    return 0;
}
