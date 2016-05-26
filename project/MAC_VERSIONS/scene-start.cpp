/**
 * CITS3003 Project
 * Tahmer Hijjawi 21130321
 * Jack Scott	21504053
 * 2016 Sem 1
 */

#include "Angel.h"

#include <OpenGL/gl3.h>

#include <stdlib.h>
#include <dirent.h>
#include <time.h>
#include <fstream>


// Open Asset Importer header files (in ../../assimp--3.0.1270/include)
// This is a standard open source library for loading meshes, see gnatidread.h
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

GLint windowHeight=640, windowWidth=960;

// gnatidread.cpp is the CITS3003 "Graphics n Animation Tool Interface & Data
// Reader" code.  This file contains parts of the code that you shouldn't need
// to modify (but, you can).
#include "gnatidread.h"
#include "gnatidread2.h"

using namespace std;        // Import the C++ standard functions (e.g., min)


// IDs for the GLSL program and GLSL variables.
GLuint shaderProgram; // The number identifying the GLSL shader program
GLuint vPosition, vNormal, vTexCoord; // IDs for vshader input vars (from glGetAttribLocation)
GLuint projectionU, modelViewU; // IDs for uniform variables (from glGetUniformLocation)
GLuint timeParam;

// Part B - D: Set global bone variables
GLuint vBoneIDs, vBoneWeights, uBoneTransforms;

static float viewDist = 1.5; // Distance from the camera to the centre of the scene
static float camRotSidewaysDeg=0; // rotates the camera sideways around the centre
static float camRotUpAndOverDeg=20; // rotates the camera up and over the centre.

mat4 projection; // Projection matrix - set in the reshape function
mat4 view; // View matrix - set in the display function.

// These are used to set the window title
char lab[] = "Project1";
char *programName = NULL; // Set in main
int numDisplayCalls = 0; // Used to calculate the number of frames per second

//------Meshes----------------------------------------------------------------
// Uses the type aiMesh from ../../assimp--3.0.1270/include/assimp/mesh.h
//                           (numMeshes is defined in gnatidread.h)
aiMesh* meshes[numMeshes]; // For each mesh we have a pointer to the mesh to draw
GLuint vaoIDs[numMeshes]; // and a corresponding VAO ID from glGenVertexArrays

// Part B - D: declaration of the meshes array
const aiScene* scenes[numMeshes];

// -----Textures--------------------------------------------------------------
//                           (numTextures is defined in gnatidread.h)
texture* textures[numTextures]; // An array of texture pointers - see gnatidread.h
GLuint textureIDs[numTextures]; // Stores the IDs returned by glGenTextures

//------Scene Objects---------------------------------------------------------
//
// For each object in a scene we store the following
// Note: the following is exactly what the sample solution uses, you can do things differently if you want.
typedef struct {
    vec4 loc;
    float scale;
    float angles[3]; // rotations around X, Y and Z axes.
    float diffuse, specular, ambient; // Amount of each light component
    float shine;
    vec3 rgb;
    float brightness; // Multiplies all colours
    int meshId;
    int texId;
    float texScale;
    float distance, speed, direction, position;
    float pose;
    float waves;
    int ninjaon = 0; //is super ninja on toggle
    //these two new variables are for the scene physics
    vec4 velocity_f = (0.0, 0.0, 0.0, 0.0);
    vec4 deltaS = (0.0, 0.0, 0.0, 0.0);
} SceneObject;

const int maxObjects = 1024; // Scenes with more than 1024 objects seem unlikely

SceneObject sceneObjs[maxObjects]; // An array storing the objects currently in the scene.
int nObjects = 0;    // How many objects are currenly in the scene.
int currObject = -1; // The current object
int toolObj = -1;    // The object currently being modified
int ObjectPhysics = 0; //toggle for physics on == 1, off == 0;

// custom variables to start a timer for the physics calculations
clock_t startTime;
clock_t endTime;
clock_t clockTicksTaken;

//used to stop mouse actions from skipping
bool mouseEngaged = false;

static bool avoidSkip() {
  if (!mouseEngaged) {
    mouseEngaged = true;
    return false;
  } else return true;
}

//----------------------------------------------------------------------------
//
// Loads a texture by number, and binds it for later use.
void loadTextureIfNotAlreadyLoaded(int i)
{
    if (textures[i] != NULL) return; // The texture is already loaded.

    textures[i] = loadTextureNum(i); CheckError();
    glActiveTexture(GL_TEXTURE0); CheckError();

    // Based on: http://www.opengl.org/wiki/Common_Mistakes
    glBindTexture(GL_TEXTURE_2D, textureIDs[i]); CheckError();

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textures[i]->width, textures[i]->height,
                 0, GL_RGB, GL_UNSIGNED_BYTE, textures[i]->rgbData); CheckError();
    glGenerateMipmap(GL_TEXTURE_2D); CheckError();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); CheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); CheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); CheckError();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); CheckError();

    glBindTexture(GL_TEXTURE_2D, 0); CheckError(); // Back to default texture
}

//------Mesh loading----------------------------------------------------------
//
// The following uses the Open Asset Importer library via loadMesh in
// gnatidread.h to load models in .x format, including vertex positions,
// normals, and texture coordinates.
// You shouldn't need to modify this - it's called from drawMesh below.

void loadMeshIfNotAlreadyLoaded(int meshNumber)
{
    if (meshNumber>=numMeshes || meshNumber < 0) {
        printf("Error - no such model number");
        exit(1);
    }

    if (meshes[meshNumber] != NULL)
        return; // Already loaded

    // Part B - D: stores the scene for the mesh
    const aiScene* scene = loadScene(meshNumber);
    scenes[meshNumber] = scene;
    aiMesh* mesh = scene->mMeshes[0];
    meshes[meshNumber] = mesh;

    glBindVertexArrayAPPLE( vaoIDs[meshNumber] );

    // Create and initialize a buffer object for positions and texture coordinates, initially empty.
    // mesh->mTextureCoords[0] has space for up to 3 dimensions, but we only need 2.
    GLuint buffer[1];
    glGenBuffers( 1, buffer );
    glBindBuffer( GL_ARRAY_BUFFER, buffer[0] );
    glBufferData( GL_ARRAY_BUFFER, sizeof(float)*(3+3+3)*mesh->mNumVertices,
                  NULL, GL_STATIC_DRAW );

    int nVerts = mesh->mNumVertices;
    // Next, we load the position and texCoord data in parts.
    glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(float)*3*nVerts, mesh->mVertices );
    glBufferSubData( GL_ARRAY_BUFFER, sizeof(float)*3*nVerts, sizeof(float)*3*nVerts, mesh->mTextureCoords[0] );
    glBufferSubData( GL_ARRAY_BUFFER, sizeof(float)*6*nVerts, sizeof(float)*3*nVerts, mesh->mNormals);

    // Load the element index data
    GLuint elements[mesh->mNumFaces*3];
    for (GLuint i=0; i < mesh->mNumFaces; i++) {
        elements[i*3] = mesh->mFaces[i].mIndices[0];
        elements[i*3+1] = mesh->mFaces[i].mIndices[1];
        elements[i*3+2] = mesh->mFaces[i].mIndices[2];
    }

    GLuint elementBufferId[1];
    glGenBuffers(1, elementBufferId);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBufferId[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * mesh->mNumFaces * 3, elements, GL_STATIC_DRAW);

    // vPosition it actually 4D - the conversion sets the fourth dimension (i.e. w) to 1.0
    glVertexAttribPointer( vPosition, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );
    glEnableVertexAttribArray( vPosition );

    // vTexCoord is actually 2D - the third dimension is ignored (it's always 0.0)
    glVertexAttribPointer( vTexCoord, 3, GL_FLOAT, GL_FALSE, 0,
                           BUFFER_OFFSET(sizeof(float)*3*mesh->mNumVertices) );
    glEnableVertexAttribArray( vTexCoord );
    glVertexAttribPointer( vNormal, 3, GL_FLOAT, GL_FALSE, 0,
                           BUFFER_OFFSET(sizeof(float)*6*mesh->mNumVertices) );
    glEnableVertexAttribArray( vNormal );
    CheckError();

    // Part B - D: get boneIDs and boneWeights for each vertex from the imported mesh data
    GLint boneIDs[mesh->mNumVertices][4];
    GLfloat boneWeights[mesh->mNumVertices][4];
    getBonesAffectingEachVertex(mesh, boneIDs, boneWeights);

    GLuint buffers[2];
    glGenBuffers( 2, buffers );  // Add two vertex buffer objects

    glBindBuffer( GL_ARRAY_BUFFER, buffers[0] ); CheckError();
    glBufferData( GL_ARRAY_BUFFER, sizeof(int)*4*mesh->mNumVertices, boneIDs, GL_STATIC_DRAW ); CheckError();
    glVertexAttribPointer(vBoneIDs, 4, GL_INT, GL_FALSE, 0, BUFFER_OFFSET(0)); CheckError();
    glEnableVertexAttribArray(vBoneIDs);     CheckError();

    glBindBuffer( GL_ARRAY_BUFFER, buffers[1] );
    glBufferData( GL_ARRAY_BUFFER, sizeof(float)*4*mesh->mNumVertices, boneWeights, GL_STATIC_DRAW );
    glVertexAttribPointer(vBoneWeights, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(vBoneWeights);    CheckError();
}

//----------------------------------------------------------------------------

static void mouseClickOrScroll(int button, int state, int x, int y)
{
    if (button==GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        if (glutGetModifiers()!=GLUT_ACTIVE_SHIFT) activateTool(button);
        else activateTool(GLUT_LEFT_BUTTON);
    }
    else if (button==GLUT_LEFT_BUTTON && state == GLUT_UP) {
      mouseEngaged = false;
      deactivateTool();
    }
    else if (button==GLUT_MIDDLE_BUTTON && state==GLUT_DOWN) activateTool(button);
    else if (button==GLUT_MIDDLE_BUTTON && state==GLUT_UP) {
      mouseEngaged = false;
      deactivateTool();
    }
    else if (button == 3) { // scroll up
        viewDist = (viewDist < 0.0 ? viewDist : viewDist*0.8) - 0.05;
    }
    else if (button == 4) { // scroll down
        viewDist = (viewDist < 0.0 ? viewDist : viewDist*1.25) + 0.05;
    }
}

//----------------------------------------------------------------------------

static void mousePassiveMotion(int x, int y)
{
    mouseX=x;
    mouseY=y;
}

//----------------------------------------------------------------------------

mat2 camRotZ()
{
    return rotZ(-camRotSidewaysDeg) * mat2(10.0, 0, 0, -10.0);
}

//------callback functions for doRotate below and later-----------------------

static void adjustCamrotsideViewdist(vec2 cv)
{
  // cout << cv << endl;
  if (avoidSkip()) {
    camRotSidewaysDeg+=cv[0];
    viewDist+=cv[1];
  }
}

static void adjustcamSideUp(vec2 su)
{
  if (avoidSkip()) {
    camRotSidewaysDeg+=su[0];
    camRotUpAndOverDeg+=su[1];
  }
}

static void adjustLocXZ(vec2 xz)
{
  if (avoidSkip()) {
    sceneObjs[toolObj].loc[0]+=xz[0];
    sceneObjs[toolObj].loc[2]+=xz[1];
  }
}

static void adjustScaleY(vec2 sy)
{
  if (avoidSkip()) {
    sceneObjs[toolObj].scale+=sy[0];
    sceneObjs[toolObj].loc[1]+=sy[1];
  }
}

//----------------------------------------------------------------------------
//------Set the mouse buttons to rotate the camera----------------------------
//------around the centre of the scene.---------------------------------------
//----------------------------------------------------------------------------

static void doRotate()
{
    setToolCallbacks(adjustCamrotsideViewdist, mat2(400,0,0,-2),
                     adjustcamSideUp, mat2(400, 0, 0,-90) );
}

//------Add an object to the scene--------------------------------------------

static void addObject(int id)
{

    vec2 currPos = currMouseXYworld(camRotSidewaysDeg);
    sceneObjs[nObjects].loc[0] = currPos[0];
    sceneObjs[nObjects].loc[1] = 0.0;
    sceneObjs[nObjects].loc[2] = currPos[1];
    sceneObjs[nObjects].loc[3] = 1.0;

    if (id!=0 && id!=55)
        sceneObjs[nObjects].scale = 0.005;

    sceneObjs[nObjects].rgb[0] = 0.7; sceneObjs[nObjects].rgb[1] = 0.7;
    sceneObjs[nObjects].rgb[2] = 0.7; sceneObjs[nObjects].brightness = 1.0;

    sceneObjs[nObjects].diffuse = 1.0; sceneObjs[nObjects].specular = 0.5;
    sceneObjs[nObjects].ambient = 0.7; sceneObjs[nObjects].shine = 10.0;

    sceneObjs[nObjects].angles[0] = 0.0; sceneObjs[nObjects].angles[1] = 180.0;
    sceneObjs[nObjects].angles[2] = 0.0;

    sceneObjs[nObjects].meshId = id;
    sceneObjs[nObjects].texId = rand() % numTextures;
    sceneObjs[nObjects].texScale = 2.0;

    sceneObjs[nObjects].distance = 0.0;
    sceneObjs[nObjects].speed = 0.0;
    sceneObjs[nObjects].pose = 0.0;
    sceneObjs[nObjects].direction = 0.0;
    sceneObjs[nObjects].position = 0.0;

    //sets the speed and distance for the animated objects
    if(id > 55)
    {
      sceneObjs[nObjects].distance = 5000.0;
      sceneObjs[nObjects].speed = 100.0;
      sceneObjs[nObjects].pose = 1.0;
      sceneObjs[nObjects].direction = 1.0;
      sceneObjs[nObjects].position = 0.0;
    }
    if(id == 57)
    {
      sceneObjs[nObjects].angles[1] = 0.0;
    }

    //intialises the waves toggle to off for every object
    sceneObjs[nObjects].waves = 0.0;

    toolObj = currObject = nObjects++;
    setToolCallbacks(adjustLocXZ, camRotZ(),
                     adjustScaleY, mat2(0.05, 0, 0, 10.0) );
    glutPostRedisplay();
}

static void deleteObject() {
  for (int i = nObjects - 1; i > 0; i--) {
    if (currObject < i) {
      sceneObjs[i - 1] = sceneObjs[i];
    }
  }
  toolObj = currObject = --nObjects - 1;
}

static void duplicateObject() {
  memcpy(&sceneObjs[nObjects], &sceneObjs[currObject], sizeof(sceneObjs[nObjects]));
  sceneObjs[nObjects].loc += vec4(1.0, 0.0, 0.0, 0.0);
  toolObj = currObject = nObjects++;
}

//------The init function-----------------------------------------------------

void init( void )
{
    srand ( time(NULL) ); /* initialize random seed - so the starting scene varies */
    aiInit();

    // for (int i=0; i < numMeshes; i++)
    //     meshes[i] = NULL;

    glGenVertexArraysAPPLE(numMeshes, vaoIDs); CheckError(); // Allocate vertex array objects for meshes
    glGenTextures(numTextures, textureIDs); CheckError(); // Allocate texture objects

    // Load shaders and use the resulting shader program
    shaderProgram = InitShader( "vStart.glsl", "fStart.glsl" );

    glUseProgram( shaderProgram );
    CheckError();

    // Initialize the vertex position attribute from the vertex shader
    vPosition = glGetAttribLocation( shaderProgram, "vPosition" );
    vNormal = glGetAttribLocation( shaderProgram, "vNormal" );
    CheckError();

    // Likewise, initialize the vertex texture coordinates attribute.
    vTexCoord = glGetAttribLocation( shaderProgram, "vTexCoord" );
    CheckError();

    projectionU = glGetUniformLocation(shaderProgram, "Projection");
    modelViewU = glGetUniformLocation(shaderProgram, "ModelView");
    CheckError();

    // Part B - D: Initialize bone variables
    vBoneIDs = glGetAttribLocation( shaderProgram, "vBoneIDs");
    vBoneWeights = glGetAttribLocation( shaderProgram, "vBoneWeights");
    uBoneTransforms = glGetUniformLocation( shaderProgram, "uBoneTransforms");
    timeParam = glGetUniformLocation(shaderProgram, "time");
    CheckError();

    // Objects 0, and 1 are the ground and the first light.
    addObject(0); // Square for the ground
    sceneObjs[0].loc = vec4(0.0, 0.0, 0.0, 1.0);
    sceneObjs[0].scale = 10.0;
    sceneObjs[0].angles[0] = 90.0; // Rotate it.
    sceneObjs[0].texScale = 5.0; // Repeat the texture.

    addObject(55); // Sphere for the first light
    sceneObjs[1].loc = vec4(2.0, 1.0, 1.0, 1.0);
    sceneObjs[1].scale = 0.1;
    sceneObjs[1].texId = 0; // Plain texture
    sceneObjs[1].brightness = 0.8; // The light's brightness is 5 times this (below).

    addObject(55); // Sphere for the second light
    sceneObjs[2].loc = vec4(-2.0, 1.0, 1.0, 1.0);
    sceneObjs[2].scale = 0.2;
    sceneObjs[2].texId = 0; // Plain texture
    sceneObjs[2].brightness = 0.3; // The light's brightness is 5 times this (below).

    addObject(rand() % numMeshes); // A test mesh
    // addObject(56); // Monkey head
    // addObject(57); // Gingerbread Man

    // We need to enable the depth test to discard fragments that
    // are behind previously drawn fragments for the same pixel.
    glEnable( GL_DEPTH_TEST );
    doRotate(); // Start in camera rotate mode.
    glClearColor( 0.0, 0.0, 0.0, 1.0 ); /* black background */
}

//----------------------------------------------------------------------------

// Part B - D: animate object
void animateObject(int index)
{

    // calculate angles for object orientation
    float roll	= (float)((int)floor(sceneObjs[index].angles[0]) % 360) * M_PI / 180;
    float pitch	= (float)((int)floor(sceneObjs[index].angles[1]) % 360) * M_PI / 180;
    float yaw	= (float)((int)floor(sceneObjs[index].angles[2]) % 360) * M_PI / 180;

    // update object location and position
    vec4 change = sceneObjs[index].direction * sceneObjs[index].speed / 10000.0 * vec4(-cos(yaw) * sin(pitch), sin(roll) *  cos(pitch), -cos(pitch), 0.0);
    sceneObjs[index].loc += change;
    sceneObjs[index].position += sceneObjs[index].direction * sceneObjs[index].speed;


    // change direction if object reaches limits
    if (sceneObjs[index].position >= sceneObjs[index].distance) {
      sceneObjs[index].position = sceneObjs[index].distance;
      sceneObjs[index].direction *= -1.0;

    } else if (sceneObjs[index].position <= 0.0) {
      sceneObjs[index].position = 0.0;
      sceneObjs[index].direction *= -1.0;

    }

    // update frame
    if(sceneObjs[index].direction > 0)
    {
      sceneObjs[index].pose--;
      if(sceneObjs[index].pose < 0)
      {
        sceneObjs[index].pose = 40;
      }
    }
    if(sceneObjs[index].direction < 0)
    {
      sceneObjs[index].pose++;
      if(sceneObjs[index].pose > 40)
      {
        sceneObjs[index].pose = 1;
      }
    }


}
//applys gravity relative to time and the gravity constant to an object
void sceneGravity(int index)
{
  endTime = clock(); //gets the clock
  clockTicksTaken = endTime - startTime; //finds the difference

  double timeInSeconds = (clockTicksTaken / (double) CLOCKS_PER_SEC); //convert it to seconds

  float gravity = -0.38; //arbitrary can be changed for various effects

  //newtonian equations to caluclate velocity and displacement
  sceneObjs[index].velocity_f.w += timeInSeconds * gravity;
  sceneObjs[index].deltaS.w = (timeInSeconds * sceneObjs[index].velocity_f.w) + ((timeInSeconds * timeInSeconds ) * gravity * 0.5);
  //applys the desiplacement to the location of the object
  vec4 change = vec4(0.0, sceneObjs[index].deltaS.w, 0.0, 0.0);
  sceneObjs[index].loc += change;

  //if the object is below the floor make sure to set it to the floor and the other things to zero.
  if(sceneObjs[index].loc.y < 0.0)
  {
    sceneObjs[index].loc.y = 0.0;
    sceneObjs[index].velocity_f.w = 0.0;
    sceneObjs[index].deltaS.w = 0.0;
  }

}

//calcualtes the x, y and z components of an object that has jsut been thrown/shot from a cannon
void objectPhysics(int index)
{
  //get clock time again
  endTime = clock();
  clockTicksTaken = endTime - startTime;
  double timeInSeconds = (clockTicksTaken / (double) CLOCKS_PER_SEC);

  //wind resistance again change this for fun effects.
  float windres = -1.0;

  //in the y direction... this competes with gravity.
  sceneObjs[index].velocity_f.y += timeInSeconds * windres;
  sceneObjs[index].deltaS.y = (timeInSeconds * sceneObjs[index].velocity_f.y) + ((timeInSeconds * timeInSeconds ) * windres * 0.5);
  if(sceneObjs[index].velocity_f.y < 0.0)
  {
    sceneObjs[index].velocity_f.y = 0.0;
    sceneObjs[index].deltaS.y = 0.0;
  }
  //in the x direction
  sceneObjs[index].velocity_f.x += timeInSeconds * windres;
  sceneObjs[index].deltaS.x = (timeInSeconds * sceneObjs[index].velocity_f.x) + ((timeInSeconds * timeInSeconds ) * windres * 0.5);
  if(sceneObjs[index].velocity_f.x < 0.0)
  {
    sceneObjs[index].velocity_f.x = 0.0;
    sceneObjs[index].deltaS.x = 0.0;
  }
  //in the z direction
  sceneObjs[index].velocity_f.z += timeInSeconds * windres;
  sceneObjs[index].deltaS.z = (timeInSeconds * sceneObjs[index].velocity_f.z) + ((timeInSeconds * timeInSeconds ) * windres * 0.5);
  if(sceneObjs[index].velocity_f.z < 0.0)
  {
    sceneObjs[index].velocity_f.z = 0.0;
    sceneObjs[index].deltaS.z = 0.0;
  }

  //apply the full effects of the throw to the objects location.
  vec4 change = vec4(sceneObjs[index].deltaS);
  sceneObjs[index].loc += change;

}


//----------------------------------------------------------------------------

void drawMesh(int index)
{
    SceneObject sceneObj = sceneObjs[index];

    // Activate a texture, loading if needed.
    loadTextureIfNotAlreadyLoaded(sceneObj.texId);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureIDs[sceneObj.texId]);

    // Texture 0 is the only texture type in this program, and is for the rgb
    // colour of the surface but there could be separate types for, e.g.,
    // specularity and normals.
    glUniform1i( glGetUniformLocation(shaderProgram, "texture"), 0 );

    // Set the texture scale for the shaders
    glUniform1f( glGetUniformLocation( shaderProgram, "texScale"), sceneObj.texScale );

    // Set the projection matrix for the shaders
    glUniformMatrix4fv( projectionU, 1, GL_TRUE, projection );

    // Set the model matrix - this should combine translation, rotation and scaling based on what's
    // in the sceneObj structure (see near the top of the program).



    mat4 model = Translate(sceneObj.loc) * Scale(sceneObj.scale) * RotateZ(sceneObj.angles[2]) * RotateY(sceneObj.angles[1]) * RotateX(-sceneObj.angles[0]);
  //  printf("%f, %f, %f\n", sceneObj.loc.x, sceneObj.loc.y, sceneObj.loc.z);
    // Set the model-view matrix for the shaders
    glUniformMatrix4fv( modelViewU, 1, GL_TRUE, view * model );

    // Activate the VAO for a mesh, loading if needed.
    loadMeshIfNotAlreadyLoaded(sceneObj.meshId);
    CheckError();
    glBindVertexArrayAPPLE( vaoIDs[sceneObj.meshId] );
    CheckError();

    //if physics is enabled then we do these
    if(ObjectPhysics == 1)
    {
      //if an object is above the floor it needs to be brought back to earth.
      if(sceneObj.loc.y > 0 && index != 1 && index != 2)
      {
        sceneGravity(index);
      }
      //apply velocity translations.
      objectPhysics(index);
    }

    //animate the gingerbread man and the monkey
    if (sceneObj.meshId == 56 || sceneObj.meshId == 57)
    {
      animateObject(index);
    }
    else
    {
      sceneObj.pose = 0.0;
    }

    int nBones = meshes[sceneObj.meshId]->mNumBones;
    if(nBones == 0)
        // If no bones, just a single identity matrix is used
        nBones = 1;

    // get boneTransforms for the first (0th) animation at the given
    // time (a float measured in frames)
    // (Replace <POSE_TIME> appropriately with a float expression
    // giving the time relative to the start of the animation,
    // measured in frames, like the frame numbers in Blender.)

    // mat4 boneTransforms[nBones];     // was: mat4 boneTransforms[mesh->mNumBones];
    mat4 *boneTransforms;
    boneTransforms = (mat4 *)malloc(sizeof(mat4)*nBones);
    // initialize each boneTransforms[i] to a zero matrix
    for (int i=0; i < nBones; i++)
        boneTransforms[i] = mat4(0.0);
    calculateAnimPose(meshes[sceneObj.meshId], scenes[sceneObj.meshId], 0, sceneObj.pose, boneTransforms);
    glUniformMatrix4fv(uBoneTransforms, nBones, GL_TRUE, (const GLfloat *)boneTransforms);


    glDrawElements(GL_TRIANGLES, meshes[sceneObj.meshId]->mNumFaces * 3,
                       GL_UNSIGNED_INT, NULL);
    CheckError();
}

//----------------------------------------------------------------------------

void display( void )
{
  numDisplayCalls++;

  glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
  CheckError(); // May report a harmless GL_INVALID_OPERATION with GLEW on the first frame

  // Set the view matrix. To start with this just moves the camera
  // backwards.  You'll need to add appropriate rotations.

  view = Translate(0.0, 0.0, -viewDist) * RotateX(camRotUpAndOverDeg) * RotateY(camRotSidewaysDeg);

  SceneObject lightObj1 = sceneObjs[1];
  SceneObject lightObj2 = sceneObjs[2];
  vec4 lightPosition1 = view * lightObj1.loc ;
  vec4 lightPosition2 = lightObj2.loc ;

  glUniform4fv( glGetUniformLocation(shaderProgram, "LightPosition1"), 1, lightPosition1);
  glUniform4fv( glGetUniformLocation(shaderProgram, "LightPosition2"), 1, lightPosition2);
  CheckError();

  for (int i=0; i < nObjects; i++) {
      SceneObject so = sceneObjs[i];

      vec3 rgb = so.rgb * lightObj1.rgb * lightObj2.rgb * so.brightness * lightObj1.brightness * lightObj2.brightness * 2.0;
      glUniform3fv( glGetUniformLocation(shaderProgram, "AmbientProduct"), 1, so.ambient * rgb );
      CheckError();
      glUniform3fv( glGetUniformLocation(shaderProgram, "DiffuseProduct"), 1, so.diffuse * rgb );
      glUniform3fv( glGetUniformLocation(shaderProgram, "SpecularProduct"), 1, so.specular * vec3(1.0, 1.0, 1.0) );
      glUniform1f( glGetUniformLocation(shaderProgram, "Shininess"), so.shine );
      glUniform1f( glGetUniformLocation(shaderProgram, "TexScale"), so.texScale );
      glUniform1f( glGetUniformLocation(shaderProgram, "Waves"), so.waves );
      glUniform1f( timeParam, glutGet(GLUT_ELAPSED_TIME) );
      CheckError();

      drawMesh(i);
  }

  glutSwapBuffers();
}

//----------------------------------------------------------------------------
//------Menus-----------------------------------------------------------------
//----------------------------------------------------------------------------

static void objectMenu(int id)
{
    deactivateTool();
    addObject(id);
}

static void texMenu(int id)
{
    deactivateTool();
    if (currObject>=0) {
        sceneObjs[currObject].texId = id;
        glutPostRedisplay();
    }
}

static void groundMenu(int id)
{
    deactivateTool();
    sceneObjs[0].texId = id;
    glutPostRedisplay();
}

static void adjustBrightnessY(vec2 by)
{
  if (avoidSkip()) {
    sceneObjs[toolObj].brightness+=by[0];
    sceneObjs[toolObj].loc[1]+=by[1];

    // add limits to brightness levels
    if (sceneObjs[toolObj].brightness > 1.0) sceneObjs[toolObj].brightness = 1.0;
    if (sceneObjs[toolObj].brightness < 0.0) sceneObjs[toolObj].brightness = 0.0;
  }
}

static void adjustRedGreen(vec2 rg)
{
  if (avoidSkip()) {
    sceneObjs[toolObj].rgb[0]+=rg[0];
    sceneObjs[toolObj].rgb[1]+=rg[1];
  }
}

static void adjustBlueBrightness(vec2 bl_br)
{
  if (avoidSkip()) {
    sceneObjs[toolObj].rgb[2]+=bl_br[0];
    sceneObjs[toolObj].brightness+=bl_br[1];
  }
}

static void adjustAmbientDiffuse(vec2 am_df)
{
  if (avoidSkip()) {
    sceneObjs[toolObj].ambient+=(am_df[0] * 10);
    sceneObjs[toolObj].diffuse+=(am_df[1] * 10);
  }
}

static void adjustSpecularShine(vec2 sp_sh)
{
  if (avoidSkip()) {
    sceneObjs[toolObj].specular+=(sp_sh[0] * 5);
    sceneObjs[toolObj].shine+=(sp_sh[1] * 5);
  }
  if (sceneObjs[toolObj].specular < 0.0) sceneObjs[toolObj].specular = 0.0;
  if (sceneObjs[toolObj].shine > 128) sceneObjs[toolObj].shine = 128;
  if (sceneObjs[toolObj].shine < 0.0) sceneObjs[toolObj].shine = 0.0;
}

static void lightMenu(int id)
{
    deactivateTool();
    if (id == 70) {
        toolObj = 1;
        setToolCallbacks(adjustLocXZ, camRotZ(),
                         adjustBrightnessY, mat2( 1.0, 0.0, 0.0, 10.0) );
    }
    else if (id >= 71 && id <= 74) {
        toolObj = 1;
        setToolCallbacks(adjustRedGreen, mat2(1.0, 0, 0, 1.0),
                         adjustBlueBrightness, mat2(1.0, 0, 0, 1.0) );
    }
    else if (id == 80) {
        toolObj = 2;
        setToolCallbacks(adjustLocXZ, camRotZ(),
                         adjustBrightnessY, mat2( 1.0, 0.0, 0.0, 10.0) );
    }
    else if (id >= 81 && id <= 84) {
        toolObj = 2;
        setToolCallbacks(adjustRedGreen, mat2(1.0, 0, 0, 1.0),
                         adjustBlueBrightness, mat2(1.0, 0, 0, 1.0) );
    }
    else {
        printf("Error in lightMenu\n");
        exit(1);
    }
}

static void fileSaveMenu(int id)
{
    if (id == 90)
    {
        ofstream saveFile("scene-image.txt", ios::out | ios::binary);
        if(!saveFile) {
            std::cout << "Cannot open file..." << std::endl;
            return;
        }
        saveFile << nObjects << endl;
	    saveFile << currObject << endl;
	    saveFile << toolObj << endl;
	    saveFile << viewDist << endl;
	    saveFile << camRotSidewaysDeg << endl;
	    saveFile << camRotUpAndOverDeg;
        for (int i = 0; i < nObjects; i++) {
            saveFile.write((char *) &sceneObjs[i], sizeof(sceneObjs[i]));
        }
        saveFile.close();
    }
    else if (id == 91)
    {
        ifstream loadFile("scene-image.txt", ios::in | ios::binary);
        if(!loadFile) {
            std::cout << "Cannot open file..." << std::endl;
            return;
        }
        loadFile >> nObjects;
	    loadFile >> currObject;
	    loadFile >> toolObj;
	    loadFile >> viewDist;
	    loadFile >> camRotSidewaysDeg;
	    loadFile >> camRotUpAndOverDeg;
        for (int i = 0; i < nObjects; i++) {
            loadFile.read((char *) &sceneObjs[i], sizeof(sceneObjs[i]));
        }
        loadFile.close();
    }
}

static int createArrayMenu(int size, const char menuEntries[][128], void(*menuFn)(int))
{
    int nSubMenus = (size-1)/10 + 1;
    int subMenus[nSubMenus];

    for (int i=0; i < nSubMenus; i++) {
        subMenus[i] = glutCreateMenu(menuFn);
        for (int j = i*10+1; j <= min(i*10+10, size); j++)
            glutAddMenuEntry( menuEntries[j-1] , j);
        CheckError();
    }
    int menuId = glutCreateMenu(menuFn);

    for (int i=0; i < nSubMenus; i++) {
        char num[6];
        sprintf(num, "%d-%d", i*10+1, min(i*10+10, size));
        glutAddSubMenu(num,subMenus[i]);
        CheckError();
    }
    return menuId;
}

static void materialMenu(int id)
{
    deactivateTool();
    if (currObject < 0) return;
    if (id==10) {
        toolObj = currObject;
        setToolCallbacks(adjustRedGreen, mat2(1, 0, 0, 1),
                         adjustBlueBrightness, mat2(1, 0, 0, 1) );
    } else if (id==20) {
      toolObj = currObject;
      setToolCallbacks(adjustAmbientDiffuse, mat2(1, 0, 0, 1),
                       adjustSpecularShine, mat2(10, 0, 0, 30) );
    }
    // You'll need to fill in the remaining menu items here.
    else {
        printf("Error in materialMenu\n");
    }
}

//my custom physicsMenu
static void physicsMenu(int id)
{

  switch(id){
    case 109:
        {
          if(ObjectPhysics == 1)
          {
            ObjectPhysics = 0;
            startTime = clock();
          }
          else if(ObjectPhysics == 0)
          {
            ObjectPhysics = 1;
          }
          break;
        }
    case 120:
        printf("v_x set to 2\n" );
        sceneObjs[currObject].velocity_f.x = 4.0;
        startTime = clock();
        break;
    case 121:
        sceneObjs[currObject].velocity_f.x = 4.0;
        sceneObjs[currObject].velocity_f.y = 2.0;
        startTime = clock();
        break;
    case 122:
        sceneObjs[currObject].velocity_f.x = 4.0;
        sceneObjs[currObject].velocity_f.z = 4.0;
        startTime = clock();
        break;
    case 130:
        sceneObjs[currObject].velocity_f.y = 2.0;
        startTime = clock();
        break;
    case 131:
        sceneObjs[currObject].velocity_f.y = 2.0;
        sceneObjs[currObject].velocity_f.z = 4.0;
        startTime = clock();
        break;
    case 140:
        sceneObjs[currObject].velocity_f.z = 4.0;
        startTime = clock();
        break;
    case 141:
        sceneObjs[currObject].velocity_f.x = 4.0;
        sceneObjs[currObject].velocity_f.y = 2.0;
        sceneObjs[currObject].velocity_f.z = 4.0;
        startTime = clock();
        break;
    }
}

static void adjustAngleYX(vec2 angle_yx)
{
  if (avoidSkip()) {
    sceneObjs[currObject].angles[1]+=angle_yx[0];
    sceneObjs[currObject].angles[0]+=angle_yx[1];
  }
}

static void adjustAngleZTexscale(vec2 az_ts)
{
  if (avoidSkip()) {
    sceneObjs[currObject].angles[2]+=az_ts[0];
    sceneObjs[currObject].texScale+=az_ts[1];
  }
}

// Part B - D: add callbacks to change distance
static void adjustObjectDistance(vec2 ob_di)
{
  if (avoidSkip()) {
    sceneObjs[currObject].distance += ob_di[0];
    sceneObjs[currObject].speed += ob_di[1];
    if(sceneObjs[currObject].speed <= 0)
    {
      sceneObjs[currObject].speed = 0;
    }
    if(sceneObjs[currObject].distance <= 0)
    {
      sceneObjs[currObject].distance = 0.0;
    }
  }
}

// Part B - D: add callbacks to change speed

static void mainmenu(int id)
{
    deactivateTool();
    if (id == 41 && currObject>=0) {
        toolObj=currObject;
        setToolCallbacks(adjustLocXZ, camRotZ(),
                         adjustScaleY, mat2(0.05, 0, 0, 10) );
    }
    if (id == 42 && currObject>=0) {
        deleteObject();
    }
    if (id == 43 && currObject>=0) {
        duplicateObject();
    }
    if (id == 50)
        doRotate();
    if (id == 55 && currObject>=0) {
        setToolCallbacks(adjustAngleYX, mat2(400, 0, 0, -400),
                         adjustAngleZTexscale, mat2(400, 0, 0, 15) );
    }
    if (id == 95 && currObject>=0) {
        setToolCallbacks(adjustObjectDistance, mat2(5000.0, 0, 0, 500.0),
                         adjustObjectDistance, mat2(5000.0, 0, 0, 500.0));
    }

    //this is super ninja hologram its just toggles the speed between very fast and regular which produces
    //a hologram type effect which i find entertaining.
    if (id == 96 && currObject>=0) {
        if(sceneObjs[currObject].ninjaon == 1)
        {
          sceneObjs[currObject].speed = 100.0;
          sceneObjs[currObject].distance = 5000;
          sceneObjs[currObject].ninjaon = 0;
        }
        else if(sceneObjs[currObject].ninjaon == 0)
        {
          sceneObjs[currObject].speed = 10000.0;
          sceneObjs[currObject].distance = 2500;
          sceneObjs[currObject].ninjaon = 1;
        }

    }
    //toggle waves for ground and object.
    if (id == 97 && currObject >= 0)
    {
      toolObj = currObject;
      if(sceneObjs[toolObj].waves == 2.0)
      {
        sceneObjs[toolObj].waves = 0.0;
      }
      else
      {
        sceneObjs[toolObj].waves = 2.0;
      }
    }
    if (id == 98)
    {
      if(sceneObjs[0].waves == 1.0)
      {
        sceneObjs[0].waves = 0.0;
      }
      else
      {
        sceneObjs[0].waves = 1.0;
      }
    }

    if (id == 99) exit(0);
}

static void makeMenu()
{
    // Correct number of meshes: (numMeshes - 1)
    int objectId = createArrayMenu((numMeshes - 1), objectMenuEntries, objectMenu);

    int materialMenuId = glutCreateMenu(materialMenu);
    glutAddMenuEntry("R/G/B/All",10);
    glutAddMenuEntry("Ambient/Diffuse/Specular/Shine",20);
    int physicsMenuId = glutCreateMenu(physicsMenu);
    glutAddMenuEntry("toggle physics", 109);
    glutAddMenuEntry("Throw X axis", 120);
    glutAddMenuEntry("Throw XY axis", 121);
    glutAddMenuEntry("Throw XZ axis", 122);
    glutAddMenuEntry("Throw Y axis", 130);
    glutAddMenuEntry("Throw YZ axis", 131);
    glutAddMenuEntry("Throw Z axis", 140);
    glutAddMenuEntry("Throw ZXY axis", 141);

    int texMenuId = createArrayMenu(numTextures, textureMenuEntries, texMenu);
    int groundMenuId = createArrayMenu(numTextures, textureMenuEntries, groundMenu);

    int lightMenuId = glutCreateMenu(lightMenu);
    glutAddMenuEntry("Move Light 1",70);
    glutAddMenuEntry("R/G/B/All Light 1",71);
    glutAddMenuEntry("Move Light 2",80);
    glutAddMenuEntry("R/G/B/All Light 2",81);

    int fileSaveMenuId = glutCreateMenu(fileSaveMenu);
    glutAddMenuEntry("Save", 90);
    glutAddMenuEntry("Load", 91);

    glutCreateMenu(mainmenu);
    glutAddMenuEntry("Rotate/Move Camera",50);
    glutAddSubMenu("Add object", objectId);
    glutAddMenuEntry("Position/Scale", 41);
    glutAddMenuEntry("Delete Object", 42);
    glutAddMenuEntry("Duplicate Object", 43);
    glutAddMenuEntry("Rotation/Texture Scale", 55);
    glutAddSubMenu("Material", materialMenuId);
    glutAddSubMenu("Texture",texMenuId);
    glutAddSubMenu("Ground Texture",groundMenuId);
    glutAddSubMenu("Lights",lightMenuId);
    glutAddSubMenu("Physics Menu",physicsMenuId);
    glutAddSubMenu("Save/Load Current Object",fileSaveMenuId);
    glutAddMenuEntry("Distance/Speed", 95);
    glutAddMenuEntry("Super Ninja Hologram!", 96);
    glutAddMenuEntry("Object Waves",97);
    glutAddMenuEntry("Ground Waves",98);
    glutAddMenuEntry("EXIT", 99);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}
//----------------------------------------------------------------------------

void keyboard( unsigned char key, int x, int y )
{
    switch ( key ) {
    case 033:
        exit( EXIT_SUCCESS );
        break;
    }
}

//----------------------------------------------------------------------------

void idle( void )
{
    glutPostRedisplay();
}

//----------------------------------------------------------------------------

void reshape( int width, int height )
{
  windowWidth = width;
  windowHeight = height;

  glViewport(0, 0, width, height);

  // You'll need to modify this so that the view is similar to that in the
  // sample solution.
  // In particular:
  //     - the view should include "closer" visible objects (slightly tricky)
  //     - when the width is less than the height, the view should adjust so
  //         that the same part of the scene is visible across the width of
  //         the window.

  GLfloat nearDist = 0.001;
  if(width >= height)
  {
    projection = Frustum(-nearDist*(float)width/(float)height,
                         nearDist*(float)width/(float)height,
                         -nearDist,
                          nearDist,
                         nearDist, 100.0);
  }
  if(height > width)
  {
    projection = Frustum(-nearDist,
                         nearDist,
                         -nearDist*(float)height/(float)width,
                          nearDist*(float)height/(float)width,
                         nearDist, 100.0);
  }
}

//----------------------------------------------------------------------------

void timer(int unused)
{
    char title[256];
    sprintf(title, "%s %s: %d Frames Per Second @ %d x %d",
                    lab, programName, numDisplayCalls, windowWidth, windowHeight );

    glutSetWindowTitle(title);

    numDisplayCalls = 0;
    glutTimerFunc(1000, timer, 1);
}

//----------------------------------------------------------------------------

char dirDefault1[] = "models-textures";
char dirDefault3[] = "/tmp/models-textures";
char dirDefault4[] = "/d/models-textures";
char dirDefault2[] = "/cslinux/examples/CITS3003/project-files/models-textures";

void fileErr(char* fileName)
{
    printf("Error reading file: %s\n", fileName);
    printf("When not in the CSSE labs, you will need to include the directory containing\n");
    printf("the models on the command line, or put it in the same folder as the exectutable.");
    exit(1);
}

//----------------------------------------------------------------------------

int main( int argc, char* argv[] )
{


    // Get the program name, excluding the directory, for the window title
    programName = argv[0];
    for (char *cpointer = argv[0]; *cpointer != 0; cpointer++)
        if (*cpointer == '/' || *cpointer == '\\') programName = cpointer+1;

    // Set the models-textures directory, via the first argument or some handy defaults.
    if (argc > 1)
        strcpy(dataDir, argv[1]);
    else if (opendir(dirDefault1)) strcpy(dataDir, dirDefault1);
    else if (opendir(dirDefault2)) strcpy(dataDir, dirDefault2);
    else if (opendir(dirDefault3)) strcpy(dataDir, dirDefault3);
    else if (opendir(dirDefault4)) strcpy(dataDir, dirDefault4);
    else fileErr(dirDefault1);

    glutInit( &argc, argv );
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize( windowWidth, windowHeight );

    //glutInitContextVersion( 3, 2);
    //glutInitContextProfile( GLUT_CORE_PROFILE );            // May cause issues, sigh, but you
    //glutInitContextProfile( GLUT_COMPATIBILITY_PROFILE ); // should still use only OpenGL 3.2 Core
                                                            // features.
    glutCreateWindow( "Initialising..." );

    // glewInit(); // With some old hardware yields GL_INVALID_ENUM, if so use glewExperimental.
    CheckError(); // This bug is explained at: http://www.opengl.org/wiki/OpenGL_Loading_Library

    init();
    CheckError();

    glutDisplayFunc( display );
    glutKeyboardFunc( keyboard );
    glutIdleFunc( idle );

    glutMouseFunc( mouseClickOrScroll );
    glutPassiveMotionFunc(mousePassiveMotion);
    glutMotionFunc( doToolUpdateXY );

    glutReshapeFunc( reshape );
    glutTimerFunc( 1000, timer, 1 );
    CheckError();

    makeMenu();
    CheckError();

    printf("%s\n%s\n",
        glGetString(GL_RENDERER),  // e.g. Intel HD Graphics 3000 OpenGL Engine
        glGetString(GL_VERSION)    // e.g. 3.2 INTEL-8.0.61
        );

    glutMainLoop();
    return 0;
}
