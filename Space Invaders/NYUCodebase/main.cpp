#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include "Matrix.h"
#include "ShaderProgram.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <cassert>
#include <vector>


#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#define RESOURCE "/Users/shayaansaiyed/Documents/School/Game/CS3113Homework/Space Invaders/NYUCodebase/"
#endif

SDL_Window* displayWindow;

enum GameState { STATE_START_SCREEN, STATE_GAME, STATE_LOST };
enum EntityType {PLAYER, ENEMY, BULLET};




//-------------VARIABLES-------------

//      STATE MANAGER
int state;

//      SETUP MATRIX
Matrix projectionMatrix;
Matrix modelMatrix;
Matrix orthographicMatrix;

//      PHYSICS
float friction_x = 10.0;

//      TIME
float lastFrameTicks = 0.0f;

//      LOAD SPRITE SHEET
int spriteSheetTexture;
int fontTexture;

//      SCORE
int score = 0;


//--------------CLASS DEFINITIONS-----------------

//      VECTOR 3 CLASS
class Vector3 {
    public:
    Vector3 (float x, float y, float z);
    Vector3(){};
    float x = 0.0;
    float y = 0.0;
    float z = 0.0;
};

//      NON-UNIFORM SPRITE SHEET
class SheetSprite {
    public:
    SheetSprite(){};
    SheetSprite(unsigned int textureID, float u, float v, float width, float height, float size):textureID(textureID), u(u), v(v), width(width), height(height), size(size) {
        aspect = width/height;
    };
    SheetSprite(unsigned int textureID, int index, float size, int spriteCountX, int spriteCountY){
        
    };
    
    void Draw(ShaderProgram program){
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        GLfloat texCoords[] = {
            u, v+height,
            u+width, v,
            u, v,
            u+width, v,
            u, v+height,
            u+width, v+height
        };
        
        
        float vertices[] = {
            -0.5f * size * aspect, -0.5f * size,
            0.5f * size * aspect, 0.5f * size,
            -0.5f * size * aspect, 0.5f * size,
            0.5f * size * aspect, 0.5f * size,
            -0.5f * size * aspect, -0.5f * size ,
            0.5f * size * aspect, -0.5f * size
        };
        
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(program.positionAttribute);
        
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(program.texCoordAttribute);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    
    float aspect;
    float size;
    unsigned int textureID;
    float u;
    float v;
    float width;
    float height;
};

//      ENTITY CLASS
class Entity{
    public:
    Entity(){};
    Vector3 position;
    float rotation;
    GLuint textureID;
    float width;
    float height;
    Vector3 velocity;
    Vector3 acceleration;
    SheetSprite sprite;
    float top;
    float bottom;
    float left;
    float right;
    void getTopBottomLeftRight(){
        height = sprite.size;
        width = sprite.size * sprite.aspect;
        top = position.y + height/2;
        bottom = position.y - height/2;
        left = position.x - width/2;
        right = position.x + width/2;
    }
};




//-------------FUNCTION DEFINITIONS-----------------

//      LOAD TEXTURES
GLuint LoadTexture(const char *filePath) {
    int w,h,comp;
    unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
    
    if(image == NULL) {
        std::cout << "Unable to load image. Make sure the path is correct\n";
        assert(false);
    }
    
    GLuint retTexture;
    glGenTextures(1, &retTexture);
    glBindTexture(GL_TEXTURE_2D, retTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    stbi_image_free(image);
    return retTexture;
}

//      LERP FUNCTION
float lerp(float v0, float v1, float t) {
    return (1.0-t)*v0 + t*v1;
}

//      DRAW TEXT FUNCTION
void DrawText(ShaderProgram program, int fontTexture, std::string text, float size, float spacing) {
    float texture_size = 1.0/16.0f;
    std::vector<float> vertexData;
    std::vector<float> texCoordData;
    for(int i=0; i < text.size(); i++) {
        int spriteIndex = (int)text[i];
        float texture_x = (float)(spriteIndex % 16) / 16.0f;
        float texture_y = (float)(spriteIndex / 16) / 16.0f;
        vertexData.insert(vertexData.end(), {
            ((size+spacing) * i) + (-0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (-0.5f * size), -0.5f * size,
            ((size+spacing) * i) + (0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (0.5f * size), -0.5f * size,
            ((size+spacing) * i) + (0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (-0.5f * size), -0.5f * size,
        });
        texCoordData.insert(texCoordData.end(), {
            texture_x, texture_y,
            texture_x, texture_y + texture_size,
            texture_x + texture_size, texture_y,
            texture_x + texture_size, texture_y + texture_size,
            texture_x + texture_size, texture_y,
            texture_x, texture_y + texture_size,
        }); }
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glEnableVertexAttribArray(program.positionAttribute);
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
    glEnableVertexAttribArray(program.texCoordAttribute);
    
    glDrawArrays(GL_TRIANGLES, 0, 6 * (int)text.size());
    // draw this data (use the .data() method of std::vector to get pointer to data)
}

//      UNIFORM SPRITE SHEET DRAWER
void DrawSpriteSheetSprite(ShaderProgram program, int index, int spriteCountX, int spriteCountY) {
    spriteSheetTexture = LoadTexture(RESOURCE"characters_1.png");
    
    float u = (float)(((int)index) % spriteCountX) / (float) spriteCountX;
    float v = (float)(((int)index) / spriteCountX) / (float) spriteCountY;
    float spriteWidth = 1.0/(float)spriteCountX;
    float spriteHeight = 1.0/(float)spriteCountY;
    GLfloat texCoords[] = {
        u, v+spriteHeight,
        u+spriteWidth, v,
        u, v,
        u+spriteWidth, v,
        u, v+spriteHeight,
        u+spriteWidth, v+spriteHeight
    };
    float vertices[] = {-0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f,  -0.5f,
        -0.5f, 0.5f, -0.5f};
    
    glBindTexture(GL_TEXTURE_2D, spriteSheetTexture);
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program.positionAttribute);
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program.texCoordAttribute);
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

std::vector<std::string> letters = {"0","1","2","3","4","5","6","7","8","9"};

std::string convertIntToString(int score){
    std::string str = "";
    int i = 0;
    while (score != 0){
        i = score % 10;
        str += letters[i];
        score /= 10;
    }
    return str;
}


//-------------SETUP----------------

//      ENTITIES AND USER
Entity player;

//      BULLETS
#define MAX_BULLETS 30
Entity bullets[MAX_BULLETS];
int bulletIndex = 0;


//      ENEMIES
#define MAX_ENEMIES 20
Entity enemies[MAX_ENEMIES];

//      SETUP BULLETS
void setupBullets(){
    for(int i=0; i < MAX_BULLETS; i++) {
        bullets[i].sprite = SheetSprite(spriteSheetTexture, 858.0/1024.0, 475.0/1024.0, 9.0/1024.0, 37.0/1024.0, 0.5);
        bullets[i].position.x = -2000.0f;
    }
}

//      SETUP ENEMIES
void setupEnemies(){
    int posInRow = 0;
    int row = 0;
    for(int i=0; i < MAX_ENEMIES; i++) {
        if (posInRow == 0){
            enemies[i].sprite = SheetSprite(spriteSheetTexture, 444.0/1024.0, 0.0/1024.0, 91.0/1024.0, 91.0/1024.0, 0.5);
            enemies[i].position.x = -2.0;
        }
        if (posInRow == 1){
            enemies[i].sprite = SheetSprite(spriteSheetTexture, 444.0/1024.0, 0.0/1024.0, 91.0/1024.0, 91.0/1024.0, 0.5);
            enemies[i].position.x = -1.0;
        }
        if (posInRow == 2){
            enemies[i].sprite = SheetSprite(spriteSheetTexture, 444.0/1024.0, 0.0/1024.0, 91.0/1024.0, 91.0/1024.0, 0.5);
            enemies[i].position.x = 1.0;
        }
        if (posInRow == 3){
            enemies[i].sprite = SheetSprite(spriteSheetTexture, 444.0/1024.0, 0.0/1024.0, 91.0/1024.0, 91.0/1024.0, 0.5);
            enemies[i].position.x = 2.0;
        }
        enemies[i].position.y = 1.75 + row;
        enemies[i].velocity.y = -0.25;
        if (row % 2 == 0){
            enemies[i].velocity.x = 0.75;
        }
        if (row % 2 == 1){
            enemies[i].velocity.x = -0.75;
        }
        posInRow++;
        if (posInRow == 4){
            row++;
            posInRow = 0;
        }
    }
}

//      SETUP PLAYERS
void setupPlayer(){
    player.position.x = 0.0;
    player.position.y = -1.5;
    player.position.z = 0.0;
    
    player.sprite = SheetSprite(spriteSheetTexture, 211.0/1024.0, 941.0/1024.0, 99.0/1024.0, 75.0/1024.0, 0.5);
}

//      MAIN SETUP FUNCTION
ShaderProgram Setup(){
    
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
    glewInit();
#endif
    
    ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    glViewport(0, 0, 640, 360);
    
    glUseProgram(program.programID);
    
    projectionMatrix.setOrthoProjection(-3.70, 3.70, -2.0f, 2.0f, -1.0f, 1.0f);
    
    program.setProjectionMatrix(projectionMatrix);
    program.setViewMatrix(orthographicMatrix);
    
    spriteSheetTexture = LoadTexture(RESOURCE_FOLDER"sheet.png");
    fontTexture = LoadTexture(RESOURCE_FOLDER"pixel_font.png");
    
    setupPlayer();
    setupBullets();
    setupEnemies();
    
    state = STATE_START_SCREEN;
    
    return program;
}




//-------------PROCESS INPUT---------------------
void shootBullet();

void ProcessInputGame(bool& done){
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            done = true;
        }
        else if(event.type == SDL_KEYDOWN) {
            if(event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
                shootBullet();
            }
        }
    }
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_RIGHT]){
        player.velocity.x += 0.5;
    }
    if (keys[SDL_SCANCODE_LEFT]){
        player.velocity.x -= 0.5;
    }

}

void ProcessInputStartScreen(bool&done){
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            done = true;
        }
    }
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_RETURN]){
        state = STATE_GAME;
    }
}

void ProcessInputLost(bool& done){
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            done = true;
        }
    }
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_RETURN]){
        done = true;
    }
}

void ProcessInput(bool& done){
    if (state == STATE_START_SCREEN){
        ProcessInputStartScreen(done);
    } else if (state == STATE_GAME){
        ProcessInputGame(done);
    } else if (state == STATE_LOST){
        ProcessInputLost(done);
    }
}




//-------------UPDATE---------------

//      SHOOT BULLET
void shootBullet(){
    bullets[bulletIndex].position.x = player.position.x;
    bullets[bulletIndex].position.y = player.position.y;
    bullets[bulletIndex].velocity.y = 3.0;
    bulletIndex++;
    if(bulletIndex > MAX_BULLETS-1) {
        bulletIndex = 0;
    }
}

void UpdatePlayerMovement(float elapsed){
    player.velocity.x = lerp(player.velocity.x, 0.0f, elapsed * friction_x);
    
    player.velocity.x += player.acceleration.x * elapsed;
    player.velocity.y += player.acceleration.y * elapsed;
    player.velocity.z += player.acceleration.z * elapsed;
    
    player.position.x += player.velocity.x * elapsed;
    player.position.y += player.velocity.y * elapsed;
    player.position.z += player.velocity.z * elapsed;
}

void UpdateBullets(float elapsed){
    for (int i = 0; i < MAX_BULLETS; i++){
        bullets[i].position.y += bullets[i].velocity.y * elapsed;
    }
}

void handleBulletEnemyCollision (Entity& bullet, Entity& enemy){
    bullet.velocity.y = 0;
    enemy.velocity.y = 0;
    
    bullet.position.x = -2000.0;
    bullet.position.y = 2000.0;
    
    //enemy.position.x = -2000.0;
    enemy.position.y = 2000.0;
    
    score++;
}

void testBulletEnemyCollision(){
    for (int i = 0; i < MAX_BULLETS; i++){
        bullets[i].getTopBottomLeftRight();
        for (int j = 0; j < MAX_ENEMIES; j++){
            enemies[j].getTopBottomLeftRight();
            if ((bullets[i].top >= enemies[j].bottom)
                && !((bullets[i].left > enemies[j].right )||( bullets[i].right < enemies[j].left))
                && (bullets[i].bottom < enemies[j].bottom)){
                handleBulletEnemyCollision(bullets[i], enemies[j]);
            }
        }
    }
}

void handleEnemyPlayerCollision(){
    state = STATE_LOST;
}

void testEnemyPlayerCollision(){
    player.getTopBottomLeftRight();
    for (int i = 0; i < MAX_ENEMIES; i++){
        enemies[i].getTopBottomLeftRight();
        if ((player.top >= enemies[i].bottom)
            && !((player.left > enemies[i].right) || (player.right < enemies[i].left))
            && (player.bottom < enemies[i].top)){
            handleEnemyPlayerCollision();
        }
    }
}

void testEnemyWinsCollision(){
    for(int i = 0; i < MAX_ENEMIES; i++){
        enemies[i].getTopBottomLeftRight();
        if (enemies[i].bottom <= -2.0){
            state = STATE_LOST;
        }
    }
}

void testEnemyCollisionWithWalls(){
    for(int i = 0; i < MAX_ENEMIES; i++){
        enemies[i].getTopBottomLeftRight();
        if (enemies[i].right >= 3.70 || enemies[i].left <= -3.70){
            for(int j = 0; j < MAX_ENEMIES; j++){
                enemies[j].velocity.x *= -1.0;
            }
        }
    }
}

void UpdateEnemies(float elapsed){
    for (int i = 0; i < MAX_ENEMIES; i++){
        enemies[i].position.y += enemies[i].velocity.y * elapsed;
        enemies[i].position.x += enemies[i].velocity.x * elapsed;
    }
}

void Update(float elapsed){
    if (state == STATE_GAME){
        testEnemyCollisionWithWalls();
        testBulletEnemyCollision();
        testEnemyPlayerCollision();
        testEnemyWinsCollision();
        UpdatePlayerMovement(elapsed);
        UpdateBullets( elapsed);
        UpdateEnemies(elapsed);
    }
}




//-------------RENDER---------------

void RenderGameState(ShaderProgram program){
    modelMatrix.identity();
    modelMatrix.Translate(player.position.x, player.position.y, player.position.z);
    program.setModelMatrix(modelMatrix);
    player.sprite.Draw(program);
    
    for (int i = 0; i < MAX_BULLETS; i++){
        modelMatrix.identity();
        modelMatrix.Translate(bullets[i].position.x, bullets[i].position.y, bullets[i].position.z);
        program.setModelMatrix(modelMatrix);
        bullets[i].sprite.Draw(program);
    }
    
    for (int i = 0; i < MAX_ENEMIES; i++){
        modelMatrix.identity();
        modelMatrix.Translate(enemies[i].position.x, enemies[i].position.y, enemies[i].position.z);
        program.setModelMatrix(modelMatrix);
        enemies[i].sprite.Draw(program);
    }
    
    modelMatrix.identity();
    modelMatrix.Translate(2.00, -1.75, 0.0);
    program.setModelMatrix(modelMatrix);
    DrawText(program, fontTexture, "Score: " + convertIntToString(score), 0.2, 0.0);
}

void RenderStartScreen(ShaderProgram program){
    modelMatrix.identity();
    modelMatrix.Translate(-2.3, 1.0, 0.0);
    program.setModelMatrix(modelMatrix);
    DrawText(program, fontTexture, "Space Invaders", 0.4, 0.00);
    
    modelMatrix.identity();
    modelMatrix.Translate(-2.3, 0.5, 0.0);
    program.setModelMatrix(modelMatrix);
    DrawText(program, fontTexture, "Instructions:", 0.15, 0.00);
    
    modelMatrix.identity();
    modelMatrix.Translate(-2.3, 0.0, 0.0);
    program.setModelMatrix(modelMatrix);
    DrawText(program, fontTexture, "-use arrow keys to move left and right", 0.15, 0.00);
    
    modelMatrix.identity();
    modelMatrix.Translate(-2.3, -0.5, 0.0);
    program.setModelMatrix(modelMatrix);
    DrawText(program, fontTexture, "-press spacebar to shoot", 0.15, 0.00);
    
    modelMatrix.identity();
    modelMatrix.Translate(-2.3, -1.0, 0.0);
    program.setModelMatrix(modelMatrix);
    DrawText(program, fontTexture, "Press Enter to begin", 0.2, 0.00);
}

void RenderLoseScreen(ShaderProgram program){
    modelMatrix.identity();
    modelMatrix.Translate(-2.5, 1.0, 0.0);
    program.setModelMatrix(modelMatrix);
    DrawText(program, fontTexture, "YOU LOSE", 0.75, 0.00);
    
}

//      MAIN RENDER PROGRAM
void Render(ShaderProgram program){
    if (state == STATE_GAME){
        RenderGameState(program);
    } else if (state == STATE_START_SCREEN){
        RenderStartScreen(program);
    } else if (state == STATE_LOST){
        RenderLoseScreen(program);
    }
}

int main(int argc, char *argv[])
{
    ShaderProgram program = Setup();
    
    bool done = false;
    while (!done) {
        //      ELAPSED TIME
        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        
        ProcessInput(done);
        Update(elapsed);
        Render(program);
        SDL_GL_SwapWindow(displayWindow);
        glClear(GL_COLOR_BUFFER_BIT);
        
    }
    SDL_Quit();
    return 0;
}
