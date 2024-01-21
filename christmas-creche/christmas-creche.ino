// Define color structure
typedef struct Color 
{
  int red;
  int green;
  int blue;
};

// Phases of the day
Color night;
Color sunrise;
Color day;
Color sunset;

// RGB Output
int PIN_BLUE = 6;
int PIN_GREEN = 5; 
int PIN_RED = 3; 

// Relays output
int PIN_REL_0 = 11;
int PIN_REL_1 = 10;
int PIN_REL_2 = 9;
int PIN_REL_3 = 8;

// Milliseconds for day and night
int WAIT_DAY_NIGHT = 60000;

// Milliseconds for transactions
// 50 milliseconds on 66 cycles = 12 seconds per transaction
int WAIT_COLOR_FADE = 50;


// Initial setup
void setup() {
  // Night
  night.red = 0;
  night.green = 0;
  night.blue = 255;

  // Sunrise
  sunrise.red = 255;
  sunrise.green = 25;
  sunrise.blue = 151;

  // Day
  day.red = 248;
  day.green = 255;
  day.blue = 168;

  // Sunset
  sunset.red = 227;
  sunset.green = 14;
  sunset.blue = 67;

  // Start first phase of the day
  setColor(day);

  // Power off relays
  pinMode(PIN_REL_0, OUTPUT);
  pinMode(PIN_REL_1, OUTPUT);
  pinMode(PIN_REL_2, OUTPUT);
  pinMode(PIN_REL_3, OUTPUT);
  digitalWrite(PIN_REL_0, HIGH);
  digitalWrite(PIN_REL_1, HIGH);
  digitalWrite(PIN_REL_2, HIGH);
  digitalWrite(PIN_REL_3, HIGH);
}


// Main loop
void loop() {

  // Wait
  delay(WAIT_DAY_NIGHT);

  // Fade from day to sunset
  fade(day, sunset);
  // Turn on fireplace
  digitalWrite(PIN_REL_0, LOW);   

  // Set Night
  fade(sunset, night);
  // Turn on house lights
  digitalWrite(PIN_REL_1, LOW);   
  // Turn on stars
  digitalWrite(PIN_REL_2, LOW);   
  
  // Wait
  delay(WAIT_DAY_NIGHT); 

  // Fade from night to sunrise
  fade(night, sunrise);
  /// Turn off fireplace
  digitalWrite(PIN_REL_0, HIGH);   
  // Turn off stars
  digitalWrite(PIN_REL_2, HIGH);   

  // Fade from sunrise to day
  fade(sunrise, day);
  // Turn off house lights
  digitalWrite(PIN_REL_1, HIGH);     
}


// Set color on RGB shield/led
void setColor(Color color)
{
  analogWrite(PIN_RED, color.red);
  analogWrite(PIN_GREEN, color.green);
  analogWrite(PIN_BLUE, color.blue);  
}


// Fading between colors
void fade(Color startColor, Color endColor)
{
  bool fade = true;
  while(fade)
  {
    // Check if the transaction is completed
    if((startColor.red == endColor.red) && (startColor.green == endColor.green) && (startColor.blue == endColor.blue))
    {
      // Exit
      fade = false;
      break;
    }
    else
    {
      // Red fading
      if(startColor.red > endColor.red)
        startColor.red--;
      else if(startColor.red < endColor.red)
        startColor.red++;
  
      // Green fading
      if(startColor.green > endColor.green)
        startColor.green--;
      else if(startColor.green < endColor.green)
        startColor.green++;
  
      // Blue fading
      if(startColor.blue > endColor.blue)
        startColor.blue--;
      else if(startColor.blue < endColor.blue)
        startColor.blue++;
  
      // Set output color
      setColor(startColor);
      
      // Transaction wait
      delay(WAIT_COLOR_FADE);
    }
  }
}


