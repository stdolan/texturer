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

#include <Eigen/Eigen> // normals

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "main.hpp"
#include "data.h"

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

using namespace std;
using namespace Eigen;

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
void init_textures(const char *imagepath);

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

// Various half edge structures
vector<HE*> half_edges;
vector<HEF*> half_edge_faces;
vector<HEV*> half_edge_vertices;
map<HEV*, Tvec*> tvecs; 
map< pair < int, int > , HE *> brother_map;



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

/* This function returns a key to brother_map. */
pair<int, int> make_sorted_pair(int x, int y) {
  return make_pair(MIN(x, y), MAX(x, y));
}

/* This function is used to recursively orient half edge faces. */
void orient_face(HEF *f) {
  if (f == NULL || f->oriented)
    return;

  HEV *a = half_edge_vertices[f->a];
  HEV *b = half_edge_vertices[f->b];
  HEV *c = half_edge_vertices[f->c];

  HE *ab = brother_map[make_sorted_pair(f->a, f->b)];
  HE *bc = brother_map[make_sorted_pair(f->b, f->c)];
  HE *ac = brother_map[make_sorted_pair(f->a, f->c)];

  // If the edge for this face isn't in brother_map, it's the brother of the
  // edge in brother_map.
  if (ab->adj_face != f)
    ab = ab->brother;

  if (bc->adj_face != f)
    bc = bc->brother;

  if (ac->adj_face != f)
    ac = ac->brother;

  // 2 ways of orienting.
  bool clockwise;
  
  if (ab->brother != NULL && ab->brother->adj_face->oriented) {
    ab->vertex = ab->brother->next->next->vertex;
    if (ab->vertex == a) {
      clockwise = false;
    }
    // ab->vertex is b
    else {
      clockwise = true;
    }
  }

  if (ab->brother != NULL && bc->brother->adj_face->oriented) {
    bc->vertex = bc->brother->next->next->vertex;
    if (bc->vertex == b) {
      clockwise = false;
    }
    // bc->vertex is c
    else {
      clockwise = true;
    }
  }

  if (ac->brother != NULL && ac->brother->adj_face->oriented) {
    ac->vertex = ac->brother->next->next->vertex;
    if (ac->vertex == c) {
      clockwise = false;
    }
    // ac->vertex is a
    else {
      clockwise = true;
    }
  }
  
  // Checking to see if two edges point to the same vertex, which
  // results in a non-orientable surface.
  if ((ab->vertex != NULL && bc->vertex != NULL && ab->vertex == bc->vertex)
      || (bc->vertex != NULL && ac->vertex != NULL && bc->vertex == ac->vertex)
      || (ac->vertex != NULL && ab->vertex != NULL
          && ac->vertex == ab->vertex)) {
    cerr << "Non-orientable surface.";
    exit(1);
  }

  if (clockwise) {
    a->out = ab;
    b->out = bc;
    c->out = ac;

    ab->next = bc;
    bc->next = ac;
    ac->next = ab;

    ab->vertex = b;
    bc->vertex = c;
    ac->vertex = a;
  }

  else {
    a->out = ac;
    b->out = ab;
    c->out = bc;
    
    ab->next = ac;
    ac->next = bc;
    bc->next = ab;

    ab->vertex = a;
    ac->vertex = c;
    bc->vertex = b;
  }

  f->oriented = true;
  
  if (ab->brother != NULL) 
    orient_face(ab->brother->adj_face);

  if (bc->brother != NULL)
    orient_face(bc->brother->adj_face);

  if (ac->brother != NULL)
    orient_face(ac->brother->adj_face);
}

/* This function starts off the recursion of the above function, via an
 * arbitrarily chosen orientation. */
void orient_faces() {
  HEF *f = half_edge_faces[0];

  HEV *a = half_edge_vertices[f->a];
  HEV *b = half_edge_vertices[f->b];
  HEV *c = half_edge_vertices[f->c];

  HE *ab = brother_map[make_sorted_pair(f->a, f->b)];
  HE *bc = brother_map[make_sorted_pair(f->b, f->c)];
  HE *ac = brother_map[make_sorted_pair(f->a, f->c)];

  if (ab->adj_face != f)
    ab = ab->brother;

  if (bc->adj_face != f)
    bc = bc->brother;

  if (ac->adj_face != f)
    ac = ac->brother;

  ab->next = bc;
  bc->next = ac;
  ac->next = ab;

  ab->vertex = b;
  bc->vertex = c;
  ac->vertex = a;

  a->out = ab;
  b->out = bc;
  c->out = ac;

  f->oriented = true;
  
  if (ab->brother != NULL)
    orient_face(ab->brother->adj_face);

  if (bc->brother != NULL)
    orient_face(bc->brother->adj_face);

  if (ac->brother != NULL)
    orient_face(ac->brother->adj_face);
}

/* This function converts from struct point to struct HEV. */
HEV *point2hev(point *p) {
  HEV *ret = new HEV;

  ret->x = p->x;
  ret->y = p->y;
  ret->z = p->z;
  
  return ret;
}

/* This function creates the half_edge_vertices struct. */
void create_half_edge_vertices(vector<point*> * points) {
  for (vector<point*>::iterator it = points->begin();
       it != points->end();
       ++it) {
    HEV *v = point2hev(*it);
    half_edge_vertices.push_back(v);
  }
}

/* THis function creates the other half_edge structures. */
void create_half_edge_structures(vector<face*> *faces) {  
  for (vector<face*>::iterator it = faces->begin();
       it != faces->end();
       ++it) {
    int ind1 = (*it)->ind1 - 1;
    int ind2 = (*it)->ind2 - 1;
    int ind3 = (*it)->ind3 - 1;

    HEF *f = new HEF;
    half_edge_faces.push_back(f);

    f->a = ind1;
    f->b = ind2;
    f->c = ind3;

    HE *ab = new HE;
    half_edges.push_back(ab);

    pair<int, int> kab = make_sorted_pair(ind1, ind2);

    if (brother_map.find(kab)
        == brother_map.end()) {
      ab->brother = NULL;
      brother_map[kab] = ab;
    }

    else {
      HE *ab_brother = brother_map[kab];
      ab->brother = ab_brother;
      ab_brother->brother = ab;
    }

    HE *bc = new HE;
    half_edges.push_back(bc);

    pair<int, int> kbc = make_sorted_pair(ind2, ind3);

    if (brother_map.find(kbc)
        == brother_map.end()) {
      bc->brother = NULL;
      brother_map[kbc] = bc;
    }

    else {
      HE *bc_brother = brother_map[kbc];
      bc->brother = bc_brother;
      bc_brother->brother = bc;
    }

    HE *ac = new HE;
    half_edges.push_back(ac);
    
    pair<int, int> kac = make_sorted_pair(ind1, ind3);

    if (brother_map.find(kac)
        == brother_map.end()) {
      ac->brother = NULL;
      brother_map[kac] = ac;
    }
    else {
      HE *ac_brother = brother_map[kac];
      ac->brother = ac_brother;
      ac_brother->brother = ac;
    }

    f->adj = ab;
    f->oriented = false;

    ab->adj_face = f;
    bc->adj_face = f;
    ac->adj_face = f;
  }

  orient_faces();
}

/* Adds a random decimal amount to each tvec, then re-normalizes the vector
 * in order to bump map. */
void bump_map() {
  for (map<HEV*, Tvec*>::iterator it = tvecs.begin();
       it != tvecs.end();
       ++it) {
    Tvec *n = it->second;

    Vector3f v_n(n->x, n->y, n->z);
    
    v_n(0) += (float)(rand() % 100) / 100.0;
    v_n(1) += (float)(rand() % 100) / 100.0;
    v_n(2) += (float)(rand() % 100) / 100.0;

    v_n.normalize();

    n->x = v_n(0);
    n->y = v_n(1);
    n->z = v_n(2);
  }
}

/* Creates all of the normals for each vector. */
void create_tvecs() {
  for (vector<HEV*>::iterator it = half_edge_vertices.begin();
       it != half_edge_vertices.end();
       ++it) {
    Tvec *n = new Tvec;

    n->x = 0;
    n->y = 0;
    n->z = 0;

    tvecs[(*it)] = n;
  }
  for (vector<HEF*>::iterator it = half_edge_faces.begin();
       it != half_edge_faces.end();
       ++it) {
    HEV *v1 = half_edge_vertices[(*it)->a];
    HEV *v2 = half_edge_vertices[(*it)->b];
    HEV *v3 = half_edge_vertices[(*it)->c];

    Tvec *n1 = tvecs[v1];
    Tvec *n2 = tvecs[v2];
    Tvec *n3 = tvecs[v3];

    Vector3f v2_1(v2->x - v1->x, v2->y - v1->y, v2->z - v1->z);
    Vector3f v3_1(v3->x - v1->x, v3->y - v1->y, v3->z - v1->z);

    Vector3f cross = v2_1.cross(v3_1);
    
    float A = 0.5 * cross.norm();
    
    cross.normalize();

    Vector3f face_normal = A * cross;

    n1->x += face_normal(0);
    n1->y += face_normal(1);
    n1->z += face_normal(2);

    n2->x += face_normal(0);
    n2->y += face_normal(1);
    n2->z += face_normal(2);

    n3->x += face_normal(0);
    n3->y += face_normal(1);
    n3->z += face_normal(2);
  }

  bump_map();
}

//draws shaded objects. takes in iv file, writes in ppm format to Canvas.txt. 
int main(int argc, char ** argv)
{
  Mesh * containerP = new Mesh;
  printf("Parsing...\n");
  parse(containerP, argv[2]);
  printf("Finished parsing.\n");

  vector<point*> * points = containerP->points;
  vector<face*> * faces = containerP->faces;
        
  create_half_edge_vertices(points);
  create_half_edge_structures(faces);
  create_tvecs();

    
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
  init_textures(argv[1]);
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

    GLuint textureID;
    glGenTextures(1, &textureID);

    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_BGR,
                 GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    return textureID;
}

void init_textures(const char *imagepath)
{
    image = loadBMP_custom(imagepath);
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
    
    for (vector<HEF*>::iterator it = half_edge_faces.begin();
         it != half_edge_faces.end();
         ++it) {
      HEV *v1 = half_edge_vertices[(*it)->a];
      HEV *v2 = half_edge_vertices[(*it)->b];
      HEV *v3 = half_edge_vertices[(*it)->c];

      Tvec *n1 = tvecs[v1];
      Tvec *n2 = tvecs[v2];
      Tvec *n3 = tvecs[v3];

      glBegin(GL_TRIANGLES);  

      glTexCoord2d(0.0, 0.0);
      glNormal3f(n1->x, n1->y, n1->z);
      glVertex3f(v1->x, v1->y, v1->z);  
 
      glTexCoord2d(0.0, 1.0);
      glNormal3f(n2->x, n2->y, n2->z);
      glVertex3f(v2->x, v2->y, v2->z);  
 
      glTexCoord2d(1.0, 0.0);
      glNormal3f(n1->x, n1->y, n1->z);
      glVertex3f(v3->x, v3->y, v3->z);  
 
      glEnd(); 
    }

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

