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


#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#define RESOURCE "/Users/shayaansaiyed/Documents/School/Game/CS3113Homework/HomeWork 1/NYUCodebase/"
#endif

SDL_Window* displayWindow;


GLuint LoadTexture(const char *filePath) {
    int w,h,comp;
    unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
    
    if(image == NULL) {
        std::cout << "Unable to load image. Make sure the path is correct\n";
        //assert(false);
    }
    
    GLuint retTexture;
    glGenTextures(1, &retTexture);
    glBindTexture(GL_TEXTURE_2D, retTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    stbi_image_free(image);
    return retTexture;
}

int main(int argc, char *argv[])
{
    
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
    glewInit();
#endif
    
    ShaderProgram program(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    
    //Load all textures
    GLuint TaylorTexture = LoadTexture(RESOURCE"TS.jpg");
    GLuint KatyTexture = LoadTexture(RESOURCE"katyperry.jpeg");
    GLuint KimTexture = LoadTexture(RESOURCE"Kim.jpg");
    
    glViewport(0, 0, 640, 360); //Set the size and offset of the rendering area
    
    Matrix projectionMatrix;
    Matrix modelMatrix;
    Matrix viewMatrix;
    
    projectionMatrix.setOrthoProjection(-3.70, 3.70, -2.0f, 2.0f, -1.0f, 1.0f);
    
    glUseProgram(program.programID);
    
    float lastFrameTicks = 0.0f;
    float angle = 0.0f;
    
    SDL_Event event;
    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }
        
//        //Get the time elapsed since the last iteration fo the loop
//        float ticks = (float)SDL_GetTicks()/1000.0f;
//        float elapsed = ticks - lastFrameTicks;
//        lastFrameTicks = ticks;
//        angle += elapsed;
        
        program.setProjectionMatrix(projectionMatrix);
        program.setViewMatrix(viewMatrix);
        
        
        //___________KIM KARDASHIAN____________
        
//        modelMatrix.identity();
//        
//        modelMatrix.Translate(-2.0f, 0.0f, 0.0f);
//        modelMatrix.Rotate(-45.0 * angle * 3.1417 / 180.0);
//        
//        program.setModelMatrix(modelMatrix);
//        
//        glBindTexture(GL_TEXTURE_2D, KimTexture);
//        
//        float verticesKim[] = {-3.0, -3.0, 3.0, -3.0, 3.0, 3.0, -3.0, -3.0, 3.0, 3.0, -3.0, 3.0};
//        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, verticesKim);
//        glEnableVertexAttribArray(program.positionAttribute);
//        float texCoordsKim[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
//        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordsKim);
//        glEnableVertexAttribArray(program.texCoordAttribute);
//        
//        glDrawArrays(GL_TRIANGLES, 0, 6);
//        
//        //___________TAYLOR SWIFT____________
//        
//        modelMatrix.identity();
//        
//        modelMatrix.Translate(1.0f, 0.0f, 0.0f);
//        modelMatrix.Rotate(45.0 * angle * 3.1417 / 180.0);
//        
//        program.setModelMatrix(modelMatrix);
//        
//        glBindTexture(GL_TEXTURE_2D, TaylorTexture);
//        
//        float verticesTaylor[] = {-1.5, -1.5, 1.5, -1.5, 1.5, 1.5, -1.5, -1.5, 1.5, 1.5, -1.5, 1.5};
//        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, verticesTaylor);
//        glEnableVertexAttribArray(program.positionAttribute);
//        float texCoordsTaylor[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
//        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordsTaylor);
//        glEnableVertexAttribArray(program.texCoordAttribute);
//        
//        glDrawArrays(GL_TRIANGLES, 0, 6);
//        
//        //___________Katy Perry____________
//        
//        modelMatrix.identity();
//        
//        modelMatrix.Translate(3.0f, 0.0f, 0.0f);
//        modelMatrix.Rotate(-45.0 * angle * 3.1417 / 180.0);
//        
//        program.setModelMatrix(modelMatrix);
//        
//        glBindTexture(GL_TEXTURE_2D, KatyTexture);
//        
//        float verticesKaty[] = {-0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5};
//        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, verticesKaty);
//        glEnableVertexAttribArray(program.positionAttribute);
//        float texCoordsKaty[] = {0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0};
//        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordsKaty);
//        glEnableVertexAttribArray(program.texCoordAttribute);
//        
//        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        SDL_GL_SwapWindow(displayWindow);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    
    SDL_Quit();
    return 0;
}
