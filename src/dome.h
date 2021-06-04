extern int EncoderPosition;
extern int QuotaParcheggio;
extern int DomePosition;
extern int DomeMotion;
extern int DomeManMotion;
extern int TelescopePosition;
extern int TelescopeAlt;
extern int DomeParking;
extern int DomeOneRotPulses;       //quanti impulsi encoder per giro di cupola
extern int DomeShiftZero;

#define Rollover 0
#define Absolute 1
#define Tracking 2

#define Cw  1
#define Ccw 0

void DomeInit(void);
void DomeParkSave(void);
void DomeMain(void);
int DomeMoveStart(int target, int type);
void DomeMoveStop(void);
void DomePark(void);
int MotionStart( int Dist2Go );
void DomeManStart(int dir);
void DomeManStop(void);

namespace Dome {
    namespace MovementDirection {
        const int Clockwise = Cw;
        const int CounterClockwise = Ccw;
    };
};