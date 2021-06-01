
struct settaggi {
      int TouchXoffset;
      int TouchYoffset;
      int TouchXmax;
      int TouchYmax;
};
extern struct settaggi Settings ;

extern int TouchX;
extern int TouchY;
extern int KeyPress;
extern int KeyPressP;
extern int TouchXg,TouchYg;


void TouchInit(void);
void keyread(void);
void TouchSetngsUpdate(int ymin, int ymax, int xmin, int xmax);
void TouchSetngsSave(void);

