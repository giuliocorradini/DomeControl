/*
 *  dome.h
 *  Routine di controllo del movimento della cupola (motore e encoder)
 */

#pragma once

#include "mbed.h"
#include "historic.h"

extern int EncoderPosition;
extern int QuotaParcheggio;
extern Historic<int> DomePosition;
extern int DomeMotion;
extern int DomeManMotion;
extern Historic<int> TelescopePosition;
extern int TelescopeAlt;
extern int DomeParking;
extern int DomeOneRotPulses;       //quanti impulsi encoder per giro di cupola
extern int DomeShiftZero;
extern int StopRampPulses;
extern bool CalibratingSlope;

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
void DomeStartSlopeCalibration();
void DomeResetSlopeCalibration();

const int TELESCOPE_POSITION_DELTA_THRESHOLD = 5;
void TelescopePositionUpdate(int new_position);

namespace Dome {
    namespace MovementDirection {
        const int Clockwise = Cw;
        const int CounterClockwise = Ccw;
    };

    namespace API {
        enum Command {CENTER, TRACK, NO_TRACK, STOP};
        extern Mail<Command, 10> command_queue;
        extern Mutex tele_pos_mutex;
    }
};

//Call this macro or lock Dome::API::tele_pos_mutex when working with
//TelescopePosition and TelescopeAlt variables
#define SAFE_TELESCOPE_UPDATE(x)    Dome::API::tele_pos_mutex.lock(); \
                                    x; \
                                    Dome::API::tele_pos_mutex.unlock();
