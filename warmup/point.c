#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "common.h"
#include "point.h"

void
point_translate(struct point *p, double x, double y)
{
  p->x = p->x+x;
  p->y = p->y+y;
   
}

double
point_distance(const struct point *p1, const struct point *p2)
{
  return (sqrt(pow(p1->x - p2->x,2) + pow(p1->y - p2->y,2)));
}

int
point_compare(const struct point *p1, const struct point *p2)
{
  if ((pow(p1->x,2)+pow(p1->y,2))>(pow(p2->x,2)+pow(p2->y,2))){
     return 1;
  }
  else if ((pow(p1->x,2)+pow(p1->y,2))==(pow(p2->x,2)+pow(p2->y,2))){
     return 0;
  }
  else{
     return -1;
  }


}
