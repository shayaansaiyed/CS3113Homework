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

#include <fstream>
#include <string>
#include <iostream>
#include <sstream>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#define RESOURCE "/Users/shayaansaiyed/Documents/School/Game/CS3113Homework/Platformer/NYUCodebase/"
#endif


SDL_Window* displayWindow;

using namespace std;

enum GameState { STATE_START_SCREEN, STATE_GAME, STATE_LOST };
enum EntityType {PLAYER, ENEMY, BULLET};

//-------------MAP-------------------
int mapWidth, mapHeight;

int SPRITE_COUNT_Y = 8;
int SPRITE_COUNT_X = 16;
float TILE_SIZE = 0.125;
GLuint LoadTexture(const char *filePath);
int tileSheetTexture;
unsigned char **levelData;


bool readHeader(std::ifstream &stream) {
    string line;
    mapWidth = -1;
    mapHeight = -1;
    while(getline(stream, line)) {
        if(line == "") { break; }
        istringstream sStream(line);
        string key,value;
        getline(sStream, key, '=');
        getline(sStream, value);
        if(key == "width") {
            mapWidth = atoi(value.c_str());
        } else if(key == "height"){
            mapHeight = atoi(value.c_str());
        } }
    if(mapWidth == -1 || mapHeight == -1) {
        return false;
    } else { // allocate our map data
        levelData = new unsigned char*[mapHeight];
        for(int i = 0; i < mapHeight; ++i) {
            levelData[i] = new unsigned char[mapWidth];
        }
        return true;
    }
}

bool readLayerData(std::ifstream &stream) {
    string line;
    while(getline(stream, line)) {
        if(line == "") { break; }
        istringstream sStream(line);
        string key,value;
        getline(sStream, key, '=');
        getline(sStream, value);
        if(key == "data") {
            for(int y=0; y < mapHeight; y++) {
                getline(stream, line);
                istringstream lineStream(line);
                string tile;
                for(int x=0; x < mapWidth; x++) {
                    getline(lineStream, tile, ',');
                    unsigned char val =  (unsigned char)atoi(tile.c_str());
                    if(val > 0) {
                        // be careful, the tiles in this format are indexed from 1 not 0
                        levelData[y][x] = val-1;
                    } else {
                        levelData[y][x] = 0;
                    }
                }
            } }
    }
    return true;
}

void placeEntity(string type, float placeX, float placeY);

bool readEntityData(std::ifstream &stream) {
    cout << "got here";
    string line;
    string type;
    while(getline(stream, line)) {
        if(line == "") { break; }
        istringstream sStream(line);
        string key,value;
        getline(sStream, key, '=');
        getline(sStream, value);
        if(key == "type") {
            type = value;
        } else if(key == "location") {
            istringstream lineStream(value);
            string xPosition, yPosition;
            getline(lineStream, xPosition, ',');
            getline(lineStream, yPosition, ',');
            float placeX = atoi(xPosition.c_str())*TILE_SIZE;
            float placeY = atoi(yPosition.c_str())*-TILE_SIZE;
            placeEntity(type, placeX, placeY);
                  
        }
    }
    return true;
}

void createMap(){
    ifstream infile(RESOURCE_FOLDER"map.txt");
    string line;
    while (getline(infile, line)) {
        if(line == "[header]") {
            if(!readHeader(infile)) {
                return; }
        } else if(line == "[layer]") {
            readLayerData(infile);
        } else if(line == "[Object Layer 1]") {
            readEntityData(infile);
        }
    }
}



void renderMap(ShaderProgram program){
    std::vector<float> vertexData;
    std::vector<float> texCoordData;
    for(int y=0; y < mapHeight; y++) {
        for(int x=0; x < mapWidth; x++) {

                float u = (float)(((int)levelData[y][x]) % SPRITE_COUNT_X) / (float) SPRITE_COUNT_X;
                float v = (float)(((int)levelData[y][x]) / SPRITE_COUNT_X) / (float) SPRITE_COUNT_Y;
                float spriteWidth = 1.0f/(float)SPRITE_COUNT_X;
                float spriteHeight = 1.0f/(float)SPRITE_COUNT_Y;
                vertexData.insert(vertexData.end(), {
                    TILE_SIZE * x, -TILE_SIZE * y,
                    TILE_SIZE * x, (-TILE_SIZE * y)-TILE_SIZE,
                    (TILE_SIZE * x)+TILE_SIZE, (-TILE_SIZE * y)-TILE_SIZE,
                    TILE_SIZE * x, -TILE_SIZE * y,
                    (TILE_SIZE * x)+TILE_SIZE, (-TILE_SIZE * y)-TILE_SIZE,
                    (TILE_SIZE * x)+TILE_SIZE, -TILE_SIZE * y
                });
                texCoordData.insert(texCoordData.end(), {
                    u, v,
                    u, v+(spriteHeight),
                    u+spriteWidth, v+(spriteHeight),
                    u, v,
                    u+spriteWidth, v+(spriteHeight),
                    u+spriteWidth, v
                });
            
        }
    }
    glBindTexture(GL_TEXTURE_2D, tileSheetTexture);
    
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
    glEnableVertexAttribArray(program.positionAttribute);
    
    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
    glEnableVertexAttribArray(program.texCoordAttribute);
    
    glDrawArrays(GL_TRIANGLES, 0, mapWidth*mapHeight*6);
}



//-------------VARIABLES-------------

//      STATE MANAGER
int state;

//      SETUP MATRIX
Matrix projectionMatrix;
Matrix modelMatrix;
Matrix orthographicMatrix;

//      PHYSICS
float friction = 2.0;
float gravity = -5.0;

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
    float topRight;
    float topLeft;
    float bottomRight;
    float bottomLeft;
    void getTopBottomLeftRight(){
        height = sprite.size;
        width = sprite.size * sprite.aspect;
        top = position.y + height/2;
        bottom = position.y - height/2;
        left = position.x - width/2;
        right = position.x + width/2;
    }
    
    bool collidesWithMap ();
    
    bool collidedTop;
    bool collidedBottom;
    bool collidedLeft;
    bool collidedRight;
};

void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY);

bool Entity::collidesWithMap() {
    int xbottom;
    int ybottom;
    int xtop;
    int ytop;
    int xright;
    int yright;
    int xleft;
    int yleft;
    getTopBottomLeftRight();
    
    //Bottom Collision
    worldToTileCoordinates(position.x, position.y - height/2, &xbottom, &ybottom);
    if(levelData[ybottom][xbottom] > 0){
        collidedBottom = true;
        position.y = (float)(2.0 - ybottom * TILE_SIZE) + height/2;
    }
    
    //Top Collision
    worldToTileCoordinates(position.x, position.y + height/2, &xtop, &ytop);
    if(levelData[ytop][xtop] > 0){
        collidedTop = true;
        position.y = ((float)(2.0 - ytop * TILE_SIZE) - height/2) - TILE_SIZE;
    }
    
    //Left Collision
    worldToTileCoordinates(position.x - width/2, position.y, &xleft, &yleft);
    if(levelData[yleft][xleft] > 0){
        collidedLeft = true;
        position.x = ((float)(xleft * TILE_SIZE - 3.70) + width/2) + TILE_SIZE;
    }
    
    //Right Collision
    worldToTileCoordinates(position.x + width/2, position.y, &xright, &yright);
    if(levelData[yright][xright] > 0){
        collidedRight = true;
        position.x = ((float)(xright * TILE_SIZE - 3.70) - width/2);
    }
    return false;
}

//-------------FUNCTION DEFINITIONS-----------------

#define MAX_COINS 100
Entity Grammys[MAX_COINS];
int grammyIndex = 0;

void placeEntity(string type, float placeX, float placeY){
    cout << "Place: " << placeX << "," << placeY << endl;
    if (grammyIndex >= MAX_COINS){
        grammyIndex %= MAX_COINS;
    }
    cout << "Grammy Index" << grammyIndex << endl;
    Grammys[grammyIndex].position.x = placeX;
    Grammys[grammyIndex].position.y = placeY;
    Grammys[grammyIndex].sprite = SheetSprite(tileSheetTexture, 12.0/16.0, 0, 16.0/256.0, 16.0/128.0, 0.5);
    grammyIndex++;
}

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


void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY) {
    *gridX = (int)((worldX + 3.7)  / (TILE_SIZE));
    *gridY = (int)((2 - worldY) / (TILE_SIZE));
}

//-------------SETUP----------------

Entity player;

void setupPlayer(){
    spriteSheetTexture = LoadTexture(RESOURCE_FOLDER"sprites.png");
    player.position.x = 0.0;
    player.position.y = 2.0;
    
    player.sprite = SheetSprite(spriteSheetTexture, 0.0, 0.0, 35.0/64.0, 86.0/128.0, 0.5);
}

//      MAIN SETUP FUNCTION
ShaderProgram Setup(){
    cout << "SETUP\n";
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
    
    tileSheetTexture = LoadTexture(RESOURCE_FOLDER"arne_sprites.png");
    
    setupPlayer();
    createMap();
    
    return program;
}




//-------------PROCESS INPUT---------------------

float xpos = 0.0;
float ypos = 0.0;
int xcounter = 0;
int ycounter = 0;
float scaleAmount = 0.0;

bool playerJump = false;

void ProcessInput(bool& done){
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            done = true;
        }
        else if(event.type == SDL_KEYDOWN) {
            
        }
    }
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_UP]){
        if (true){
            player.velocity.y = 2.0;
        }
    }
    if (keys[SDL_SCANCODE_LEFT]){
        player.velocity.x = -1.0;
    }
    if (keys[SDL_SCANCODE_RIGHT]){
        player.velocity.x = 1.0;
    }
    if (keys[SDL_SCANCODE_W]){
        orthographicMatrix.Scale(1.1,1.1,0.0);
    }
    if (keys[SDL_SCANCODE_S]){
        orthographicMatrix.Scale(0.9,0.9,0.0);
    }
}


//-------------UPDATE---------------

void collidesWithGrammy(){
    player.getTopBottomLeftRight();
    cout << "CHECK COLLIDES" << endl;
    for (int i = 0; i < MAX_COINS; i++){
        Grammys[i].getTopBottomLeftRight();
        if (!((player.bottom >= Grammys[i].top + 2.0)
        || (player.top <= Grammys[i].bottom + 2.0)
            || (player.left >= Grammys[i].right - 3.70)
            || (player.right <= Grammys[i].left - 3.70))) {
            cout << "COLLIDES" << endl;
            Grammys[i].position.y = 2000;
        }
    }
}

void UpdatePlayer(float elapsed){
    
    player.collidedBottom = false;
    player.collidedTop = false;
    player.collidedLeft = false;
    player.collidedRight = false;
    
    player.velocity.x += player.acceleration.x * elapsed;
    player.velocity.y += player.acceleration.y * elapsed;
    
    player.velocity.y += gravity * elapsed;
    player.velocity.x = lerp(player.velocity.x, 0.0f, friction*elapsed);
    
    player.position.x += player.velocity.x * elapsed;
    player.position.y += player.velocity.y * elapsed;
    player.collidesWithMap();
    collidesWithGrammy();
    if(player.collidedBottom){
        player.velocity.y = 0;
    }
    if(player.collidedTop){
        player.velocity.y *= -1;
    }
}

void Update(float elapsed){
    UpdatePlayer(elapsed);
}




//-------------RENDER---------------


//      MAIN RENDER PROGRAM
void Render(ShaderProgram program){
    cout << "RENDER\n";
    modelMatrix.identity();
    modelMatrix.Translate(-3.7, 2.0, 0.0);
    program.setModelMatrix(modelMatrix);
    renderMap(program);
    
    modelMatrix.identity();
    modelMatrix.Translate(player.position.x, player.position.y, player.position.z);
    program.setModelMatrix(modelMatrix);
    player.sprite.Draw(program);
    
    for (int i = 0; i < MAX_COINS; i++){
        modelMatrix.identity();
        modelMatrix.Translate(-3.7, 2.0+TILE_SIZE, 0.0);
        modelMatrix.Translate(Grammys[i].position.x, Grammys[i].position.y, 0.0);
        cout << "grammy position: " << Grammys[i].position.x << " " << Grammys[i].position.y << endl;
        cout << "Player position: " << player.position.x << " " << player.position.y << endl;
        program.setModelMatrix(modelMatrix);
        Grammys[i].sprite.Draw(program);
    }
    
    orthographicMatrix.identity();
    orthographicMatrix.Translate(-player.position.x, 0.0, 0.0);
    program.setViewMatrix(orthographicMatrix);
    

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
        
        cout << "Player Position: "  <<player.position.x << " "  << player.position.y << endl;
        
        ProcessInput(done);
        Update(elapsed);
        Render(program);
        SDL_GL_SwapWindow(displayWindow);
        glClear(GL_COLOR_BUFFER_BIT);
        
    }
    cout << "x amount: " << xcounter << "\ny amount: " << ycounter << "\nscale amount: " << scaleAmount;
    SDL_Quit();
    return 0;
}
