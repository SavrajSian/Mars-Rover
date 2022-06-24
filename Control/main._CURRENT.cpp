#include <Arduino.h>

#include <Arduino_JSON.h>

#include <SPI.h>
#include <Wire.h>

#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <stdlib.h>

#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// TO DO: add in buffer for reading instruction
// TO DO 2: read correct values to database for direction

//_______________________________________________HEADERS FOR DRIVE MODULE_____________________________________________

#include "headers/motor_control.h"
#include "headers/ADNS3080.h"

//_______________________________________________VARIABLE DELCARATION FOR CONTROL MODULE___________________________________________________________________________

const char* ssid = "LaptopWiFi";
const char* password = "summerProject1234";

//Your Domain name with URL path or IP address with path
String serverName = "http://146.169.197.133/get_rover_data.php";
const char* serverNameOne = "http://146.169.197.133/post_rover_data.php"; //wireless lan adapter wifi IPV4 address
 
String apiKey = "tPmAT5Ab3j7F9";

//Adafruit_BME280 bme;
int x_direction, y_direction, battery_life, angle;
String alien_present, infrastructure_present, colour, building_present;

int distance, degrees;

//____________________________________________VARIABLE DELCARATION FOR DRIVE MODULE______________________________________________________________
//_____________________________________________OPTICAL_FLOW____________________________________________________

float total_x = 0;
float total_y = 0;
float total = 0;


float total_x1 = 0;
float total_y1 = 0;


int x=0;
int y=0;

int a=0;
int b=0;

float distance_x=0;
float distance_y=0;

volatile byte movementflag=0;
volatile int xydat[2];

int tdistance = 0;

int counter =0;
float total_dy= 0;
float average_dy = 0;

byte frame[ADNS3080_PIXELS_X * ADNS3080_PIXELS_Y];

float m_dtheta = 0;
float m_dtheta_total = 0;
float m_theta = 0;
float r_theta = 0;


// ________________________________________________________CONTROL MODULE _________________________________________________________

void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin("LaptopWiFi", "summerProject1234");
  Serial.print("Connecting to WiFi ..");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

//_________________________________________________GENERATING RANDOM DATA____________________________________________________________
int generate_x(){
  int x;
  x = random(0,500);
  return x;
}

int generate_y(){
  int x;
  x = random(0,500);
  return x;
}

int generate_angle(){
  int x;
  x = random(0,360);
  return x;
}

int generate_battery(){
  int x;
  x = random(0,100);
  return x;
}

String generate_alien(){
  int x;
  String alien;
  x = random(0,100);
  if (x<50){
    alien = "yes";
  }
  else{
    alien = "no";
  }
  return alien;
}

String generate_colour(){
  int x;
  String alien;
  x = random(0,100);
  if (x<25){
    alien = "red";
  }
  else if (26<x && x<50){
    alien = "yellow";
  }

  else if (51<x && x<75){
    alien = "green";
  }

  else{
    alien = "blue";
  }
  return alien;
}

String generate_infrastructure(){
  int x;
  String alien;
  x = random(0,100);
  if (x<50){
    alien = "yes";
  }
  else{
    alien = "no";
  }
  return alien;
}

String generate_building_present(){
  int x;
  String alien;
  x = random(0,100);
  if (x<50){
    alien = "yes";
  }
  else{
    alien = "no";
  }
  return alien;
}

void setup() {
//_____________________________________________CONTROL MODULE_______________________________________________________
  
  Serial.begin(115200);
  initWiFi();
  //SPI.begin();

//_____________________________________________DRIVE_MODULE_______________________________________________________
    
    //_________________________________________MOTOR_CONTROL______________________________________________________
    
    int a = 0;
    robot.begin();

    //_______________________________________OPTICAL_FLOW_____________________________________________________________

      //L298N DC Motor by Robojax.com
      pinMode(PIN_SS,OUTPUT);
      pinMode(PIN_MISO,INPUT);
      pinMode(PIN_MOSI,OUTPUT);
      pinMode(PIN_SCK,OUTPUT);

      SPI.begin();
      SPI.setClockDivider(SPI_CLOCK_DIV32);
      SPI.setDataMode(SPI_MODE3);
      SPI.setBitOrder(MSBFIRST);

      if(mousecam_init()==-1){
        Serial.println("Mouse cam failed to init");
        while(1);
      }
    
}

void loop() {


  //________________________________________________GETTING DATA_________________________________________________________________________
  

  // the following variables are unsigned longs because the time, measured in
  // milliseconds, will quickly become a bigger number than can be stored in an int.
  unsigned long lastTime = 0;

  // Set timer to 5 seconds (5000)
  unsigned long timerDelay = 1000;

  //Send an HTTP POST request every 10 minutes
  if ((millis() - lastTime) > timerDelay) {

    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){

        HTTPClient http;
        Serial.print('\n');
        Serial.println("GET DATA");
        // Your Domain name with URL path or IP address with path
        http.begin(serverName);
        
        // Send HTTP GET request
        int httpResponseCode = http.GET();
        String payload;
        float manual_control_arr[2]; 

        if (httpResponseCode>0) {
          
          Serial.print("HTTP Response code: ");
          Serial.println(httpResponseCode);
            
          payload = http.getString();
          Serial.print("Payload: ");
          Serial.println(payload);
            
          Serial.println(payload.substring(0,4));
          Serial.println(payload.substring(4,9));

          distance = payload.substring(0,4).toInt();
          degrees = payload.substring(4,9).toInt();

          Serial.print("distance: ");
          Serial.println(distance);
          Serial.print("degrees: ");
          Serial.print(degrees);

        }
          
        else {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
        }

       // Free resources
       http.end();
    }

    else {
      Serial.println("WiFi Disconnected");
    }
 
  lastTime = millis();

  }


  //____________________________________________________DRIVE MODULE____________________________________________________


    // if enabled this section produces a bar graph of the surface quality that can be used to focus the camera
    // also drawn is the average pixel value 0-63 and the shutter speed and the motion dx,dy.

    int val = mousecam_read_reg(ADNS3080_PIXEL_SUM);
    MD md;
    md.x  = 0;
    md.y = 0;

    mousecam_read_motion(&md);

    bool out_of_range = false;
    if(movementflag){

    tdistance = tdistance + convTwosComp(xydat[0]);
    Serial.println("Distance = " + String(tdistance));
    movementflag=0;
    delay(1);
    }

    for(int i=0; i<md.squal/4; i++) {
      Serial.print('*');
    }

    Serial.print(' ');
    Serial.print((val*100)/351);
    Serial.print(' ');
    Serial.print(md.shutter); Serial.print(" (");
    Serial.print((int)md.dx); Serial.print(',');
    Serial.print((int)md.dy); Serial.println(')');

    // Serial.println(md.max_pix);
    delay(1);

    distance_x = convTwosComp(md.dx);
    distance_y = convTwosComp(md.dy);

    total_x1 = total_x1 + distance_x;
    total_y1 = total_y1 + distance_y;
    md.x = md.x + 0.2*(float)md.dx;
    total_x = 0.2*total_x1;
    total_y = 0.2*total_y1;

    Serial.print("md.dx: ");    Serial.println(md.dx);

    total_x = total_x1/157;
    total_y = total_y1/157;

    m_dtheta = (((0.2*md.dx)/125)/3.141592653)*180;
    m_dtheta_total = m_dtheta_total + m_dtheta;
    m_theta = (((0.2*md.x)/125)/3.141592653)*180;
    Serial.println("Difference: "+String(m_dtheta_total-m_theta));


    Serial.print('\n');


    Serial.println("Distance_x = " + String((float)((total_x))));
    Serial.println("Distance_y = " + String((float)((total_y))));
    Serial.print('\n');


  //___________________________________________MANUAL CONTROL____________________________________________
  //___________________________________________STRAIGHT LINE_____________________________________________
           
                    
   
    int left_speed = 0;

    int right_speed = 0;

    int direction = CW; //CW is forward

    if (distance < 0) {
      direction = CCW;
      left_speed = 50;
      right_speed = 50;
    }

    if (distance > 0) {
      direction = CW;
      left_speed = 50;
      right_speed = 50;
    }


    if (distance == 0){
      left_speed = 0;
      right_speed = 0;
    }

  float real_x = (float)(total_x);
  Serial.println("real_x = " + String((float)((real_x))));
  Serial.println("md.dy = " + String((float)((md.dy))));
  
   if (md.y < distance) { //forwards 
    
    if (real_x>0)
    {
      real_x = sqrt(real_x*real_x);
      Serial.println("Left motor Speed: "+ String(left_speed+10*(+real_x*real_x)));
      Serial.println("Right motor Speed: "+ String(right_speed+10*(-real_x*real_x)));
      robot.rotate(left_motor, left_speed+10*(+real_x*real_x), direction);
      robot.rotate(right_motor, right_speed +10*(-real_x*real_x), direction);//run right_motor at 60% speed in CW direction
    }
   
    if (real_x<0)
    {
      real_x = sqrt(real_x*real_x);
      Serial.println("Right motor Speed: "+ String(left_speed+10*(+real_x*real_x)));
      Serial.println("Left motor Speed: "+ String(right_speed+10*(-real_x*real_x)));
      robot.rotate(right_motor, left_speed+10*(+real_x*real_x), direction);
      robot.rotate(left_motor, right_speed +10*(-real_x*real_x), direction);//run right_motor at 60% speed in CW direction
    }

    if(real_x == 0){
      robot.rotate(right_motor, left_speed, direction);//run right_motor at 60% speed in CW direction
      robot.rotate(left_motor, right_speed, direction);//run right_motor at 60% speed in CW direction
    }
    
    if (md.y > 0)
    {
      counter = counter + 1;
      if(counter >= 6){
        total_dy = total_dy + md.dy;
        average_dy = total_dy / counter;
        if (average_dy + 10 > md.dy && average_dy - 10 < md.dy)
        {
          md.y = md.y + 0.2*(float)md.dy;
          out_of_range = true;
        }
        else{
          md.y = md.y + 0.2*average_dy;
        }
      }
    }



    Serial.println("Counter: "+String(counter));
    Serial.println("Average: "+String(average_dy));
    Serial.println("total y: "+String(total_dy));
    if(out_of_range)
    {
      Serial.println("md.dx OUT OF RANGE. Subsituted "+String(average_dy));
    }
    Serial.println("Distance: total md.y " + String(md.y));

  }

  else if (md.y > distance) { //backwards
     
     if (real_x>0)
    {
      real_x = sqrt(real_x*real_x);
      Serial.println("Left motor Speed: "+ String(left_speed+10*(+real_x*real_x)));
      Serial.println("Right motor Speed: "+ String(right_speed+10*(-real_x*real_x)));
      robot.rotate(left_motor, left_speed+10*(+real_x*real_x), direction);
      robot.rotate(right_motor, right_speed +10*(-real_x*real_x), direction);//run right_motor at 60% speed in CW direction
    }
   
    if (real_x<0)
    {
      real_x = sqrt(real_x*real_x);
      Serial.println("Right motor Speed: "+ String(left_speed+10*(+real_x*real_x)));
      Serial.println("Left motor Speed: "+ String(right_speed+10*(-real_x*real_x)));
      robot.rotate(right_motor, left_speed+10*(+real_x*real_x), direction);
      robot.rotate(left_motor, right_speed +10*(-real_x*real_x), direction);//run right_motor at 60% speed in CW direction
    }

    if(real_x == 0){
      robot.rotate(right_motor, left_speed, direction);//run right_motor at 60% speed in CW direction
      robot.rotate(left_motor, right_speed, direction);//run right_motor at 60% speed in CW direction
    }
    
    if (md.y > 0)
    {
      counter = counter + 1;
      if(counter >= 6){
        total_dy = total_dy + md.dy;
        average_dy = total_dy / counter;
        if (average_dy + 10 > md.dy && average_dy - 10 < md.dy)
        {
          md.y = md.y + 0.2*(float)md.dy;
          out_of_range = true;
        }
        else{
          md.y = md.y + 0.2*average_dy;
        }
      }
    }

    Serial.println("Counter: "+String(counter));
    Serial.println("Average: "+String(average_dy));
    Serial.println("total y: "+String(total_dy));
    if(out_of_range)
    {
      Serial.println("md.dx OUT OF RANGE. Subsituted "+String(average_dy));
    }
    Serial.println("Distance: total md.y " + String(md.y));
  }

  else{
  robot.brake(1);
  robot.brake(2);  

  bool lineDone = true; 
  Serial.print ("lineDone: "); Serial.println(lineDone);
  }

delay(1000);
 
//________________________________________________ROTATE _____________________________________________________________________________


r_theta = degrees;
m_dtheta_total = 0; 

while((int)r_theta > (int)m_dtheta_total){
    for(int i=0; i<md.squal/4; i++)
      Serial.print('*');
    Serial.print(' ');
    Serial.print((val*100)/351);
    Serial.print(' ');
    Serial.print(md.shutter); Serial.print(" (");
    Serial.print((int)md.dx); Serial.print(',');
    Serial.print((int)md.dy); Serial.println(')');
   
  
    distance_x = convTwosComp(md.dx);
    distance_y = convTwosComp(md.dy);
    float speed = (r_theta- m_dtheta_total)*(50/r_theta);
    Serial.println("Speed %: "+String(speed));
    robot.rotate(right_motor,(float)speed, CCW);//run right_motor at 60% speed in CW direction
    robot.rotate(left_motor, (float)speed, CW);//run right_motor at 60% speed in CW direction
    counter = counter + 1;
      
    mousecam_read_motion(&md);
	  total_x1 = total_x1 + distance_x;
  	total_y1 = total_y1 + distance_y;
  	float real_x =(float)total_x;
  	md.x = md.x + 0.2*(float)md.dx;

  	m_dtheta = (((0.2*md.dx)/125)/3.141592653)*180;
  	m_dtheta_total = m_dtheta_total + m_dtheta;
    m_theta = ((md.x)/125)*(180/3.141592653);
    Serial.println("Angle: "+String(m_dtheta_total));
   // Serial.println("m_dtheta Angle: "+String(m_dtheta));
   //Serial.println("m_dtheta_total: "+String(m_dtheta_total));
  	Serial.println("md.x: "+String(md.x));
  	Serial.println("md.dx: "+String(md.dx)); 
    
  }

  while((int)r_theta < (int)m_dtheta_total){
    for(int i=0; i<md.squal/4; i++)
      Serial.print('*');
    Serial.print(' ');
    Serial.print((val*100)/351);
    Serial.print(' ');
    Serial.print(md.shutter); Serial.print(" (");
    Serial.print((int)md.dx); Serial.print(',');
    Serial.print((int)md.dy); Serial.println(')');
   
  
    distance_x = convTwosComp(md.dx);
    distance_y = convTwosComp(md.dy);
    float speed = (r_theta-m_dtheta_total)*(50/r_theta);
    Serial.println("Speed %: "+String(speed));
    robot.rotate(right_motor,(float)speed, CW);//run right_motor at 60% speed in CW direction
    robot.rotate(left_motor, (float)speed, CCW);//run right_motor at 60% speed in CW direction
    counter = counter + 1;
      
    mousecam_read_motion(&md);
	  total_x1 = total_x1 + distance_x;
  	total_y1 = total_y1 + distance_y;
  	float real_x =(float)total_x;
  	md.x = md.x + 0.2*(float)md.dx;

  	m_dtheta = (((0.2*md.dx)/125)/3.141592653)*180;
  	m_dtheta_total = m_dtheta_total + m_dtheta;
  	m_theta = ((md.x)/125)*(180/3.141592653);
    Serial.println("Angle: "+String(m_dtheta_total));
   // Serial.println("m_dtheta Angle: "+String(m_dtheta));
  //  Serial.println("m_dtheta_total: "+String(m_dtheta_total));
  	Serial.println("md.x: "+String(md.x));
  	Serial.println("md.dx: "+String(md.dx)); 
    
  }

//____________________________________________CONTROL_MODULE__________________________________________________________________________
   
   unsigned long previousMillis = 0;
   unsigned long interval = 1000;
   unsigned long currentMillis = millis();
    
  //________________________________________RECONNECTING WIFI___________________________________________________________
   
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
      Serial.println(millis());
      Serial.println("Reconnecting to WiFi...");
     
      WiFi.begin("LaptopWiFi", "summerProject1234");

      while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(1000);
      }

      Serial.println(WiFi.localIP());
      
      previousMillis = currentMillis;
  }


  //Check WiFi connection status
  if(WiFi.status()== WL_CONNECTED){
    WiFiClient client;
    HTTPClient http;
   
    // Your Domain name with URL path or IP address with path
    http.begin(client, serverNameOne);
      
    // Specify content-type header
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  
      
 //_____________________________________ ASSIGNING RANDOM DATA_______________________________________________________
   
    x_direction = total_x; 
    y_direction = total_y;
    angle = m_theta; 
    battery_life = generate_battery();
    alien_present = generate_alien();
    colour = generate_colour();
    infrastructure_present = generate_infrastructure();
    building_present = generate_building_present();

  //_______________________________________POSTING DATA____________________________________________________________________
    
    Serial.print('\n');
    Serial.println("POST DATA");
    
    // Data to send with HTTP POST
    String httpRequestData = "api_key=" + apiKey + "&x_direction=" + x_direction + "&y_direction=" + y_direction + "&angle=" + angle + "&battery_life=" + battery_life + "&alien_present=" + alien_present + "&colour=" + colour + "&infrastructure_present=" +infrastructure_present + + "&building_present=" + building_present + "";
    Serial.print("httpRequestData: ");
    Serial.println(httpRequestData);
     
    // Send HTTP POST request
    int httpResponseCode = http.POST(httpRequestData);
      
    if (httpResponseCode>0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    }

    else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
       
    // Free resources
    http.end();

  }

  delay(1000); // 1 second

  
 
}


