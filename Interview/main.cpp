// Robot Game

// installed newer versions of SDL and SDL_image
#include "SDL.h" 
#include "SDL_image.h"
#include "SDL_ttf.h" // used ttf to render text.
#include <iostream>
#include <string>


enum eSpriteIds
{
    ID_EMPTY = -1,
    ID_PLAYER = 0,
    ID_AI = 1,
    ID_HOLE = 2,
    ID_BOMB = 3,
    ID_COUNT
};

// Used to determine position of on-screen text
enum eDrawType
{
    E_Score,
    E_Lives,
    E_WinLoss
};

struct sSprite
{
    const char* path;
    SDL_Texture* texture;
    int w, h;
};

sSprite gSprites[ID_COUNT] = {
    {"assets\\Player\\p1_stand.png", nullptr, 0, 0},
    {"assets\\Enemies\\slimeWalk.png", nullptr, 0, 0},
    {"assets\\Tiles\\liquidWaterTop.png", nullptr, 0, 0},
    {"assets\\Items\\bomb.png", nullptr, 0, 0}
};

// Initialise pointers, could lead to undefined behaviour if not
SDL_Texture* gTexMissing = nullptr;
SDL_Renderer* gRenderer = nullptr;
SDL_Window* gWindow = nullptr;

int gScore = 0;
// added player lives variable
int playerLives = 5;

const int TILE_W = 40;
const int TILE_H = 40;
const int TILES_X = 20;
const int TILES_Y = 16;
const int MAX_AI = TILES_X >> 1;

bool quitGame = false;
bool wonGame = false;
bool gameOver = false;

eSpriteIds gWorld[TILES_Y][TILES_X];
SDL_Point gAIPos[MAX_AI];
SDL_Point gPlayerPos;

TTF_Font* gFont = nullptr;

void closeSDL();
void loadSprites();
void initWorld();
void movePlayer(SDL_Event& event);
bool moveAI();

void drawWorld();
void initFont();
void closeFont();
void drawText(std::string text, eDrawType type);

SDL_Texture* loadTexture(const char* filepath, int* w, int* h);

// TODO Loads!
int main(int argc, char* args[])
{
    // initialise ttf (fonts)
    if (TTF_Init() != 0)
    {
        fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
        return -1;   // Handle initialization error
    }

    // freopen("stdout.log", "w", stdout);
    // freopen("stderr.log", "w", stderr);

    int isOK = SDL_Init(SDL_INIT_EVERYTHING);
    if (isOK != 0)
    {
        return -1;
    }

    gWindow = SDL_CreateWindow("Robot Chase", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, TILE_W * TILES_X,
                               TILE_H * TILES_Y, SDL_WINDOW_SHOWN);
    if (gWindow == nullptr)
    {
        std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (gRenderer == nullptr)
    {
        SDL_DestroyWindow(gWindow);
        std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        closeSDL();
        return 2;
    }

    int na;
    SDL_Texture* texBackground = loadTexture("assets\\bg_castle.png", &na, &na);

    // Initialize the game world 
    initWorld();
    loadSprites();

    SDL_Event event;

    // while the game isn't over
    while (!quitGame)
    {
        // For player input 
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                quitGame = true;
            }
            if (event.type == SDL_KEYUP)
            {
                // allow player to quit at any time
                if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                {
                    quitGame = true;
                }
                // if the player hasn't won/lost, allow player to move
                if (!gameOver)
                {
                    movePlayer(event);
                    // gameOver = moveAI();
                }
                else // if the player has lost or won
                {
                    if (event.key.keysym.scancode == SDL_SCANCODE_R) 
                    {
                        // Reset game state and variables
                        initWorld();
                    }
                }
            }
        }
        // Wipes previous frames content from screen, preparing it for current frame
        SDL_RenderClear(gRenderer);

        // render the background before other elements, so later elements will appear on top
        SDL_RenderCopy(gRenderer, texBackground, NULL, NULL);
        
        drawWorld();
        // draw score and lives using drawText
        drawText("Score: ",E_Score);
        drawText("Lives: ",E_Lives);
        
        // if the player wins or loses
        if (gameOver)
        {
            // draw loss screen
            if (playerLives > 0)
            {
                drawText("You Win!! Press 'R' to restart.", E_WinLoss);
            }
            else
            {
                // draw loss screen
                std::cout << "Game Over! You were caught by a robot." << std::endl;
                drawText("You Lose!! Press 'R' to restart.", E_WinLoss);
            }
        }

        // this function shows the render window
        SDL_RenderPresent(gRenderer);

        // this just allows for delays betweeen inputs
        SDL_EventState(SDL_KEYUP, SDL_IGNORE);
        SDL_Delay(100);
        SDL_EventState(SDL_KEYUP, SDL_ENABLE);
    }

    closeSDL();
    return 0;
}

void loadSprites()
{
    for (int id = 0; id < ID_COUNT; id++)
    {
        gSprites[id].texture = loadTexture(gSprites[id].path, &gSprites[id].w, &gSprites[id].h);
    }
}

void initWorld()
{
    // initialise player variables
    gScore = 0;
    playerLives = 5;
    gameOver = false;

    // initialise positions
    for (int x = 0; x < TILES_X; x++)
    {
        for (int y = 0; y < TILES_Y; y++)
        {
            eSpriteIds tile = ID_EMPTY;
            int rndPercent = std::rand() % 100;
            if (rndPercent < 3)
            {
                tile = ID_HOLE;
            }
            else if (rndPercent < 7)
            {
                tile = ID_BOMB;
            }
            gWorld[y][x] = tile;
        }
    }
    for (int ai = 0; ai < MAX_AI; ai++)
    {
        int x;
        int y;
        do
        {
            x = std::rand() % TILES_X;
            y = std::rand() % TILES_Y;
        }
        while (gWorld[y][x] != ID_EMPTY);
        gWorld[y][x] = ID_AI;
        gAIPos[ai].x = x;
        gAIPos[ai].y = y;
    }

    do
    {
        gPlayerPos.x = std::rand() % TILES_X;
        gPlayerPos.y = std::rand() % TILES_Y;
    }
    while (gWorld[gPlayerPos.y][gPlayerPos.x] != ID_EMPTY);
    gWorld[gPlayerPos.y][gPlayerPos.x] = ID_PLAYER;
}

void movePlayer(SDL_Event& event)
{
    int newX = gPlayerPos.x;
    int newY = gPlayerPos.y;
    switch (event.key.keysym.scancode)
    {
    case SDL_SCANCODE_Z:
    case SDL_SCANCODE_X:
    case SDL_SCANCODE_C:
        newY++;
        break;
    case SDL_SCANCODE_Q:
    case SDL_SCANCODE_W:
    case SDL_SCANCODE_E:
        newY--;
        break;
    }
    switch (event.key.keysym.scancode)
    {
    case SDL_SCANCODE_E:
    case SDL_SCANCODE_D:
    case SDL_SCANCODE_C:
        newX++;
        break;
    case SDL_SCANCODE_Q:
    case SDL_SCANCODE_A:
    case SDL_SCANCODE_Z:
        newX--;
        break;
    }
    // want the AI to move closer to the player but don't want the player to move for 'S'
    // make sure the player can't go out of bounds of screen - can cause memory stomp
   if (newX >= 0 && newX < TILES_X && newY >= 0 && newY < TILES_Y && (gWorld[newY][newX] == ID_EMPTY || event.key.keysym.scancode == SDL_SCANCODE_S))
   {
        // if the player presses a valid key and the new space is empty
        gScore++; 
        gameOver = moveAI(); // move ai and set gameover if there are AI left
        gWorld[gPlayerPos.y][gPlayerPos.x] = ID_EMPTY;
        gWorld[newY][newX] = ID_PLAYER;
        gPlayerPos.x = newX;
        gPlayerPos.y = newY;
   }
}

bool moveAI()
{
    int aiCount = 0;
    for (int ai = 0; ai < MAX_AI; ai++)
    {
        if (gAIPos[ai].x >= 0) // this checks if the ai is active, inactive if = -1
        {
            SDL_Point newPos = gAIPos[ai];
            if (gPlayerPos.x > gAIPos[ai].x)
            {
                newPos.x++;
            }
            else if (gPlayerPos.x < gAIPos[ai].x)
            {
                newPos.x--;
            }
            if (gPlayerPos.y > gAIPos[ai].y)
            {
                newPos.y++;
            }
            else if (gPlayerPos.y < gAIPos[ai].y)
            {
                newPos.y--;
            }
            gWorld[gAIPos[ai].y][gAIPos[ai].x] = ID_EMPTY; // sets the current position to empty (as they're about to move)
            switch (gWorld[newPos.y][newPos.x])
            {
            case ID_BOMB:
                gWorld[newPos.y][newPos.x] = ID_EMPTY;
                gAIPos[ai].x = gAIPos[ai].y = -1;
                gScore += 10;
                break;
            case ID_HOLE:
            case ID_AI:
                gAIPos[ai].x = gAIPos[ai].y = -1;
                gScore += 10;
                break;
            case ID_PLAYER:
                // TODO
                // Reduce player lives and reset game. If the player has ran out of lives
                gAIPos[ai].x = gAIPos[ai].y = -1; // upon clashing with the player, the robot kills itself and takes a life from the player
                gScore -= 10; // the robot will take 10 points from the player if they clash
                playerLives--;
                if (playerLives <= 0)
                {
                    // this sets gameover = true
                    return true;
                }
                break;
            default:
                aiCount++;
                gWorld[newPos.y][newPos.x] = ID_AI;
                gAIPos[ai] = newPos;
            }
        }
    }
    return aiCount == 0;
}

void drawWorld()
{
    SDL_Rect srcRect = {0, 0, 0, 0};
    SDL_Rect dstRect;
    for (int x = 0; x < TILES_X; x++)
    {
        for (int y = 0; y < TILES_Y; y++)
        {
            eSpriteIds content = gWorld[y][x];
            if (content >= 0)
            {
                dstRect.x = x * TILE_W + (TILE_W >> 1);
                dstRect.y = y * TILE_H + (TILE_H >> 1);
                srcRect.w = gSprites[content].w;
                dstRect.w = srcRect.w >> 1;
                srcRect.h = gSprites[content].h;
                dstRect.h = srcRect.h >> 1;
                dstRect.x -= dstRect.w >> 1;
                dstRect.y -= dstRect.h >> 1;
                SDL_RenderCopy(gRenderer, gSprites[content].texture, &srcRect, &dstRect);
            }
        }
    }
}

void initFont()
{
    gFont = TTF_OpenFont("assets\\Fonts\\Browood-Regular.ttf", 24);
    if (gFont == nullptr) {
        std::cerr << "Error opening font: " << TTF_GetError() << std::endl;
    }
}

void closeFont() {
    if (gFont != nullptr) {
        TTF_CloseFont(gFont);
        gFont = nullptr;
    }
}

void drawText(std::string text, eDrawType type)
{
    // if display score or lives, append the value
    if (type == E_Score)
    {
        text += std::to_string(gScore);
    }
    if (type == E_Lives)
    {
        text += std::to_string(playerLives);
    }

    //// load font (I found a pre-made font online: https://alitdesign.net/product/browood-layered-comic-font/)
    //TTF_Font* font = TTF_OpenFont("assets\\Fonts\\Browood-Regular.ttf", 24);
    //// null check 
    //if (font == nullptr)
    //{
    //    std::cerr << "Error opening font: " << TTF_GetError() << std::endl;
    //    return;
    //}

    initFont();

    // Create color (white)
    const SDL_Color textColour = {255, 255, 255};

    // create text surface with null check
    SDL_Surface* textSurface = TTF_RenderText_Solid(gFont, text.c_str(), textColour);
    if (textSurface == nullptr)
    {
        std::cerr << "Error creating text surface: " << TTF_GetError() << std::endl;
        // TTF_CloseFont(font);
        return;
    }

    // texture creation with nullcheck
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(gRenderer, textSurface);
    if (textTexture == nullptr)
    {
        std::cerr << "Error creating text texture: " << SDL_GetError() << std::endl;
        SDL_FreeSurface(textSurface);
        // TTF_CloseFont(font);
        return;
    }
    
    SDL_Rect textRect;
    // this switch statement places text in correct parts of the screen 
    switch (type)
    {
    case E_Score:
        // Define the position for the score
        textRect.x = TILE_W * TILES_X - textSurface->w - 20;
        textRect.y = 0;
        break;
    case E_Lives:
        // Define the position for the lives
        textRect.x = 20;
        textRect.y = 0;
        break;
    case E_WinLoss:
        textRect.x = (TILE_W * TILES_X - textSurface->w) / 2;
        textRect.y = (TILE_H * TILES_Y - textSurface->h) / 2;
    }
    
    textRect.w = textSurface->w;
    textRect.h = textSurface->h;
    
    SDL_RenderCopy(gRenderer, textTexture, nullptr, &textRect);

    // part of the cleanup process to avoid memory leaks
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
}

SDL_Texture* loadTexture(const char* filepath, int* w, int* h)
{
    SDL_Surface* surface = IMG_Load(filepath);
    if (surface == nullptr)
    {
        std::cout << "SDL_LoadBMP Error: " << SDL_GetError() << std::endl;
        surface = IMG_Load("missing.png");
    }
    *w = surface->w;
    *h = surface->h;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(gRenderer, surface);
    SDL_FreeSurface(surface);
    if (texture == nullptr)
    {
        std::cout << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
        return gTexMissing;
    }
    // RETURN THE LOADED TEXTURE
    return texture;
}

void closeSDL(void)
{
    for (int id = 0; id < ID_COUNT; id++)
    {
        SDL_DestroyTexture(gSprites[id].texture);
    }
    SDL_DestroyTexture(gTexMissing);
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    closeFont();
    SDL_Quit();
}
