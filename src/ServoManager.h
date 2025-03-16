#ifndef SERVOMANAGER_H
#define SERVOMANAGER_H

#include <Servo.h>

namespace ServoManager {
  extern Servo classificationServo;
  extern Servo lidServo;
  
  void attachServos();
  void setClassificationAngle(int angle);
  void operateLid();
}

#endif // SERVOMANAGER_H
