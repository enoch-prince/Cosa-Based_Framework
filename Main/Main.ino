#include "lib_common.h"
#include "message.h"
#include "control.h"

// Start time to force wrap-around; 16 ms aligned for watchdog
uint32_t START_TIME = UINT32_MAX - 16383UL;

enum class CustomMsgTypes : uint8_t {
  Ping,
  MotorStart,
  MotorStop,
  MotorDuty,
  MotorPWM
};

class DCMotorController : public epk::lib::controlInterface<CustomMsgTypes> {
  public:
    DCMotorController(uint8_t id=1) 
      : epk::lib::controlInterface<CustomMsgTypes>(id) {}

  protected:
    virtual void OnMessage(Nucleo::Actor*& client, epk::lib::message<CustomMsgTypes>& msg) {
      switch(msg.cmd) {
        case CustomMsgTypes::MotorStart: {
          start = (!start) ? true:start;
          msg.body = "Start Confirmed!";
        }
        break;
        
        case CustomMsgTypes::MotorStop: {
          start = (start) ? false:start;
          msg.body = "Stop Confirmed!";
        }
        break;
        
        case CustomMsgTypes::MotorDuty: {
          msg.body = String(value);
        }
        break;
        
        case CustomMsgTypes::MotorPWM: {
          msg.body = "nan";
        }
        break;

        default:
          break;
      }

      // send message to client thread
      SendMsg(client, msg);
    }
    
    virtual void RunAlgorithm() {
      if (start) {
        uint32_t u_now = Watchdog::millis();
        if (Watchdog::since(u_prev) > 1000UL) {
          value++;
          u_prev = u_now;
        }
      }
    }

  // Variables for your algorithm goes here
  private:
    uint16_t value = 0;
    uint32_t u_prev = START_TIME;
    bool start = false;
};


class MySerial : public epk::lib::serialInterface<CustomMsgTypes> {
  public:
    MySerial(Nucleo::Actor* controller, uint8_t id)
      : epk::lib::serialInterface<CustomMsgTypes>(controller, id) {}

    MySerial(Nucleo::Actor* controller, uint32_t baudrate, uint8_t id)
      : epk::lib::serialInterface<CustomMsgTypes>(controller, baudrate, id) {}

  protected:
    virtual int ProcessMsg(epk::lib::message<CustomMsgTypes>& msg) {
      int response;
      switch(msg.cmd) {
        case CustomMsgTypes::Ping: {
          msg.body = "old:" + msg.body + " | new: Ping received";
          // Send message via serial port
//          Nucleo::Mutex mux(epk::lib::semaphore); 
//          SendMsg(msg);
          response = 0;  
        }
        break;

        default:
          response = int(msg.cmd);
          break;
      }
      return response;
    }
};

DCMotorController controller;
MySerial serial(&controller, 2);


void setup() {
  serial.Begin(); // default baudrate is 9600
  
  Watchdog::begin(); // Start watchdog timer as clock
  //Watchdog::millis(START_TIME); // Set millis secs clock

  // start the nucleo actors
  Nucleo::Thread::begin(&serial, 256);
  Nucleo::Thread::begin(&controller, 256);

  // start the main thread
  Nucleo::Thread::begin();

  START_TIME = Watchdog::millis();
}

void loop() {
  Nucleo::Thread::service();
}
