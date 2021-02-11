#ifndef LIB_COMMON_H
#define LIB_COMMON_H

#define USE_SYNC_DELAY

#include <Nucleo.h>
#include "Cosa/String.hh"
#include "Cosa/Watchdog.hh"
#include "Cosa/Trace.hh"
#include "Cosa/UART.hh"

namespace epk {
  namespace lib {

    Nucleo::Semaphore semaphore;
    bool IncomingMessage = false;
  
    template<typename DataType>
    class vector {
        size_t d_size; // Stores no. of actually stored objects
        size_t d_capacity; // Stores allocated capacity
        DataType *d_data; // Stores data
      
      public:
        vector() : d_size(0), d_capacity(0), d_data(0) {} // Default constructor
        
        vector(vector const &other) : d_size(other.d_size), d_capacity(other.d_capacity), d_data(0) {
          d_data = (DataType *)malloc(d_capacity * sizeof(DataType));
          //d_data = new DataType[d_capacity * sizeof(DataType)];
          memcpy(d_data, other.d_data, d_size * sizeof(DataType));
        } // Copy constuctor
        
        ~vector() {
          //delete[] d_data;
          free(d_data);
        } // Destructor
        
        vector &operator=(vector const &other) {
          free(d_data);
          //delete[] d_data;
          d_size = other.d_size;
          d_capacity = other.d_capacity;
          d_data = (DataType *)malloc(d_capacity * sizeof(DataType));
          //d_data = new DataType[d_capacity * sizeof(DataType)];
          memcpy(d_data, other.d_data, d_size * sizeof(DataType));
          return *this;
        } // Needed for memory management

        
        void push_back(DataType const &x) {
          if (d_capacity == d_size) resize();
          //memcpy(d_data[d_size++], (DataType)x, sizeof(DataType));
          d_data[d_size++] = x;
        } // Adds new value. If needed, allocates more space

        DataType pop_back() {
          DataType d = d_data[d_size-1];
//          DataType d;
//          memcpy(&d, d_data[d_size-1], sizeof(DataType));
          resize(--d_size);
          return d;
        }
        
        size_t size() const {
          return d_size;
        } // Size getter

        DataType* data() {
          return d_data;
        }
        
        DataType const &operator[](size_t idx) const {
          return d_data[idx];
        } // Const getter
        
        DataType &operator[](size_t idx) {
          return d_data[idx];
        } // Changeable getter

        friend IOStream& operator << (IOStream& ios, const vector& vec) {
          for(size_t i = 0; i < vec.size(); i++){
            ios << vec[i];
          }
          return ios;
        }

        void resize(size_t sz) {
          d_capacity = sz;
           DataType *newdata = (DataType *)malloc(d_capacity * sizeof(DataType));
          memcpy(newdata, d_data, d_size * sizeof(DataType));
          free(d_data);
          //delete[] d_data;
          d_data = newdata;
        }

        bool empty() const {
          return d_size == 0;
        }

        void clear(){
          d_capacity = 0;
          d_size = 0;
          d_data = 0;
        }
      
      private:
        void resize() {
          //d_capacity = d_capacity ? d_capacity * 2 : 1;
          d_capacity++; 
          DataType *newdata = (DataType *)malloc(d_capacity * sizeof(DataType));
          //DataType* newdata = new DataType[d_capacity * sizeof(DataType)];
          memcpy(newdata, d_data, d_size * sizeof(DataType));
          free(d_data);
          //delete[] d_data;
          d_data = newdata;
        }
    };
  }
}

#endif
