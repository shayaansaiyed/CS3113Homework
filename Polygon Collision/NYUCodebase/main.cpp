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
#include "Vector.hpp"

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

#include <cmath>

using namespace std;

SDL_Window* displayWindow;


Matrix projectionMatrix;
Matrix modelMatrix;
Matrix orthographicMatrix;

float lastFrameTicks = 0.0f;

Vector poly1_pos;
Vector poly2_pos;

Vector poly1_velocity;
Vector poly2_velocity;

Vector collision;


bool testSATSeparationForEdge(float edgeX, float edgeY, const std::vector<Vector> &points1, const std::vector<Vector> &points2, Vector &penetration) {
    float normalX = -edgeY;
    float normalY = edgeX;
    float len = sqrtf(normalX*normalX + normalY*normalY);
    normalX /= len;
    normalY /= len;
    
    std::vector<float> e1Projected;
    std::vector<float> e2Projected;
    
    for(int i=0; i < points1.size(); i++) {
        e1Projected.push_back(points1[i].x * normalX + points1[i].y * normalY);
    }
    for(int i=0; i < points2.size(); i++) {
        e2Projected.push_back(points2[i].x * normalX + points2[i].y * normalY);
    }
    
    std::sort(e1Projected.begin(), e1Projected.end());
    std::sort(e2Projected.begin(), e2Projected.end());
    
    float e1Min = e1Projected[0];
    float e1Max = e1Projected[e1Projected.size()-1];
    float e2Min = e2Projected[0];
    float e2Max = e2Projected[e2Projected.size()-1];
    
    float e1Width = fabs(e1Max-e1Min);
    float e2Width = fabs(e2Max-e2Min);
    float e1Center = e1Min + (e1Width/2.0);
    float e2Center = e2Min + (e2Width/2.0);
    float dist = fabs(e1Center-e2Center);
    float p = dist - ((e1Width+e2Width)/2.0);
    
    if(p >= 0) {
        return false;
    }
    
    float penetrationMin1 = e1Max - e2Min;
    float penetrationMin2 = e2Max - e1Min;
    
    float penetrationAmount = penetrationMin1;
    if(penetrationMin2 < penetrationAmount) {
        penetrationAmount = penetrationMin2;
    }
    
    penetration.x = normalX * penetrationAmount;
    penetration.y = normalY * penetrationAmount;
    
    return true;
}

bool penetrationSort(const Vector &p1, const Vector &p2) {
    return p1.length() < p2.length();
}

bool checkSATCollision(const std::vector<Vector> &e1Points, const std::vector<Vector> &e2Points, Vector &penetration) {
    std::vector<Vector> penetrations;
    for(int i=0; i < e1Points.size(); i++) {
        float edgeX, edgeY;
        
        if(i == e1Points.size()-1) {
            edgeX = e1Points[0].x - e1Points[i].x;
            edgeY = e1Points[0].y - e1Points[i].y;
        } else {
            edgeX = e1Points[i+1].x - e1Points[i].x;
            edgeY = e1Points[i+1].y - e1Points[i].y;
        }
        Vector penetration;
        bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points, penetration);
        if(!result) {
            return false;
        }
        penetrations.push_back(penetration);
    }
    for(int i=0; i < e2Points.size(); i++) {
        float edgeX, edgeY;
        
        if(i == e2Points.size()-1) {
            edgeX = e2Points[0].x - e2Points[i].x;
            edgeY = e2Points[0].y - e2Points[i].y;
        } else {
            edgeX = e2Points[i+1].x - e2Points[i].x;
            edgeY = e2Points[i+1].y - e2Points[i].y;
        }
        Vector penetration;
        bool result = testSATSeparationForEdge(edgeX, edgeY, e1Points, e2Points, penetration);
        
        if(!result) {
            return false;
        }
        penetrations.push_back(penetration);
    }
    
    std::sort(penetrations.begin(), penetrations.end(), penetrationSort);
    penetration = penetrations[0];
    
    Vector e1Center;
    for(int i=0; i < e1Points.size(); i++) {
        e1Center.x += e1Points[i].x;
        e1Center.y += e1Points[i].y;
    }
    e1Center.x /= (float)e1Points.size();
    e1Center.y /= (float)e1Points.size();
    
    Vector e2Center;
    for(int i=0; i < e2Points.size(); i++) {
        e2Center.x += e2Points[i].x;
        e2Center.y += e2Points[i].y;
    }
    e2Center.x /= (float)e2Points.size();
    e2Center.y /= (float)e2Points.size();
    
    Vector ba;
    ba.x = e1Center.x - e2Center.x;
    ba.y = e1Center.y - e2Center.y;
    
    if( (penetration.x * ba.x) + (penetration.y * ba.y) < 0.0f) {
        penetration.x *= -1.0f;
        penetration.y *= -1.0f;
    }
    
    return true;
}

/*
 float vertices1[] = {0.75, 1.0, -0.5, 0.75, -0.75, 0.0,
 0.75, 1.0, -0.75, 0.0, 0.0, -1.0,
 0.75, 1.0, 0.0, -1.0, 1.25, 0.0}
;
float vertices2[] = {0.5, 1.0, -0.5, 1, -1, 0.5,
    1.0, 0.5, 0.5, 1.0, -1.0, 0.5,
    0.0, -1.0, 1.0, 0.5, -1.0, 0.5
};
*/

vector<Vector> edges1 (5);
vector<Vector> edges2 (5);

ShaderProgram Setup(){
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 360, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
#ifdef _WINDOWS
    glewInit();
#endif
    
    ShaderProgram program(RESOURCE_FOLDER"vertex.glsl", RESOURCE_FOLDER"fragment.glsl");
    glViewport(0, 0, 640, 360);
    
    glUseProgram(program.programID);
    
    projectionMatrix.setOrthoProjection(-3.70, 3.70, -2.0f, 2.0f, -1.0f, 1.0f);
    
    program.setProjectionMatrix(projectionMatrix);
    program.setViewMatrix(orthographicMatrix);
    
    poly1_pos.x = -2.75;
    poly2_pos.x = 2.75;
    poly1_velocity.x = 1.0;
    poly2_velocity.x = -1.0;
    
    edges1[0].x = 3;
    
    return program;
}

void Update(float elapsed){
    poly1_pos.x += poly1_velocity.x * elapsed;
    poly1_pos.y += poly1_velocity.y * elapsed;
    
    poly2_pos.x += poly2_velocity.x * elapsed;
    poly2_pos.y += poly2_velocity.y * elapsed;
}


void drawPolygon(ShaderProgram program, float vertices[]){
    program.setModelMatrix(modelMatrix);
    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program.positionAttribute);
    glDrawArrays(GL_LINE_LOOP, 0, 5);
    glDisableVertexAttribArray(program.positionAttribute);
}

void Render(ShaderProgram program){
    float vertices1[] = {
        -0.50, 0.75,
        -0.75, 0.0,
        0.00, -1.0,
        1.25, 0.00,
        0.75, 1.00
    }
    ;
    float vertices2[] = {
        0.5, 1.0,
        -0.5, 1.0,
        -1.0, 0.5,
        0.0, -1.0,
        1.0, 0.5
    };
    
    for(int i = 0; i < 5; i++){
        edges1[i].x = vertices1[i * 2] + poly1_pos.x;
        edges1[i].y = vertices1[i * 2 + 1] + poly1_pos.y;
        
        edges2[i].x = vertices2[i * 2] + poly2_pos.x;
        edges2[i].y = vertices2[i * 2 + 1] + poly2_pos.y;
        
    }
    
    bool collided = false;
    float xpenetration, ypenetration;
    
    cout << "x:" << collision.x << " " << "y:" << collision.y;
    
    collided = checkSATCollision(edges1, edges2, collision);
    if (collided){
        
        xpenetration = collision.x;
        ypenetration = collision.y;
        
        // NORMALIZE
        
        collision.x /= fabs(collision.x);
        collision.y /= fabs(collision.y);
        
        poly1_velocity.x *= -collision.x;
        poly1_velocity.y *= -collision.y;
        
        poly1_pos.x += -0.5 * xpenetration;
        poly1_pos.y += -0.5 * ypenetration;
        
        poly2_velocity.x *= -collision.x;
        poly2_velocity.y *= -collision.y;
        
        poly2_pos.x += -0.5 * xpenetration;
        poly2_pos.y += -0.5 * ypenetration;
    }
    
    modelMatrix.identity();
    modelMatrix.Translate(poly1_pos.x, poly1_pos.y, poly1_pos.z);
    
    drawPolygon(program, vertices1);
    
    modelMatrix.identity();
    modelMatrix.Translate(poly2_pos.x, poly2_pos.y, poly2_pos.z);
    drawPolygon(program, vertices2);
    
}

int main(int argc, char *argv[])
{
    ShaderProgram program = Setup();

    
    SDL_Event event;
    bool done = false;
    while (!done) {
        //      ELAPSED TIME
        float ticks = (float)SDL_GetTicks()/1000.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }
        glClear(GL_COLOR_BUFFER_BIT);
        Update(elapsed);
        program.setProjectionMatrix(projectionMatrix);
        program.setViewMatrix(orthographicMatrix);
        Render(program);
        
        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}
