#include <GL/glew.h>  // This must be before other GL libs.
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "collision.h"
#include "block.h"
#include "cog/camera/camera.h"
#include "cog/dbg.h"
#include "cog/shaders/shaders.h"
#include "util.h"

// --- //

GLfloat* blockToCoords();
void initBoard();
void clearBoard();
void newBlock();
void refreshBlock();
int refreshBoard();

// --- //

#define BOARD_CELLS 200
// 6 floats per vertex, 3 vertices per triangle, 12 triangles per Cell
#define CELL_FLOATS 6 * 3 * 12
#define TOTAL_FLOATS BOARD_CELLS * CELL_FLOATS

bool gameOver = false;
bool running  = true;
bool keys[1024];
GLuint wWidth  = 400;
GLuint wHeight = 720;

// Buffer Objects
GLuint gVAO;
GLuint gVBO;
GLuint bVAO;
GLuint bVBO;
GLuint fVAO;
GLuint fVBO;

// Timing Info
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;
GLfloat keyDelta  = 0.0f;
GLfloat lastKey   = 0.0f;

camera_t* camera;
matrix_t* view;
block_t*  block;   // The Tetris block.
Fruit board[BOARD_CELLS];  // The Board, represented as Fruits.

// --- //

/* Move Camera with WASD */
/* DEPRECIATED
void moveCamera() {
        cogcMove(camera,
                 deltaTime,
                 keys[GLFW_KEY_W],
                 keys[GLFW_KEY_S],
                 keys[GLFW_KEY_A],
                 keys[GLFW_KEY_D]);
}
*/

/* Init/Reset the Camera */
void resetCamera() {
        matrix_t* camPos = coglV3(0,0,4);
        matrix_t* camDir = coglV3(0,0,-1);
        matrix_t* camUp = coglV3(0,1,0);

        camera = cogcCreate(camPos,camDir,camUp);
}

/* Clears the board and starts over */
void resetGame() {
        initBoard();
        newBlock();
}

void newBlock() {
        block = randBlock();
        refreshBlock();
}

void refreshBlock() {
        GLfloat* coords = blockToCoords();
        
        glBindVertexArray(bVAO);
        glBindBuffer(GL_ARRAY_BUFFER, bVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 
                        CELL_FLOATS * 4 * sizeof(GLfloat), coords);
        glBindVertexArray(0);

        free(coords);  // Necessary?
}

void key_callback(GLFWwindow* w, int key, int code, int action, int mode) {
        GLfloat currentTime = glfwGetTime();

        // Update key timing.
        keyDelta = currentTime - lastKey;
        lastKey  = currentTime;

        if(action == GLFW_PRESS ||
           (action == GLFW_REPEAT && keyDelta > 0.01)) {
                keys[key] = true;

                if(keys[GLFW_KEY_Q]) {
                        glfwSetWindowShouldClose(w, GL_TRUE);
                } else if(key == GLFW_KEY_P) {
                        if(running) {
                                running = false;
                        } else {
                                running = true;
                        }
                } else if(key == GLFW_KEY_C) {
                        resetCamera();
                } else if(key == GLFW_KEY_R) {
                        resetGame();
                } else if(key == GLFW_KEY_LEFT && block->x > 0) {
                        if(isColliding(block,board) != Left) {
                                block->x -= 1;
                                refreshBlock();
                        }
                } else if(key == GLFW_KEY_RIGHT && block->x < 9) {
                        if(isColliding(block,board) != Right) {
                                block->x += 1;
                                refreshBlock();
                        }
                } else if(key == GLFW_KEY_DOWN && block->y > 0) {
                        block->y -= 1;
                        refreshBlock();
                } else if(key == GLFW_KEY_UP && block->y < 19) {
                        block_t* copy = copyBlock(block);
                        copy = rotateBlock(copy);
                        if(isColliding(copy,board) == Clear) {
                                block = rotateBlock(block);
                                destroyBlock(copy);
                                refreshBlock();
                        } else {
                                debug("Flip would collide!");
                        }
                } else if(key == GLFW_KEY_SPACE) {
                        block = shuffleFruit(block);
                        refreshBlock();
                }
        } else if(action == GLFW_RELEASE) {
                keys[key] = false;
        }
}

void mouse_callback(GLFWwindow* w, double xpos, double ypos) {
        cogcPan(camera,xpos,ypos);
}

GLfloat* gridLocToCoords(int x, int y, Fruit f) {
        GLfloat* coords = NULL;
        GLfloat* c      = fruitColour(f);
        GLuint i;
        // TODO: This is wrong.
        GLfloat temp[CELL_FLOATS] = {
                // Back T1
                33 + x*33, 33 + y*33,  0, c[0], c[1], c[2],
                33 + x*33, 66 + y*33,  0, c[0], c[1], c[2],
                66 + x*33, 33 + y*33,  0, c[0], c[1], c[2],
                // Back T2
                33 + x*33, 66 + y*33,  0, c[0], c[1], c[2], 
                66 + x*33, 33 + y*33,  0, c[0], c[1], c[2],
                66 + x*33, 66 + y*33,  0, c[0], c[1], c[2],
                // Front T1
                33 + x*33, 33 + y*33, 33, c[0], c[1], c[2],
                33 + x*33, 66 + y*33, 33, c[0], c[1], c[2],
                66 + x*33, 33 + y*33, 33, c[0], c[1], c[2],
                // Front T2
                33 + x*33, 66 + y*33, 33, c[0], c[1], c[2], 
                66 + x*33, 33 + y*33, 33, c[0], c[1], c[2],
                66 + x*33, 66 + y*33, 33, c[0], c[1], c[2],
                // Left T1
                33 + x*33, 33 + y*33,  0, c[0], c[1], c[2],
                33 + x*33, 66 + y*33,  0, c[0], c[1], c[2],
                33 + x*33, 66 + y*33, 33, c[0], c[1], c[2],
                // Left T2
                33 + x*33, 33 + y*33,  0, c[0], c[1], c[2],
                33 + x*33, 66 + y*33, 33, c[0], c[1], c[2],
                33 + x*33, 33 + y*33, 33, c[0], c[1], c[2],
                // Right T1
                66 + x*33, 66 + y*33,  0, c[0], c[1], c[2],
                66 + x*33, 33 + y*33,  0, c[0], c[1], c[2],
                66 + x*33, 66 + y*33, 33, c[0], c[1], c[2],
                // Right T2
                66 + x*33, 66 + y*33, 33, c[0], c[1], c[2],
                66 + x*33, 33 + y*33, 33, c[0], c[1], c[2],
                66 + x*33, 33 + y*33,  0, c[0], c[1], c[2],
                // Top T1
                33 + x*33, 66 + y*33, 33, c[0], c[1], c[2],
                66 + x*33, 66 + y*33, 33, c[0], c[1], c[2],
                66 + x*33, 66 + y*33,  0, c[0], c[1], c[2],
                // Top T2
                33 + x*33, 66 + y*33, 33, c[0], c[1], c[2],
                33 + x*33, 66 + y*33,  0, c[0], c[1], c[2],
                66 + x*33, 66 + y*33,  0, c[0], c[1], c[2],
                // Bottom T1
                33 + x*33, 33 + y*33, 33, c[0], c[1], c[2],
                66 + x*33, 33 + y*33, 33, c[0], c[1], c[2],
                66 + x*33, 33 + y*33,  0, c[0], c[1], c[2],
                // Bottom T2
                33 + x*33, 33 + y*33, 33, c[0], c[1], c[2],
                33 + x*33, 33 + y*33,  0, c[0], c[1], c[2],
                66 + x*33, 33 + y*33,  0, c[0], c[1], c[2]
        };

        check(x > -1 && x < 10 &&
              y > -1 && x < 20,
              "Invalid coords given.");

        coords = malloc(sizeof(GLfloat) * CELL_FLOATS);
        check_mem(coords);

        if(f == None) {
                // Nullify all the coordinates
                for(i = 0; i < CELL_FLOATS; i++) {
                        temp[i] = 0.0;
                }
        }

        // Copy values.
        for(i = 0; i < CELL_FLOATS; i++) {
                coords[i] = temp[i];
        }
        
        return coords;
 error:
        return NULL;
}

/* Produce locations and colours based on the current global Block */
GLfloat* blockToCoords() {
        GLfloat* temp1;
        GLfloat* temp2;
        GLfloat* cs = NULL;

        // 4 cells, each has 36 vertices of 6 data points each.
        //GLfloat* cs = malloc(sizeof(GLfloat) * 4 * 36 * 6);
        //check_mem(cs);

        // Coords and colours for each cell.
        GLfloat* a = gridLocToCoords(block->x+block->coords[0],
                                     block->y+block->coords[1],
                                     block->fs[0]);
        GLfloat* b = gridLocToCoords(block->x+block->coords[2],
                                     block->y+block->coords[3],
                                     block->fs[1]);
        GLfloat* c = gridLocToCoords(block->x,
                                     block->y,
                                     block->fs[2]);
        GLfloat* d = gridLocToCoords(block->x+block->coords[4],
                                     block->y+block->coords[5],
                                     block->fs[3]);

        check(a && b && c && d, "Couldn't get Cell coordinates.");

         // Construct return value
        temp1 = append(a, CELL_FLOATS, b, CELL_FLOATS);
        temp2 = append(c, CELL_FLOATS, d, CELL_FLOATS);
        cs    = append(temp1, CELL_FLOATS * 2, temp2, CELL_FLOATS * 2);
        check(cs, "Couldn't construct final list of coords/colours.");

        free(temp1); free(temp2);
        free(a); free(b); free(c); free(d);
        
        return cs;
 error:
        if(cs) { free(cs); }
        return NULL;
}

/* Initialize the Block */
int initBlock() {
        block = randBlock();
        check(block, "Failed to initialize first Block.");
        debug("Got a: %c", block->name);

        debug("Initializing Block.");

        GLfloat* coords = blockToCoords();
        
        // Set up VAO/VBO
        glGenVertexArrays(1,&bVAO);
        glBindVertexArray(bVAO);
        glGenBuffers(1,&bVBO);
        glBindBuffer(GL_ARRAY_BUFFER,bVBO);
        glBufferData(GL_ARRAY_BUFFER,
                     CELL_FLOATS * 4 * sizeof(GLfloat),coords,
                     GL_DYNAMIC_DRAW);

        // Tell OpenGL how to process Block Vertices
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,
                              6 * sizeof(GLfloat),(GLvoid*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,
                              6 * sizeof(GLfloat),
                              (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);  // Reset the VAO binding.
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        debug("Block initialized.");

        return 1;
 error:
        return 0;
}

/* Initialize the Grid */
// Insert TRON pun here.
void initGrid() {
        GLfloat gridPoints[768];  // Contains colour info as well.
        int i;

        debug("Initializing Grid.");

        // Back Vertical lines
	for (i = 0; i < 11; i++) {
                // Bottom coord
		gridPoints[12*i]     = 33.0 + (33.0 * i);
                gridPoints[12*i + 1] = 33.0;
                gridPoints[12*i + 2] = 0;
                // Bottom colour
                gridPoints[12*i + 3] = 1;
                gridPoints[12*i + 4] = 1;
                gridPoints[12*i + 5] = 1;
                // Top coord
		gridPoints[12*i + 6] = 33.0 + (33.0 * i);
                gridPoints[12*i + 7] = 693.0;
                gridPoints[12*i + 8] = 0;
                // Top colour
                gridPoints[12*i + 9]  = 1;
                gridPoints[12*i + 10] = 1;
                gridPoints[12*i + 11] = 1;
	}

	// Back Horizontal lines
	for (i = 0; i < 21; i++) {
                // Left coord
		gridPoints[132 + 12*i]     = 33.0;
                gridPoints[132 + 12*i + 1] = 33.0 + (33.0 * i);
                gridPoints[132 + 12*i + 2] = 0;
                // Left colour
                gridPoints[132 + 12*i + 3] = 1;
                gridPoints[132 + 12*i + 4] = 1;
                gridPoints[132 + 12*i + 5] = 1;
                // Right coord
		gridPoints[132 + 12*i + 6] = 363.0;
                gridPoints[132 + 12*i + 7] = 33.0 + (33.0 * i);
                gridPoints[132 + 12*i + 8] = 0;
                // Right colour
                gridPoints[132 + 12*i + 9]  = 1;
                gridPoints[132 + 12*i + 10] = 1;
                gridPoints[132 + 12*i + 11] = 1;
	}

        // Front Vertical lines
	for (i = 0; i < 11; i++) {
                // Bottom coord
		gridPoints[384 + 12*i]     = 33.0 + (33.0 * i);
                gridPoints[384 + 12*i + 1] = 33.0;
                gridPoints[384 + 12*i + 2] = 33.0;
                // Bottom colour
                gridPoints[384 + 12*i + 3] = 1;
                gridPoints[384 + 12*i + 4] = 1;
                gridPoints[384 + 12*i + 5] = 1;
                // Top coord
		gridPoints[384 + 12*i + 6] = 33.0 + (33.0 * i);
                gridPoints[384 + 12*i + 7] = 693.0;
                gridPoints[384 + 12*i + 8] = 33.0;
                // Top colour
                gridPoints[384 + 12*i + 9]  = 1;
                gridPoints[384 + 12*i + 10] = 1;
                gridPoints[384 + 12*i + 11] = 1;
	}

	// Front Horizontal lines
	for (i = 0; i < 21; i++) {
                // Left coord
		gridPoints[516 + 12*i]     = 33.0;
                gridPoints[516 + 12*i + 1] = 33.0 + (33.0 * i);
                gridPoints[516 + 12*i + 2] = 33.0;
                // Left colour
                gridPoints[516 + 12*i + 3] = 1;
                gridPoints[516 + 12*i + 4] = 1;
                gridPoints[516 + 12*i + 5] = 1;
                // Right coord
		gridPoints[516 + 12*i + 6] = 363.0;
                gridPoints[516 + 12*i + 7] = 33.0 + (33.0 * i);
                gridPoints[516 + 12*i + 8] = 33.0;
                // Right colour
                gridPoints[516 + 12*i + 9]  = 1;
                gridPoints[516 + 12*i + 10] = 1;
                gridPoints[516 + 12*i + 11] = 1;
	}

        // Set up VAO/VBO
        glGenVertexArrays(1,&gVAO);
        glBindVertexArray(gVAO);
        glGenBuffers(1,&gVBO);
        glBindBuffer(GL_ARRAY_BUFFER, gVBO);
        glBufferData(GL_ARRAY_BUFFER,sizeof(gridPoints),
                     gridPoints,GL_STATIC_DRAW);

        // Tell OpenGL how to process Grid Vertices
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,
                              6 * sizeof(GLfloat),(GLvoid*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,
                              6 * sizeof(GLfloat),
                              (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);  // Reset the VAO binding.
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        debug("Grid initialized.");
}

/* Initialize the game board */
void initBoard() {
        debug("Initializing Board.");

        GLfloat temp[TOTAL_FLOATS];
        
        // Set up VAO/VBO
        glGenVertexArrays(1,&fVAO);
        glBindVertexArray(fVAO);
        glGenBuffers(1,&fVBO);
        glBindBuffer(GL_ARRAY_BUFFER,fVBO);
        // 200 cells, each has 36 vertices of 6 data points each.
        glBufferData(GL_ARRAY_BUFFER, 
                     TOTAL_FLOATS * sizeof(GLfloat),
                     temp,
                     GL_DYNAMIC_DRAW);
        
        // Tell OpenGL how to process Block Vertices
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,
                              6 * sizeof(GLfloat),(GLvoid*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,
                              6 * sizeof(GLfloat),
                              (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);  // Reset the VAO binding.
        //        glBindBuffer(GL_ARRAY_BUFFER, 0);
        
        clearBoard();

        debug("Board initialized.");
}

/* Move all coloured Cells from the Board */
void clearBoard() {
        int i;

        for(i = 0; i < BOARD_CELLS; i++) {
                board[i] = 0;
        }

        refreshBoard();
}

/* Draw all the coloured Board Cells */
int refreshBoard() {
        GLuint i,j,k;
        GLfloat* cellData;

        debug("Refreshing Board...");
        
        GLfloat* coords = malloc(sizeof(GLfloat) * TOTAL_FLOATS);
        check_mem(coords);

        for(i = 0; i < BOARD_CELLS; i++) {
                cellData = gridLocToCoords(i % 10, i / 10, board[i]);
                check(cellData, "Couldn't get coord data for Cell.");
                
                for(j = i*CELL_FLOATS, k=0; k < CELL_FLOATS; j++, k++) {
                        coords[j] = cellData[k];
                }
        }

        glBindVertexArray(fVAO);
        glBindBuffer(GL_ARRAY_BUFFER, fVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, 
                        TOTAL_FLOATS * sizeof(GLfloat), coords);
        glBindVertexArray(0);

        free(coords);
        
        return 1;
 error:
        return 0;        
}

/* Removes any solid lines, if it can */
void lineCheck() {
        int i,j;
        bool fullRow = true;

        for(i = 0; i < BOARD_CELLS; i+=10) {
                fullRow = true;

                // Check for full row
                for(j = 0; j < 10; j++) {
                        if(board[i + j] == None) {
                                fullRow = false;
                                break;
                        }
                }

                if(fullRow) {
                        debug("Found a full row!");
                        // Empty the row
                        for(j = 0; j < 10; j++) {
                                board[i + j] = None;
                        }

                        // Drop the other pieces.
                        // This is evil. C is stupid.
                        for(i = i + j; i < BOARD_CELLS; i++) {
                                board[i-10] = board[i];
                        }

                        break;
                }
        }
}

/* Removes sets of 3 matching Fruits, if it can */
void fruitCheck() {
        int i,j,k;
        Fruit curr;
        Fruit streakF = None;
        int streakN;

        // Check columns
        for(i = 0; i < 10; i++) {
                streakN = 1;

                for(j = 0; j < 20; j++) {
                        curr = board[i + j*10];

                        if(curr != None && curr == streakF) {
                                streakN++;

                                if(streakN == 3) {
                                        board[i + j*10] = None;
                                        board[i + (j-1)*10] = None;
                                        board[i + (j-2)*10] = None;

                                        for(j = j+1; j < 20; j++) {
                                                board[i+(j-3)*10] = board[i+j*10];
                                        }
                                        break;
                                }
                        } else {
                                streakF = curr;
                                streakN = 1;
                        }
                }
        }

        // Check rows
        for(j = 0; j < 20; j++) {
                streakN = 1;

                for(i = 0; i < 10; i++) {
                        curr = board[i + j*10];

                        if(curr != None && curr == streakF) {
                                streakN++;

                                if(streakN == 3) {
                                        board[i + j*10] = None;
                                        board[i-1 + j*10] = None;
                                        board[i-2 + j*10] = None;

                                        for(k = j+1; k < 20; k++) {
                                                board[i-2 + (k-1)*10] = board[i-2 + k*10];
                                        }
                                        for(k = j+1; k < 20; k++) {
                                                board[i-1 + (k-1)*10] = board[i-1 + k*10];
                                        }
                                        for(k = j+1; k < 20; k++) {
                                                board[i + (k-1)*10] = board[i + k*10];
                                        }
                                }
                        } else {
                                streakF = curr;
                                streakN = 1;
                        }
                }
        }
}

/* Scrolls the Block naturally down */
void scrollBlock() {
        static double lastTime = 0;
        double currTime = glfwGetTime();
        int* cells = NULL;
        int i,j;

        if(!running) {
                return;
        }
        
        if(isColliding(block, board) != Bottom) {
                if(currTime - lastTime > 0.5) {

                        lastTime = currTime;
                        block->y -= 1;

                        GLfloat* coords = blockToCoords();
        
                        glBindVertexArray(bVAO);
                        glBindBuffer(GL_ARRAY_BUFFER, bVBO);
                        glBufferSubData(GL_ARRAY_BUFFER, 0, 
                                        CELL_FLOATS * 4 * sizeof(GLfloat),
                                        coords);
                        glBindVertexArray(0);
                }
        } else if(block->y == 19) {
                gameOver = true;
        } else {
                cells = blockCells(block);

                // Add the Block's cells to the master Board
                for(i = 0,j=0; i < 8; i+=2,j++) {
                        board[cells[i] + 10*cells[i+1]] = block->fs[j];
                }

                lineCheck();
                fruitCheck();
                newBlock();
                refreshBoard();
        }
}

int main(int argc, char** argv) {
        // Initial settings.
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
        
        // Make a window.
        GLFWwindow* w = glfwCreateWindow(wWidth,wHeight,"Fetris",NULL,NULL);
        glfwMakeContextCurrent(w);

        // Fire up GLEW.
        glewExperimental = GL_TRUE;  // For better compatibility.
        glewInit();

        // For the rendering window.
        glViewport(0,0,wWidth,wHeight);

        // Register callbacks.
        glfwSetKeyCallback(w, key_callback);
        glfwSetInputMode(w,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
        glfwSetCursorPosCallback(w,mouse_callback);
        
        // Depth Testing
        glEnable(GL_DEPTH_TEST);

        // Create Shader Program
        debug("Making shader program.");
        shaders_t* shaders = cogsShaders("vertex.glsl", "fragment.glsl");
        GLuint shaderProgram = cogsProgram(shaders);
        cogsDestroy(shaders);
        check(shaderProgram > 0, "Shaders didn't compile.");
        debug("Shaders good.");

        srand((GLuint)(100000 * glfwGetTime()));

        // Initialize Board, Grid, and first Block
        initBoard();
        initGrid();
        quiet_check(initBlock());

        // Set initial Camera state
        resetCamera();
        
        // Projection Matrix
        matrix_t* proj = coglMPerspectiveP(tau/8, 
                                           (float)wWidth/(float)wHeight,
                                           0.1f,1000.0f);

        GLfloat currentFrame;
        
        debug("Entering Loop.");
        // Render until you shouldn't.
        while(!glfwWindowShouldClose(w)) {
                if(gameOver) {
                        sleep(1);
                        break;
                }

                currentFrame = glfwGetTime();
                deltaTime = currentFrame - lastFrame;
                lastFrame = currentFrame;

                glfwPollEvents();
                //moveCamera();
                
                glClearColor(0.5f,0.5f,0.5f,1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                glUseProgram(shaderProgram);

                // Move the block down.
                scrollBlock();
                
                GLuint viewLoc = glGetUniformLocation(shaderProgram,"view");
                GLuint projLoc = glGetUniformLocation(shaderProgram,"proj");

                // Update View Matrix
                coglMDestroy(view);
                view = coglM4LookAtP(camera->pos,camera->tar,camera->up);

                // Set transformation Matrices
                glUniformMatrix4fv(viewLoc,1,GL_FALSE,view->m);
                glUniformMatrix4fv(projLoc,1,GL_FALSE,proj->m);

                // Draw Grid
                glBindVertexArray(gVAO);
                glDrawArrays(GL_LINES, 0, 128);
                glBindVertexArray(0);
                
                // Draw Block
                glBindVertexArray(bVAO);
                glDrawArrays(GL_TRIANGLES,0,36 * 4);
                glBindVertexArray(0);

                // Draw Board
                glBindVertexArray(fVAO);
                glDrawArrays(GL_TRIANGLES,0,7200);
                glBindVertexArray(0);

                // Always comes last.
                glfwSwapBuffers(w);
        }
        
        // Clean up.
        glfwTerminate();
        log_info("Thanks for playing!");

        return EXIT_SUCCESS;
 error:
        return EXIT_FAILURE;
}
