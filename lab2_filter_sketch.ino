// A digital frequency selective filter
// A. Kruger, 2019
// revised R. Mudumbai, 2020 & 2024
// revised N. Najeeb, 2025

#include <WiFi.h>

#define WIFI_SSID "UI-DeviceNet"
#define WIFI_PASSWORD "UI-DeviceNet"

#define SMTP_HOST "ns-mx.uiowa.edu"
#define SMTP_PORT 25

#define AUTHOR_EMAIL "seniordesignteam3@uiowa.edu"
#define RECIPIENT_EMAIL "sage-marks@uiowa.edu"

WiFiClient client;

int analogPin = 15;     // Specify analog input pin. 
const int BUZZER_PIN = 23; // Corresponds to GPIO15 or pin D15

// num and den are the numerator and denominator coeffs of a digital frequency-selective filter
// designed for a sample rate Fs=2000 HZ

const int n = 10;   // number of past input and output samples to buffer; change this to match order of your filter
int m = 10; // number of past outputs to average for hysteresis

float den[] = {1, -2.10007517706225, 6.45525419156908, -8.65572833763769, 14.0428045937685, -12.7032216008626, 13.4011439722928, -7.88138375259155, 5.60915797650072, -1.74034180145190, 0.790812825614102}; //Denominator Coefficients
float num[] = {0.000184579082014068, -0.000270420331775373, 0.000481723705391312, -0.000411438996736231, 0.000325864338053812, 0, -0.000325864338053812, 0.000411438996736231, -0.000481723705391312, 0.000270420331775373, -0.000184579082014068}; //Numerator Coefficients


float x[n],y[n],y_n, s[10];     // Space to hold previous samples and outputs; n'th order filter will require upto n samples buffered

float threshold_val = 0.2; // Threshold value

// time between samples Ts = 1/Fs. If Fs = 2000 Hz, Ts=500 us
int Ts = 500;

bool is_alarm_active = false; 
bool email_sent_flag = false;

void setup()
{
   Serial.begin(115200);
   int i;
   
   pinMode(BUZZER_PIN, OUTPUT);

   for (int i = 0; i < n; i++) {
    x[i] = y[i] = 0;
   }
   for (int i = 0; i < m; i++) {
    s[i] = 0;
   }
   y_n = 0;

   analogReadResolution(12);      // 12-bit (0-4095)
   analogSetAttenuation(ADC_11db); // scale ~0-3.6V

   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
   Serial.print("Connecting to Wi-Fi");
   while (WiFi.status() != WL_CONNECTED)
         {
           Serial.print(".");
           delay(300);
         }
   Serial.println();
   Serial.print("Connected with IP: ");
   Serial.println(WiFi.localIP());  
}

void loop()
{
   unsigned long t1;
   int i,count,val;
   int k=0;
   float changet = micros();

   count = 0;
   void loop()
{
    unsigned long t1;
    int i,count,val;
    // float changet = micros(); // changet is reset every time loop() runs, which is incorrect in Arduino.
    // However, in your original code, the logic jumps to while(1) immediately, so this is okay.
    // For clarity and standard practice, it's best to declare it outside loop() or rely on the while(1) structure.
    
    // Note: 'count' and 'k' are not used and can be removed for cleaner code.
    
    // We maintain changet as a local variable since your original structure relies on the
    // infinite while(1) loop nested inside the standard Arduino loop().
    float changet = micros(); 

    while (1) {
        t1 = micros();

        // Shift samples
        for(i=n-1; i>0; i--){
            x[i] = x[i-1];
            y[i] = y[i-1];
        }
        
        // Shift absolute output for hysteresis
        for(i=m-1; i>0; i--){
            s[i] = s[i-1];
        }
        
        val = analogRead(analogPin);  // New input

        // Scale to match ADC resolution and range: (0-4095) * (3.3/4095) - 1.65 (Center-shifted)
        x[0] = val * (3.3 / 4095.0) - 1.65;

        // Calculate the next output y[0] using the difference equation
        y_n = num[0] * x[0];
        
        for(i=1; i<n; i++) {
            y_n = y_n - den[i]* y[i] + num[i] * x[i];
        }
        
        y[0] = y_n; // Store the new output

        s[0] = abs(2*y_n); // Absolute value of the filter output.

        // Hysteresis: Take the max of the past 'm' samples
        float maxs = 0;
        for(int i = 0; i < m; i++)
        {
            if (s[i] > maxs)
                maxs = s[i];
        }

        if ((micros() - changet) > 1000000)
        {
            Serial.print("Max Filter Output: ");
            Serial.println(maxs);

            changet = micros();

            // Alarm is triggered when the filtered signal is BELOW the threshold (beam broken)
            if (maxs < threshold_val)
            {
                // Beam Broken: Activate Alarm State
                is_alarm_active = true;
            }
            else
            {
                // Beam Connected: Clear Alarm State
                is_alarm_active = false;
                email_sent_flag = false;
            }

            if (is_alarm_active) 
            {
                digitalWrite(BUZZER_PIN, HIGH);
            } 
            else 
            {
                digitalWrite(BUZZER_PIN, LOW);
            }

            // Only proceed if alarm is active AND email has NOT been sent for this event
            if (is_alarm_active && !email_sent_flag) 
            {
                if (WiFi.status() != WL_CONNECTED) {
                    Serial.println("Warning: Wi-Fi lost, cannot send email.");
                } 
                else if (!client.connect(SMTP_HOST, SMTP_PORT))
                {
                    Serial.println("Connection to SMTP server failed");
                }
                else
                {
                    // Basic SMTP conversation
                    client.println("HELO esp32");
                    client.println("MAIL FROM:<" AUTHOR_EMAIL ">");
                    client.println("RCPT TO:<" RECIPIENT_EMAIL ">");
                    client.println("DATA");
                    client.println("Subject: Beam Break Alert!");
                    client.println("The IR beam connection was broken.");
                    client.println(".");
                    client.println("QUIT");
                    client.stop(); // Close connection
                    
                    Serial.println("Email alert sent successfully!");
                    email_sent_flag = true; // <<< CRITICAL: Prevent re-sending
                }
            }
        }
        
        if((micros()-t1) > Ts)
        {
            // If this happens, your filtering/logic took too long (> 500 us)
            Serial.println("MISSED A SAMPLE");
        }
        
        // Block until the 500 us interval (Ts) has passed
        while((micros()-t1) < Ts);
    }
}
