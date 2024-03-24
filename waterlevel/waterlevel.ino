int resval = 0;  // holds the value
int respin = A1; // sensor pin used

int resval2 = 0;  // holds the value
int respin2 = A2; // sensor pin used
  
void setup() { 
 
  // start the serial console
  Serial.begin(9600);
} 
  
void loop() { 
   
  resval = analogRead(respin); //Read data from analog pin and store it to resval variable
 resval2 = analogRead(respin2);
 Serial.print(resval);
 Serial.print(" ");
 Serial.print(resval2);
 Serial.print("\n");

  delay(1000); 
}
