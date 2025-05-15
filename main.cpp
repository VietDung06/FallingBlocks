#include <SDL.h>
#include <SDL_image.h>
#include <SDL_mixer.h>
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

bool showPlayer = true;
SDL_Texture* gameOverTexture = nullptr;
SDL_Texture* menuTexture = nullptr;
SDL_Texture* scoreBoardTexture = nullptr;
enum GameState { MENU, PLAYING, PAUSED, GAME_OVER };
GameState gameState = MENU;

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
int survivalTime = 0;
bool isGameStarted = false;
std::vector<int> highScores;

bool initSDL() {
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

Mix_Chunk* loadSound(const char* path) {
    return Mix_LoadWAV(path);
}

Mix_Music* loadMusic(const char* path) {
    return Mix_LoadMUS(path);
}

void loadMedia() {
    menuTexture = loadTexture("menu.png");
    gameOverTexture = loadTexture("gameover.png");
    scoreBoardTexture = loadTexture("scoreBoardTexture.png");
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
    levelUpBanner = loadTexture("levelup.png");
    jumpSound = loadSound("jump.wav");
    hitSound = loadSound("hit.wav");
    gameOverSound = loadSound("gameover.wav");
    levelUpSound = loadSound("levelup.wav");
    backgroundMusic = loadMusic("background.mp3");
    Mix_PlayMusic(backgroundMusic, -1);
}

void loadHighScores() {
    std::ifstream file("highscores.txt");
    int score;
    while (file >> score) {
        highScores.push_back(score);
    }
}

void saveHighScores() {
    std::ofstream file("highscores.txt");
    for (int score : highScores) {
        file << score << std::endl;
    }
}
void spawnBlock() {
    if ((int)blocks.size() >= MAX_BLOCKS) return;
    Block newBlock;
    newBlock.w = BLOCK_WIDTH;
    newBlock.h = BLOCK_HEIGHT;
    newBlock.x = (rand() % (SCREEN_WIDTH / BLOCK_WIDTH)) * BLOCK_WIDTH;
    newBlock.y = -BLOCK_HEIGHT;
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
                    // Bấm space khi ở menu để bắt đầu chơi
                    resetGame();        // Reset trạng thái game
                    gameState = PLAYING; // Chuyển trạng thái sang chơi
                }
            } else if (gameState == PLAYING) {
                // Các phím điều khiển game khi đang chơi
                if (e.key.keysym.sym == SDLK_LEFT) player.velocityX = -PLAYER_SPEED;
                if (e.key.keysym.sym == SDLK_RIGHT) player.velocityX = PLAYER_SPEED;
                if (e.key.keysym.sym == SDLK_SPACE && !player.isJumping) {
                    player.velocityY = -20;
                    player.isJumping = true;
                    Mix_PlayChannel(-1, jumpSound, 0);
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

            // Thoát game khi nhấn ESC
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

    score++;
    survivalTime++;

    player.velocityY += GRAVITY;
    player.y += player.velocityY;
    player.x += player.velocityX;

    if (player.y >= SCREEN_HEIGHT - player.h - 100) {
        player.y = SCREEN_HEIGHT - player.h - 100;
        player.isJumping = false;
    }
    if (player.x < 0) player.x = 0;
    if (player.x + player.w > SCREEN_WIDTH) player.x = SCREEN_WIDTH - player.w;

    bool currentlyOnBlock = false;
    bool crushed = false;

    for (auto& block : blocks) {
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

        if (player.y + player.h > block.y && player.y + player.h - player.velocityY <= block.y &&
            player.x + player.w > block.x && player.x < block.x + block.w) {
            player.y = block.y - player.h;
            player.velocityY = 0;
            player.isJumping = false;
            currentlyOnBlock = true;
        }

        if (player.x + player.w > block.x && player.x < block.x + block.w &&
            player.y < block.y + block.h && player.y + player.h > block.y) {
            if (player.velocityX > 0) {
                player.x = block.x - player.w;
            } else if (player.velocityX < 0) {
                player.x = block.x + block.w;
            }
        }
   }

    if (crushed && !player.wasCrushed) {
        player.lives--;
        Mix_PlayChannel(-1, hitSound, 0);
        player.wasCrushed = true;
        if (player.lives <= 0) {
            Mix_PlayChannel(-1, gameOverSound, 0);
            Mix_HaltMusic();
            gameOver = true;
            if (highScores.size() < 5 || score > highScores.back()) {
                highScores.push_back(score);
                std::sort(highScores.rbegin(), highScores.rend());
                if (highScores.size() > 5) highScores.pop_back();
                saveHighScores();
            }
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
        if (scoreBoardTexture) {
            SDL_Rect scoreRect = {SCREEN_WIDTH / 2 - 100, SCREEN_HEIGHT / 2, 200, 50};
            SDL_RenderCopy(renderer, scoreBoardTexture, nullptr, &scoreRect);
        }
    } else {
        if (background) SDL_RenderCopy(renderer, background, nullptr, nullptr);

        if (player.pauseAfterHit) {
            Uint32 elapsed = SDL_GetTicks() - player.pauseStartTime;
            int second = 3 - elapsed / 1000;
            if (second >= 1 && second <= 3 && player.countdownTextures[second - 1]) {
                SDL_Rect dst = {SCREEN_WIDTH / 2 - 64, SCREEN_HEIGHT / 2 - 64, 128, 128};
                SDL_RenderCopy(renderer, player.countdownTextures[second - 1], nullptr, &dst);
            }
        }

        if (showPlayer && currentPlayerTexture) {
            SDL_Rect playerRect = { player.x, player.y, player.w, player.h };
            SDL_RenderCopy(renderer, currentPlayerTexture, nullptr, &playerRect);
        }

        for (const auto& block : blocks) {
            SDL_Rect rect = { block.x, block.y, block.w, block.h };
            SDL_RenderCopy(renderer, blockTexture, nullptr, &rect);
        }

        if (scoreBoardTexture) {
            SDL_Rect scoreRect = {10, 10, 100, 50};
            SDL_RenderCopy(renderer, scoreBoardTexture, nullptr, &scoreRect);
        }
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
    loadHighScores();
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
