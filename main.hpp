#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <list>
#include <vector>
#include <math.h>
#include <limits>
#include <string>
#include <cstring>
#include <fstream>

// half edge structure
typedef struct HE {
    struct HE * brother; // the HE in the opp dir.
    struct HEF * adj_face; // the face
    struct HE * next; // the HE that is 'out' of 'vertex'
    struct HEV * vertex; // the vertex this HE points to
} HE;

typedef struct HEF {
  int a, b, c; // useful for helping the initial loading process.
  struct HE * adj; // one of the three HEs that surround a face
  bool oriented; // also useful for the loading. marks if this face was oriented or not.
} HEF;

typedef struct HEV {
    float x;
    float y;
    float z;
    struct HE * out; // one of the two HEs whose brother points to this vertex
} HEV;

typedef struct Tvec {
    float x,y,z;

} 
Tvec;
