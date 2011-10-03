// compile this program using:
//   gcc -O3 -Wall -std=c99 -o scene scene.c bitmap.c -lglut
// Or, with cygwin:        (-mno-cygwin is only so the executable runs from windows)
//   gcc -mno-cygwin -O3 -Wall -std=c99 -o scene scene.c bitmap.c -lglut32 -lglu32 -lopengl32
// Or, use make via the supplied Makefile:     (For cygwin, install the package for make)
//   make

#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glut.h>

#include "bitmap.h"

// Type definitions for vertex-coordinates, normals, texture-coordinates, 
// and triangles (via the indices of 3 vertices).
typedef GLfloat vertex[3];
typedef GLfloat normal[3];
typedef GLfloat texCoord[2];
typedef GLint vertexIndex;
typedef vertexIndex triangle[3];

// A type for a mesh
typedef struct {         
    int nVertices;           //  The number of vertices in the mesh
    vertex* vertices;        //  Array with coordinates of vertices
    normal* normals;         //  Array with normals of vertices
    texCoord* texCoords;     //  Array with texture-coordinates of vertices
    int nTriangles;          //  The number of triangles in the mesh
    triangle* triangles;     //  Array of trangles via 3 indices into "vertices"
} mesh;

#define NMESH 54       // The number of meshes (in the models-textures dir)
mesh* meshes[NMESH];   // An array of pointers to the meshes - see getMesh

// A type for a 2D texture, with height and width in pixels
typedef struct {
    int height;
    int width;
    GLubyte *rgbData;   // Array of bytes with the colour data for the texture
} texture;

#define NTEXTURE 30     // The number of textures (in the models-textures dir)
texture* textures[NTEXTURE];   // An array of texture pointers - see getTexture

typedef struct {  
    // You'll need to add scale, rotation, material, mesh number, etc.,
    // to this structure
    float x,y,z;
} SceneObject;

#define MAXOBJECTS 256
SceneObject sceneObjs[MAXOBJECTS];  // An array with details of the objects in a scene
int nObjects=0;                     // How many objects there are in the scene currently.

void fileErr(char* fileName) {
    printf("Error reading file: %s\n", fileName);
    printf("If not in the CSSE labs, you will need to include the directory containing\n");
    printf("the models on the command line, or put it in the same folder as the exectutable.");
    exit(1);
}  

texture* loadTexture(char *fileName) {
    texture* t = malloc(sizeof (texture));
    BITMAPINFO *info;

    t->rgbData = LoadDIBitmap(fileName, &info);
    t->height=info->bmiHeader.biHeight;
    t->width=info->bmiHeader.biWidth;

    return t;
}

// The following works for the supplied .x files
// but probably not for .x files from other sources.
mesh* loadMesh(char* fileName) {
    mesh* m = malloc(sizeof (mesh));
    FILE* fp = fopen(fileName, "r");
    char line[256] = "";
    int lineBuffSize = 256;

    if(fp == NULL) fileErr(fileName);

    while(strcmp(line,"Mesh {\r\n") != 0 && strcmp(line,"Mesh {\n") != 0 )
	fgets(line, lineBuffSize, fp);

    fscanf(fp, "%d;\n", &(m->nVertices));
    m->vertices = malloc(m->nVertices * sizeof(vertex));
    for(int i=0; i < m->nVertices; i++)
	fscanf(fp, "%f; %f; %f;%*[,;]\n", &(m->vertices[i][0]), &(m->vertices[i][1]), &(m->vertices[i][2]) );

    fscanf(fp, "%d;\n", &(m->nTriangles));
    m->triangles = malloc(m->nTriangles * sizeof(triangle));
    for(int i=0; i < m->nTriangles; i++)
	fscanf(fp, "%*d; %d, %d, %d;%*[;,]", m->triangles[i], m->triangles[i]+1, m->triangles[i]+2);

    while(strcmp(line,"  MeshNormals {\r\n") != 0 && strcmp(line,"  MeshNormals {\n") != 0)
	fgets(line, lineBuffSize, fp);

    fgets(line, lineBuffSize, fp);
    m->normals = malloc(m->nVertices * sizeof(normal));
    for(int i=0; i < m->nVertices; i++)
	fscanf(fp, "%f; %f; %f;%*[;,]\n",
	       &(m->normals[i][0]), &(m->normals[i][1]), &(m->normals[i][2]));

    while(strcmp(line,"MeshTextureCoords {\r\n") != 0 && strcmp(line,"MeshTextureCoords {\n") != 0)
	fgets(line, lineBuffSize, fp);

    fgets(line, lineBuffSize, fp);
    m->texCoords = malloc(m->nVertices * sizeof(texCoord));
    for(int i=0; i < m->nVertices; i++)
	fscanf(fp, "%f;%f;%*[,;]\n", &(m->texCoords[i][0]), &(m->texCoords[i][1]) );
    fclose(fp);
    
    return m;
}

char dataDir[200];  // Stores the directory name for the meshes and textures.

// getMesh(i) loads mesh[i] if it isn't already loaded.  
// You must call getMesh(i) at least once before using mesh[i].
// [You may want to add to this function.]
void getMesh(int i) { // getMesh(i) loads mesh[i] if it isn't already loaded.  
    char fileName[220];
    if(i>=NMESH || i<0) {
	printf("Error in getMesh - wrong model number");
	exit(1);
    }
    if(meshes[i] != NULL)
	return;
    sprintf(fileName, "%s/model%d.x", dataDir, i+1);
    meshes[i] = loadMesh(fileName);
}

// getTexture(i) loads texture i if it isn't already loaded.
// After calling getTexture(i), you can make texture i the current texture using
//     glBindTexture(GL_TEXTURE_2D, i);    (Use i=0 to return to the default plain texture.)
// You can then scale the texture via:   (See the textbook, section 8.8.3.)
//      glMatrixMode(GL_TEXTURE);     
// You must call getTexture(i) at least once before using texture i.
void getTexture(int i) { // getTexture(i) loads texture i if it isn't already loaded.
    char fileName[220];
    if(i<1 || i>NTEXTURE) {
	printf("Error in getTexture - wrong texture number");
	exit(1);
    }
    if(textures[i-1] != NULL)
	return;
    sprintf(fileName, "%s/texture%d.bmp", dataDir, i);

    textures[i-1] = loadTexture(fileName);

    glBindTexture(GL_TEXTURE_2D, i);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textures[i-1]->width, textures[i-1]->height,
		 0, GL_RGB, GL_UNSIGNED_BYTE, textures[i-1]->rgbData);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, textures[i-1]->width, textures[i-1]->height, GL_RGB, 
		      GL_UNSIGNED_BYTE, textures[i-1]->rgbData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);  // Back to default texture
}

// You may want to use the following two when building your menus
char *textureMenuEntries[NTEXTURE] = {
  "1 Plain", "2 Rust", "3 Concrete", "4 Carpet", "5 Beach Sand", 
  "6 Rocky", "7 Brick", "8 Water", "9 Paper", "10 Marble", 
  "11 Wood", "12 Scales", "13 Fur", "14 Denim", "15 Hessian",
  "16 Orange Peel", "17 Ice Crystals", "18 Grass", "19 Corrugated Iron", "20 Styrofoam",
  "21 Bubble Wrap", "22 Leather", "23 Camouflage", "24 Asphalt", "25 Scratched Ice",
  "26 Rattan", "27 Snow", "28 Dry Mud", "29 Old Concrete", "30 Leopard Skin"
};

char *objectMenuEntries[NMESH] = {
  "1 Thin Dinosaur","2 Big Dog","3 Saddle Dinosaur", "4 Dragon", "5 Cleopatra", 
  "6 Bone I", "7 Bone II", "8 Rabbit", "9 Long Dragon", "10 Buddha", 
  "11 Sitting Rabbit", "12 Frog", "13 Cow", "14 Monster", "15 Sea Horse", 
  "16 Head", "17 Pelican", "18 Horse", "19 Kneeling Angel", "20 Porsche I", 
  "21 Truck", "22 Statue of Liberty", "23 Sitting Angel", "24 Metal Part", "25 Car", 
  "26 Apatosaurus", "27 Airliner", "28 Motorbike", "29 Dolphin", "30 Spaceman", 
  "31 Winnie the Pooh", "32 Shark", "33 Crocodile", "34 Toddler", "35 Fat Dinosaur", 
  "36 Chihuahua", "37 Sabre-toothed Tiger", "38 Lioness", "39 Fish", "40 Horse (head down)", 
  "41 Horse (head up)", "42 Skull", "43 Fighter Jet I", "44 Toad", "45 Convertible", 
  "46 Porsche II", "47 Hare", "48 Vintage Car", "49 Fighter Jet II", "50 Winged Monkey", 
  "51 Chef", "52 Parasaurolophus", "53 Rooster", "54 T-rex"
};

void mymenu(int id) {
   if(id == 99) exit(0);
}

void makeMenu() {
   // A very basic menu that you can add to.
   glutCreateMenu(mymenu);
   glutAddMenuEntry("Exit", 99);
   glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void display() {
   // You probably want to change both of the following.
   glClear(GL_COLOR_BUFFER_BIT);
   glFlush();
}

char *dirDefault1 = "models-textures"; 
char *dirDefault2 = "/cslinux/examples/CITS2231/project-files/models-textures";
int main(int argc, char **argv) {

    if(argc>1)
	  strcpy(dataDir, argv[1]);
    else if(opendir(dirDefault1))
	  strcpy(dataDir, dirDefault1);
    else if(opendir(dirDefault2))
      strcpy(dataDir, dirDefault2);
    else fileErr(dirDefault1);

    for(int i=0; i<NMESH; i++) meshes[i]=NULL;
    for(int i=0; i<NTEXTURE; i++) textures[i]=NULL;

    // The following is enough to run the program, but you'll
    // need to change it significantly.

    glutInit(&argc, argv);
    glutCreateWindow("Scene Editor");
    glutDisplayFunc(display);

    makeMenu();
    glutMainLoop();
}
