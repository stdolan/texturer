// This includes more headers than you will probably need.
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>

#include <iostream>
#include <sstream>
#include <map>
#include <utility> // pair
#include <list>
#include <vector>
#include <limits>
#include <string>
#include <cstring>
#include <fstream>
#include <algorithm> //swap
#include <cassert>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "main.hpp"
#include "data.h"

using namespace std;

struct Point_Light
{
    float position[4];
    float color[3];
    float attenuation_k;
};

// prototype functions
GLuint loadBMP_custom(const char * imagepath);

//gl prototypes
void init(void);
void reshape(int width, int height);
void display(void);

void init_lights();
void set_lights();
void draw_objects();
void initTextures();

void mouse_pressed(int button, int state, int x, int y);
void mouse_moved(int x, int y);
void key_pressed(unsigned char key, int x, int y);

void create_lights();

//gl globals
GLUquadricObj *quadratic;
GLuint image;

int mouse_x, mouse_y;
float mouse_scale_x, mouse_scale_y;

const float x_view_step = 90.0, y_view_step = 90.0;
float x_view_angle = 0, y_view_angle = 0;

bool is_pressed = false;

float cam_position[] = {0, 0, 2};

float cam_orientation_axis[] = {1, 1, 1};

float cam_orientation_angle = 0; // degrees

float near_param = 1, far_param = 10,
      left_param = -1, right_param = 1,
      top_param = 1, bottom_param = -1;
vector<Point_Light> lights;



/* This function splits a given string into tokens based on the specified delimiter
 * and stores the tokens into a preconstructed, passed-in vector.
 */
vector<string> &split(const string &str, char delim, vector<string> &tokens)
{
    stringstream ss(str);
    string token;

    while (getline(ss, token, delim))
        if(!token.empty())
            tokens.push_back(token);

    return tokens;
}

/* This function splits a given string into tokens based on the specified delimiter
 * and returns a new vector with all the tokens.
 */
vector<string> split(const string &str, char delim)
{
    vector<string> tokens;
    split(str, delim, tokens);
    return tokens;
}

/* String to integer conversion.
 */
float stoi(const string str)
{
    return atoi(str.c_str());
}

/* String to float conversion.
 */
float stof(const string str)
{
    return atof(str.c_str());
}

/* This function fills a passed-in mesh data container with a list of vertices and
 * faces specified by the given .obj file.
 */
void parse(Mesh *mesh_data, string file_name)
{
    mesh_data->points = new vector<point*>;
    mesh_data->faces = new vector<face*>;

    string read_line;
    ifstream file_in(file_name.c_str());

    if(file_in.is_open())
    {
        while(getline(file_in, read_line))
        {
            vector<string> tokens = split(read_line, ' ');

            if(tokens[0].at(0) == 'v')
            {
                point *p = new point;
                p->x = stof(tokens[1]);
                p->y = stof(tokens[2]);
                p->z = stof(tokens[3]);

                mesh_data->points->push_back(p);
            }
            else if(tokens[0].at(0) == 'f')
            {
                face *f = new face;
                f->ind1 = stoi(tokens[1]);
                f->ind2 = stoi(tokens[2]);
                f->ind3 = stoi(tokens[3]);

                mesh_data->faces->push_back(f);
            }
        }
    }
    else
    {
        cerr << "Specified input file not found!" << endl;
        exit(1);
    }
}



//draws shaded objects. takes in iv file, writes in ppm format to Canvas.txt. 
int main(int argc, char ** argv)
{
    Mesh * containerP = new Mesh;
    printf("Parsing...\n");
    parse(containerP, argv[1]);
    printf("Finished parsing.\n");

    vector<point*> * points = containerP->points;
    vector<face*> * faces = containerP->faces;
        
    //TODO: use points and faces, to create half edge data structure.

    
//use opengl stuff --------------------------------------
    int xres = 500;
    int yres = 500;
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(xres, yres);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("Object");
    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse_pressed);
    glutMotionFunc(mouse_moved);
    glutKeyboardFunc(key_pressed);
    glutMainLoop();

    return 0;
}

GLuint loadBMP_custom(const char * imagepath)
{
    // Data read from the header of the BMP file
    unsigned char header[54]; // Each BMP file begins by a 54-bytes header
    unsigned int dataPos;     // Position in the file where the actual data begins
    unsigned int width, height;
    unsigned int imageSize;   // = width*height*3
    // Actual RGB data
    unsigned char * data;

    // Open the file
    printf("filename:%s\n",imagepath);
    FILE * file = fopen(imagepath,"rb");
    if (!file)                              {printf("Image could not be opened\n"); return 0;}

    if ( fread(header, 1, 54, file)!=54 ){ // If not 54 bytes read : problem
        printf("Not a correct BMP file\n");
        return false;
    }

    if ( header[0]!='B' || header[1]!='M' ){
        printf("Not a correct BMP file\n");
        return 0;
    }

    // Read ints from the byte array
    dataPos    = *(int*)&(header[0x0A]);
    imageSize  = *(int*)&(header[0x22]);
    width      = *(int*)&(header[0x12]);
    height     = *(int*)&(header[0x16]);

    // Create a buffer
    data = new unsigned char [imageSize];
     
    // Read the actual data from the file into the buffer
    fread(data,1,imageSize,file);
     
    //Everything is in memory now, the file can be closed
    fclose(file);

//----------------- finished loading bmp file. important variables: width, height, data

    //TODO: give texture to opengl



    //return ?
}

void initTextures()
{
    //TODO: change strawberry.bmp to your own .bmp file.
    image = loadBMP_custom("./data/strawberry.bmp");
}

void init(void)
{
    glShadeModel(GL_SMOOTH);
    glEnable(GL_DEPTH_TEST);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glFrustum(left_param, right_param,
              bottom_param, top_param,
              near_param, far_param);
    glMatrixMode(GL_MODELVIEW); 

    quadratic = gluNewQuadric();
    gluQuadricNormals(quadratic, GLU_SMOOTH);
    gluQuadricTexture(quadratic, GL_TRUE);

    initTextures();

    create_lights();
    init_lights();
}

void reshape(int width, int height)
{
    height = (height == 0) ? 1 : height;
    width = (width == 0) ? 1 : width;
    glViewport(0, 0, width, height);
    mouse_scale_x = (float) (right_param - left_param) / (float) width;
    mouse_scale_y = (float) (top_param - bottom_param) / (float) height;
    glutPostRedisplay();
}

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glRotatef(-cam_orientation_angle,
              cam_orientation_axis[0], cam_orientation_axis[1], cam_orientation_axis[2]);
    glTranslatef(-cam_position[0], -cam_position[1], -cam_position[2]);
    set_lights();
    glEnable(GL_TEXTURE_2D);
    draw_objects();
    glutSwapBuffers();
}

void init_lights()
{
    glEnable(GL_LIGHTING);
    int num_lights = lights.size();
    for(int i = 0; i < num_lights; ++i)
    {
        int light_id = GL_LIGHT0 + i;
        glEnable(light_id);
        glLightfv(light_id, GL_AMBIENT, lights[i].color);
            glLightfv(light_id, GL_DIFFUSE, lights[i].color);
            glLightfv(light_id, GL_SPECULAR, lights[i].color);
        glLightf(light_id, GL_QUADRATIC_ATTENUATION, lights[i].attenuation_k);
    }
}

void draw_objects()
{
    float colrs1[3]; float colrs2[3]; float colrs3[3]; 
    colrs1[0]=.5; colrs1[1]=.5; colrs1[2]=.5;
    glMaterialfv(GL_FRONT, GL_AMBIENT, colrs1);

    colrs2[0]=.9; colrs2[1]=.9; colrs2[2]=.9;
    glMaterialfv(GL_FRONT, GL_DIFFUSE, colrs2);

    colrs3[0]=.3; colrs3[1]=.3; colrs3[2]=.3;    
    glMaterialfv(GL_FRONT, GL_SPECULAR, colrs3);

    glMaterialf(GL_FRONT, GL_SHININESS, 8);

    glPushMatrix();

    glRotatef(x_view_angle, 0, 1, 0);
    glRotatef(y_view_angle, 1, 0, 0);
    

    //TODO: draw faces. repeat: Texture coordinate, normal coordinate, vertex coordinate.


    glPopMatrix();
}

void set_lights()
{
    int num_lights = lights.size();
    
    for(int i = 0; i < num_lights; ++i)
    {
        int light_id = GL_LIGHT0 + i;
        
        glLightfv(light_id, GL_POSITION, lights[i].position);
    }
}
void mouse_pressed(int button, int state, int x, int y)
{
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        mouse_x = x;
        mouse_y = y;
        is_pressed = true;
    }
    else if(button == GLUT_LEFT_BUTTON && state == GLUT_UP)
    {
        is_pressed = false;
    }
}
void mouse_moved(int x, int y)
{
    if(is_pressed)
    {
        x_view_angle += ((float) x - (float) mouse_x) * mouse_scale_x * x_view_step;
        y_view_angle += ((float) y - (float) mouse_y) * mouse_scale_y * y_view_step;

        mouse_x = x;
        mouse_y = y;
        glutPostRedisplay();
    }
}
float deg2rad(float angle)
{
    return angle * M_PI / 180.0;
}
void key_pressed(unsigned char key, int x, int y)
{
    if(key == 'q')
    {
        exit(0);
    }
}
void create_lights()
{
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Light 1 Below
    ///////////////////////////////////////////////////////////////////////////////////////////////
    
    Point_Light light1;
    
    light1.position[0] = -1;
    light1.position[1] = 0;
    light1.position[2] = 1;
    light1.position[3] = 1;
    
    light1.color[0] = 1;
    light1.color[1] = 1;
    light1.color[2] = 1;
    light1.attenuation_k = 0;
    
    lights.push_back(light1);
    
    
}

