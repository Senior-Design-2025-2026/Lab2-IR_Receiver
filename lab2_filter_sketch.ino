// A digital frequency selective filter
// A. Kruger, 2019
// revised R. Mudumbai, 2020 & 2024
// revised N. Najeeb, 2025

#include <WiFi.h>

#define WIFI_SSID "UI-DeviceNet"
#define WIFI_PASSWORD "UI-DeviceNet"

#define SMTP_HOST "ns-mx.uiowa.edu"
#define SMTP_PORT 25

/* The sign in credentials */
#define AUTHOR_EMAIL "seniordesignteam3@uiowa.edu"

/* Recipient's email*/
#define RECIPIENT_EMAIL "sage-marks@uiowa.edu"

WiFiClient client;

int analogPin = 34;     // Specify analog input pin. 
int LED = 2;           // Specify output analog pin with indicator LED 

   // num and den are the numerator and denominator coeffs of a digital frequency-selective filter
   // designed for a sample rate Fs=3000 HZ
  
const int n = 7;   // number of past input and output samples to buffer; change this to match order of your filter
int m = 10; // number of past outputs to average for hysteresis

float den[] = {1, -2.10007517706225, 6.45525419156908, -8.65572833763769, 14.0428045937685, -12.7032216008626, 13.4011439722928, -7.88138375259155, 5.60915797650072, -1.74034180145190, 0.790812825614102}; //Denominator Coefficients
float num[] = {0.000184579082014068, -0.000270420331775373, 0.000481723705391312, -0.000411438996736231, 0.000325864338053812, 0, -0.000325864338053812, 0.000411438996736231, -0.000481723705391312, 0.000270420331775373, -0.000184579082014068}; //Numerator Coefficients


float x[n],y[n],y_n, s[10];     // Space to hold previous samples and outputs; n'th order filter will require upto n samples buffered

float threshold_val = 0.2; // Threshold value. Anything higher than the threshold will turn the LED off, anything lower will turn the LED on

// time between samples Ts = 1/Fs. If Fs = 3000 Hz, Ts=333 us
int Ts = 333;

void setup()
{
   Serial.begin(115200);
   pinMode(LED, OUTPUT);
   int i;

   pinMode(LED,OUTPUT);   // Makes the LED pin an output

   for (int i = 0; i < n; i++) {
    x[i] = y[i] = 0;
   }
   for (int i = 0; i < m; i++) {
    s[i] = 0;
   }
   y_n = 0;

   analogReadResolution(12);      // 12-bit (0-4095)
   analogSetAttenuation(ADC_11db); // scale ~0-3.6V
}



void loop()
{
   unsigned long t1;
   int i,count,val;
   int k=0;
   float changet = micros();

   count = 0;
   while (1) {
      t1 = micros();

      // Calculate the next value of the difference equation.

      for(i=n-1; i>0; i--){             // Shift samples
         x[i] = x[i-1];                                            
         y[i] = y[i-1];
      }
      
      for(i=m-1; i>0; i--){             // Shift absoulute output
         s[i] = s[i-1];
      }
      val = analogRead(analogPin);  // New input

      x[0] = val * (3.3 / 4095.0) - 1.65;  // Scale to match ADC resolution and range

      y_n = num[0] * x[0];
      
      for(i=1;i<n;i++)             // Incorporate previous outputs (y[n])
         y_n = y_n - den[i]* y[i] + num[i] * x[i];          
         

       y[0] = y_n;                  // New output

      //  The variable yn is the output of the filter at this time step.
      //  Now we can use it for its intended purpose:
      //       - Apply theshold
      //       - Apply hysteresis
      //       - What to do when the beam is interrupted, turn on a buzzer, send SMS alert.
      //       - etc.

      s[0] = abs(2*y_n);  // Absolute value of the filter output.

      // SAMPLE Hystersis: Take the max of the past 10 samples and compare that with the threshold
      float maxs = 0;
      for(int i = 0; i< m; i++)
      {
        if (s[i]>maxs)
          maxs = s[i];
      }


      
      // Check the output value against the threshold value every 10^6 microseconds or 1 second
      if ((micros()-changet) > 1000000)
      {
        Serial.println(maxs);

        changet = micros();
//        if(maxs < threshold_val)
          if (true)
        {
          digitalWrite(LED, HIGH);
           WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
           Serial.print("Connecting to Wi-Fi");
            while (WiFi.status() != WL_CONNECTED){
              Serial.print(".");
              delay(300);
            }
            Serial.println();
            Serial.print("Connected with IP: ");
            Serial.println(WiFi.localIP());
            Serial.println();

            if (!client.connect(SMTP_HOST, SMTP_PORT)) {
              Serial.println("Connection to SMTP server failed");
              return;
            }
          
            // Basic SMTP conversation
            client.println("HELO esp32");
            client.println("MAIL FROM:<" AUTHOR_EMAIL ">");
            client.println("RCPT TO:<" RECIPIENT_EMAIL ">");
            client.println("DATA");
            client.println("Subject: Test Email from ESP32");
            client.println("Hello! This email was sent without authentication.");
            client.println(".");
            client.println("QUIT");
          
            Serial.println("Email sent (check inbox)");
            }
          else
              {
                digitalWrite(LED, HIGH);
              }
        }
        
      
      // The filter was designed for a 3000 Hz sampling rate. This corresponds
      // to a sample every 333 us. The code above must execute in less time
      // (if it doesn't, it is not possible to do this filtering on this processor).
      // Below we tread some water until it is time to process the next sample

     
      if((micros()-t1) > Ts)
      {
		// if this happens, you must reduce Fs, and/or simplify your filter to run faster        
		Serial.println("MISSED A SAMPLE"); 
      }
      while((micros()-t1) < Ts);  
      
   }

}
