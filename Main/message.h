#ifndef MESSAGE_H
#define MESSAGE_H

#include "lib_common.h"


namespace epk {
namespace lib {

template<typename T>
struct message
{
  T cmd{};
  char seperator = ':';
  String body;

  // returns the entire size of the message packet
  size_t size() const {
    return sizeof(cmd) + body.length();
  }

  bool isempty() const {
    return (size() == 0);
  }

  void clear() {
    cmd = {};
    body = "";
  }


  // Override for Cosa Trace cout operator
  friend IOStream& operator << (IOStream& ios, message<T>& msg) {
    if (msg.body.length() > 0)
      ios << int(msg.cmd) << msg.seperator << msg.body << '\n';
    else
      ios << int(msg.cmd) << '\n';
    return ios;
  }

  // Push data into the Message Object directly from the serial
  friend IOStream::Device& operator >> (IOStream::Device& ios, message<T>& msg) {
    char buff[16];
    String data = ios.gets(buff, sizeof(buff));
    data.trim();
    int sepIndex = data.indexOf(msg.seperator);
    if (sepIndex >= 0) {
      String s_cmd = data.substring(0, sepIndex);
      msg.cmd = static_cast<T>(s_cmd.toInt());
      msg.body = data.substring(sepIndex + 1);
    }
    else {
      msg.cmd = static_cast<T>(data.toInt());
      msg.body = "";
    }
    return ios;
  }

  // Push data into the plain datatypes directly from the serial
  template<typename DataType>
  friend IOStream::Device& operator >> (IOStream::Device& ios, DataType& data) {
    char buff[16];
    data = DataType(ios.gets(buff, sizeof(buff)));
    return ios;
  }

  // Alternative to >> operator
  char* readFromStream(IOStream::Device& ios) {
    char buff[16];
    return ios.gets(buff, sizeof(buff));
  }

};



/***********************************************************************/
/***************** Customizable Serial Interface Class *****************/
/***********************************************************************/
template<typename T>
class serialInterface : public Nucleo::Actor {

  public:
    serialInterface(Actor* ctrller, uint8_t id) 
      : Actor(), m_controller(ctrller), u_baudrate(0), u_id(id) {}
    
    serialInterface(Actor* ctrller, uint32_t baudrate, uint8_t id) 
      : Actor(), m_controller(ctrller), u_baudrate(baudrate), u_id(id) {}
    
    ~serialInterface() {}

    void Begin() {
      uart.blocking();

      if (u_baudrate)
        uart.begin(u_baudrate);
      else
        uart.begin();

      trace.begin(&uart, PSTR("[Serial Comm.] started!"));
    }

    void Wait() {
      while (!uart.available()) {}
    }

    bool Available() {
      return uart.available();
    }


    String sReadMsg() {
      message<T> m_dataIn;
      //char* msg = m_dataIn.readFromStream(uart);
      uart >> m_dataIn.body;
      return m_dataIn.body;
    }

    message<T> ReadMsg() {
      message<T> m_dataIn;
      uart >> m_dataIn;
      return m_dataIn;
    }

    template<typename DataType>
    void ReadMsg(vector<DataType>& data) {

    }

    // Transmit message directly to through the serial port
    void SendMsg(message<T>& m_dataIn) {
      trace << m_dataIn;
    }
    
    // Transmit message directly to through the serial port
    template<typename DataType>
    void SendMsg(T cmd, DataType dt = nullptr) {
      message<T> m_dataOut;
      // push data into the message object
      m_dataOut.cmd = cmd;
      if (dt != nullptr)
        m_dataOut.body = String(dt);

      // transmit data via trace
      trace << m_dataOut;
    }

    template<typename DataType>
    void SendMsg(T cmd, vector<DataType> vec) {
      message<T> m_dataOut;
      // push vector data into the message object
      m_dataOut.cmd = cmd;

      for (size_t i = 0; i < vec.size(); i++) {
        m_dataOut.body = vec[i];

        // transmit data via trace
        trace << m_dataOut;
      }
    }

  // Can be overrriden bu user
  public:
    // Called by the Actor class 
    virtual void run() {
      { // Mutually exclusive traces
        Nucleo::Mutex mux(semaphore);
        trace << PSTR("serialInterface:running") << endl; 
      }
      while (1) {
        // Check, process and send data from the serial port
        
        if (Available()) {
          
          // Read message from the serialport
          message<T> msg = ReadMsg();

          // Process the message
          int response = ProcessMsg(msg);

          Nucleo::Mutex mux(semaphore);
            trace << PSTR("Thread:") << u_id << PSTR(">> ")<< msg;

          delay(200);
          
          // Pass the message to the appropriate receive (thread) if meant for it
          if (response > 0) {
            mutex(semaphore) {
              IncomingMessage = true;
              m_controller->send(u_id, &msg, sizeof(msg));
            }
          }
        }
        

        // get incoming data from other threads and send via serial
        else if (IncomingMessage) {
          uint8_t port = 0;
          message<T> msg = {};

          // receive message from thread, returns size or error code
          int resp; 
          mutex(semaphore) {
            IncomingMessage = false;
            resp = recv(m_controller, port, &msg, sizeof(msg));
          }
          if (resp > 0) {
            // send message via serial (usually data from the microcontroller)
            Nucleo::Mutex mux(semaphore); 
            SendMsg(msg);
          }
        }
      }
    }


  // Meant to be overrided by user
  protected:
    // Returns 0 if processed message is not meant for other thread workers
    virtual int ProcessMsg(message<T>& msg) {
      SendMsg(msg);
      return 0;
    }

  private:
    Actor* m_controller;
    uint32_t u_baudrate;
    uint8_t u_id;
};

}
}

#endif
