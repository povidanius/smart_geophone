#include <ADS1115_WE.h>
#include <Wire.h>
#include <ArduinoQueue.h> // https://github.com/EinarArnason/ArduinoQueue
#include <SPI.h>
#include <SdFat.h>

#define I2C_ADDRESS 0x48
#define QUEUE_SIZE  64
#define TRIGGER_DELAY 0.1

ADS1115_WE adc = ADS1115_WE(I2C_ADDRESS);
ArduinoQueue<float> readingQueue(QUEUE_SIZE); // approx one second.
float trigger_voltage_mV = 30; // will save data if exceeded
bool trigger_activated;

//#define OPERATION_MODE_PLOT 1 
#define OPERATION_MODE_CONTINUOUS_ACQUISITION 1
//#define OPERATION_MODE_EVENT_TRIGGER 1


/*
 *  Geophone docs: http://cdn.sparkfun.com/datasheets/Sensors/Accelerometers/SM-24%20Brochure.pdf
 * 
 */

unsigned long int frame_counter;
unsigned long trigger_time;
SdFat sd;
SdFile logFileCA;
const int chipSelect = 10;


void setup() {
  
  Wire.begin();
  Serial.begin(9600);
  Serial.println("Smart seismic sensor");
  delay(100);
  
  if(!adc.init()){
    Serial.println("ADS1115 not connected!");
  }
  else
  {
    Serial.println("ADS115 connected");
  }
  
  delay(100);
  
  if (!sd.begin(chipSelect, SPI_HALF_SPEED)){
    sd.initErrorHalt();
  }
  
  
  adc.setVoltageRange_mV(ADS1115_RANGE_6144); //comment line/change parameter to change range 
  //adc.setVoltageRange_mV(ADS1115_RANGE_4096); //comment line/change parameter to change range
  //adc.setVoltageRange_mV(ADS1115_RANGE_1024);
  //adc.setVoltageRange_mV(ADS1115_RANGE_0512);
  frame_counter = 0;
  trigger_activated = false;  

  #ifdef OPERATION_MODE_CONTINUOUS_ACQUISITION
  #endif
}

void loop() {
  float voltage = 0.0;
  voltage = readChannel(ADS1115_COMP_0_1); 

  #ifdef OPERATION_MODE_PLOT
   Serial.print("0: ");
   Serial.println(voltage);
  #endif

  readingQueue.enqueue(voltage);

  #ifdef OPERATION_MODE_CONTINUOUS_ACQUISITION   
   if (readingQueue.isFull())
   {
    //SdFile logFileCA;
    //logFileCA.open("LOG.TXT",  O_RDWR | O_CREAT | O_AT_END); 
    
    logFileCA.open("LOG.TXT",   O_CREAT | O_APPEND | O_WRITE); 
    while (!readingQueue.isEmpty())
      {
        float x = readingQueue.dequeue();   
        String data_string = String(x);
        logFileCA.println(data_string);      
      }
   logFileCA.close();
   }
  #endif
  

  #ifdef OPERATION_MODE_EVENT_TRIGGER
  
  if (fabs(voltage) > trigger_voltage_mV && trigger_activated == false)
  {
    trigger_activated = true;
    trigger_time = millis();
    Serial.println("Trigger activated");
  }  

  if (trigger_activated && millis() - trigger_time > 100 && readingQueue.isFull()) {
    trigger_activated = false;
    saveData();   
  }
  #endif
  
  frame_counter++;
  if (frame_counter % 1000 == 0)
  {
   // Serial.println((float) 1000* frame_counter/millis());
  }  
  
}

float readChannel(ADS1115_MUX channel) {
  float voltage = 0.0;
  adc.setCompareChannels(channel);
  adc.startSingleMeasurement();
  while(adc.isBusy()){}
  voltage = adc.getResult_mV(); // alternative: getResult_mV for Millivolt
  return voltage;
}

// save queue to SD card
void saveData() {
  char file_name[16];
  int idx = getAvailableFileNameIdx();
  SdFile logFile;


  sprintf(file_name, "LOG%d.TXT", idx);    
  Serial.println("Saving data to");
  Serial.println(file_name);
  
  String data_string = "";
  if (logFile.open(file_name,  O_RDWR | O_CREAT | O_AT_END))
  {  
    int num_pt = 0;
    while (!readingQueue.isEmpty())
    {
      float x = readingQueue.dequeue();   
      data_string = String(x);
      logFile.println(data_string);
      num_pt++;
    }
   logFile.close();  
   Serial.println("Data written");
   Serial.println(file_name); 
   Serial.println(num_pt);
  }
  else
  {
   Serial.println("Cant open file for writing");
   Serial.println(file_name); 
   Serial.println(logFile);
  }  

  //delay(100);
  
}

int getAvailableFileNameIdx()
{ 
 char file_name[16];
 int idx = 0;

 while (1)
 {
   sprintf(file_name, "LOG%d.TXT", idx);
   if (!sd.exists(file_name)) return idx;
   idx++;
 } 
 
}
