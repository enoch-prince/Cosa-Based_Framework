#ifndef CONTROL_H
#define CONTROL_H

#include "Cosa/Math.hh"
#include "lib_common.h"

namespace epk {
  namespace lib {

    template<typename T>
    class controlInterface : public Nucleo::Actor {
      public:
        controlInterface (uint8_t id) : Actor(), u_id(id) {}
        ~controlInterface () {}

      public:
        virtual void run() {
          { // Mutually exclusive traces
            Nucleo::Mutex mux(semaphore);
            trace << PSTR("controlInterface:running") << endl; 
          }
          while(1) {
            if (IncomingMessage) ReceiveMsg();
            RunAlgorithm();
          }
        }

        // Send message back to other threads. Called inside the OnMessage method
        void SendMsg(Actor*& client, message<T>& msg){
          mutex(semaphore) {
            IncomingMessage = true;
            client->send(u_id, &msg, sizeof(msg));
          }
        }

        void ReceiveMsg(){
          Actor* sender = NULL;
          uint8_t id = 0;
          message<T> msg = {};

          int response; 
          mutex(semaphore){
            IncomingMessage = false;
           response = recv(sender, id, &msg, sizeof(msg));
          }
          Nucleo::Mutex mux(semaphore);
            trace << PSTR("Thread:") << u_id << PSTR(">> ") << msg;
          
          delay(200);
          
          if (response > 0)
            OnMessage(sender, msg);
        }

      // To be overrided by user
      protected:
        virtual void OnMessage(Actor*& client, message<T>& msg) {}
        virtual void RunAlgorithm() {}

      private:
        uint8_t u_id;
    };
    
  }
}


#endif
