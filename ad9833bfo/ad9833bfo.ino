/*
Revision 1.0 - July 22, 2018  - AD9833 oscillator/BFO

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// Include the library code
#include <SPI.h>
#include <rotary.h>
#include <Adafruit_SSD1306.h>
#define OLED_RESET 5   //12
Adafruit_SSD1306 display(OLED_RESET);

//Setup some items
const int SINE = 0x2000;                    // Define AD9833's waveform register value.
const int SQUARE = 0x2028;                  // When we update the frequency, we need to
const int TRIANGLE = 0x2002;                // define the waveform when we end writing.    

int wave = 0;
int waveType = SQUARE;
int wavePin = 7;

#define FBUTTON (A0)       // tuning step freq CHANGE from 1Hz to 1MHz step for single rotary encoder possition
//#define BTNDEC (A2)        // BAND CHANGE BUTTON from 1,8 to 29 MHz - 11 bands

const int FSYNC = 10;                       // Standard SPI pins for the AD9833 waveform generator.
const int CLK = 13;                         // CLK and DATA pins are shared with the TFT display.
const int DATA = 11;
const float refFreq = 25000000.0;           // On-board crystal reference frequency

#define pulseHigh(pin) {digitalWrite(pin, HIGH); digitalWrite(pin, LOW); }
Rotary r = Rotary(3,2); // sets the pins for rotary encoder uses.  Must be interrupt pins.
  
unsigned long rx=5998932; // Starting frequency of VFO    5998900  5999080 5998880
unsigned long rx2=1; // temp variable to hold the updated frequency
unsigned long increment = 50; // starting VFO update increment in HZ. tuning step
int buttonstate = 0;   // temp var
String hertz = "50Hz";
int  hertzPosition = 0;
int byteRead = 0;
int var_i = 0;

//byte ones,tens,hundreds,thousands,tenthousands,hundredthousands,millions ;  //Placeholders
String freq; // string to hold the frequency

// buttons temp var
int BTNdecodeON = 0;   
int BTNinc = 1; // set number of default band minus 1

// start variable setup

void setup() {

//set up the pins in/out and logic levels

//pinMode(BTNDEC,INPUT);    // band change button
//digitalWrite(BTNDEC,HIGH);    // level

pinMode(FBUTTON,INPUT); // Connect to a button that goes to GND on push - rotary encoder push button - for FREQ STEP change
digitalWrite(FBUTTON,HIGH);  //level

// Initialize the Serial port so that we can use it for debugging
Serial.begin(115200);
  
// Can't set SPI MODE here because the display and the AD9833 use different MODES.
SPI.begin();
delay(50); 

  AD9833reset();                                   // Reset AD9833 module after power-up.
  delay(50);
  AD9833setFrequency(rx, SQUARE);                  // Set the frequency and Sine Wave output
  
  Serial.println("BFO ver 1.0");

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C address 0x3C (for oled 128x32)
  
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
//  display.display();

  // Clear the buffer.
//  display.clearDisplay();	
//	display.setTextSize(2);
//	display.setTextColor(WHITE);
//	display.setCursor(0,0);
//	display.println(rx);
//	display.setTextSize(1);
//	display.setCursor(0,16);
//	display.print("St:");display.print(hertz);
//	display.display();
  
   //  rotary
  PCICR |= (1 << PCIE2);
  PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
  sei();
  
  AD9833setFrequency(rx, SQUARE);     // Set AD9833 to frequency and selected wave type.
  delay(50);

  Serial.println("START");
}

///// START LOOP - MAIN LOOP

void loop() {
//	checkBTNdecode();  // BAND change
	
// freq change 
  if (rx != rx2){
//	    showFreq();
      AD9833setFrequency(rx, SQUARE);     // Set AD9833 to frequency and selected wave type.
      rx2 = rx;
      }

//  step freq change + RIT ON/OFF  
  buttonstate = digitalRead(FBUTTON);
  if(buttonstate == LOW) {
        setincrement();        
  };

///    SERIAL COMMUNICATION - remote computer control for DDS - 1, 2, 3, 4, 5, 6 - worked 
   /*  check if data has been sent from the computer: */
if (Serial.available()) {
    /* read the most recent byte */
    byteRead = Serial.read();
  if(byteRead == 49){     // 1 - up freq
    rx = rx + increment;
    AD9833setFrequency(rx, SQUARE); 
    Serial.println(rx);
    }
  if(byteRead == 50){   // 2 - down freq
    rx = rx - increment;
    AD9833setFrequency(rx, SQUARE); 
    Serial.println(rx);
    }
  if(byteRead == 51){   // 3 - up increment
    setincrement();
    Serial.println(increment);
    }
  if(byteRead == 52){   // 4 - print VFO state in serial console
    Serial.println("BFO_VERSION 1.0");
    Serial.println(rx);
    Serial.println(increment);
    Serial.println(hertz);
    }
  if(byteRead == 53){   // 5 - scan freq forvard 40kHz 
             var_i=0;           
             while(var_i<=4000){
                var_i++;
                rx = rx + 10;
                AD9833setFrequency(rx, SQUARE);
                Serial.println(rx);
//                showFreq();
                if (Serial.available()) {
                    if(byteRead == 53){
                        break;                       
                    }
                }
             }        
   }

   if(byteRead == 54){   // 6 - scan freq back 40kHz  
             var_i=0;           
             while(var_i<=4000){
                var_i++;
                rx = rx - 10;
                AD9833setFrequency(rx, SQUARE); 
                Serial.println(rx);
//                showFreq();
                if (Serial.available()) {
                    if(byteRead == 54){
                        break;                       
                    }
                }
             }        
   }
   if(byteRead == 55){     // 1 - up freq
    rx = rx + increment;
    AD9833setFrequency(rx, SQUARE);
    Serial.println(rx);
   }
  if(byteRead == 56){   // 2 - down freq
    rx = rx - increment;
    AD9833setFrequency(rx, SQUARE); 
    Serial.println(rx);
  }
}

}	  
/// END of main loop ///
/// ===================================================== END ============================================


/// START EXTERNAL FUNCTIONS

ISR(PCINT2_vect) {
  unsigned char result = r.process();
  if (result) {  
		if (result == DIR_CW){rx=rx+increment;}
		else {rx=rx-increment;}
	}
}

// step increments for rotary encoder button
void setincrement(){
  if(increment == 0){increment = 1; hertz = "1Hz"; hertzPosition=0;} 
  else if(increment == 1){increment = 10; hertz = "10Hz"; hertzPosition=0;}
  else if(increment == 10){increment = 50; hertz = "50Hz"; hertzPosition=0;}
  else if (increment == 50){increment = 100;  hertz = "100Hz"; hertzPosition=0;}
  else if (increment == 100){increment = 500; hertz="500Hz"; hertzPosition=0;}
  else if (increment == 500){increment = 1000000; hertz="1Mhz"; hertzPosition=0;} 
  else{increment = 0; hertzPosition=0;};  
  showFreq();
  delay(250); // Adjust this delay to speed up/slow down the button menu scroll speed.
}

// oled display functions
void showFreq(){
//	display.clearDisplay();	
//	display.setTextSize(2);
//	display.setTextColor(WHITE);
//	display.setCursor(0,0);
//	display.println(rx);
	Serial.println(rx);
//	display.setTextSize(1);
//	display.setCursor(0,16);
//	display.print("St:");display.print(hertz);
//	display.setCursor(64,16);
//	display.print("rit:");display.print(rxRIT);
//	display.display();
}

// AD9833 documentation advises a 'Reset' on first applying power.
void AD9833reset() {
  WriteRegister(0x100);   // Write '1' to AD9833 Control register bit D8.
  delay(10);
}

// Set the frequency and waveform registers in the AD9833.
void AD9833setFrequency(long frequency, int Waveform) {

  long FreqWord = (frequency * pow(2, 28)) / refFreq;

  int MSB = (int)((FreqWord & 0xFFFC000) >> 14);    //Only lower 14 bits are used for data
  int LSB = (int)(FreqWord & 0x3FFF);
  
  //Set control bits 15 ande 14 to 0 and 1, respectively, for frequency register 0
  LSB |= 0x4000;
  MSB |= 0x4000; 
  
  WriteRegister(0x2100);   
  WriteRegister(LSB);                  // Write lower 16 bits to AD9833 registers
  WriteRegister(MSB);                  // Write upper 16 bits to AD9833 registers.
  WriteRegister(0xC000);               // Phase register
  WriteRegister(Waveform);             // Exit & Reset to SINE, SQUARE or TRIANGLE

}

void WriteRegister(int dat) { 
  
  // Display and AD9833 use different SPI MODES so it has to be set for the AD9833 here.
  SPI.setDataMode(SPI_MODE2);       
  
  digitalWrite(FSYNC, LOW);           // Set FSYNC low before writing to AD9833 registers
  delayMicroseconds(10);              // Give AD9833 time to get ready to receive data.
  
  SPI.transfer(highByte(dat));        // Each AD9833 register is 32 bits wide and each 16
  SPI.transfer(lowByte(dat));         // bits has to be transferred as 2 x 8-bit bytes.

  digitalWrite(FSYNC, HIGH);          //Write done. Set FSYNC high
}

//// OK END OF PROGRAM
