// Define the pin connected to the buzzer's transistor base
const int BUZZER_PIN = 15; // Corresponds to GPIO15 or pin D15

void setup() {
  // Set the buzzer pin as an output
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Set the pin HIGH to turn on the buzzer (via the transistor)
  // NOTE: If using an active-low relay/transistor setup, you might need to set this LOW.
  digitalWrite(BUZZER_PIN, HIGH);
}

void loop() {
  sleep(5);
  digitalWrite(BUZZER_PIN, LOW);
}
