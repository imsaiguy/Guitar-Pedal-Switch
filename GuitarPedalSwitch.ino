
//==============================================================================================================
// Guitar Pedal Switch
// IMSAIGUY
// 2-25-2017
//==============================================================================================================

                                //pin assingments
int shift_data = A3;            //data to shift register
int shift_clock = A2;           //clock to shift register
int shift_load_LED = 3;         //load LEDs
int shift_load_Mux = A1;        //load multiplexers
int shift_load_Switches = A0;   //load switches
int read_SW1 = 4;               //switch bank one read sense (pedals)
int read_SW2 = 2;               //switch bank two read sense (menu)
int switch_state = 0;           //last switch pressed
int menu_state = 0;
int LED_buffer = 0;

//Multiplexer ABC value 0 to 7
//int mux[0] = 6;               //Guitar             outputs are 0 -> pedal 1
//int mux[1] = 6;               //Pedal 1                        1 -> pedal 2
//int mux[2] = 6;               //Pedal 2                        2 -> pedal 3
//int mux[3] = 6;               //Pedal 3                        3 -> pedal 4
//int mux[4] = 6;               //Pedal 4                        4 -> pedal 5
//int mux[5] = 6;               //Pedal 5                        5 -> pedal 6
//int mux[6] = 6;               //Pedal 6                        6 -> NC
//                              //                               7 -> amplifier

char mux[7]  {6, 6, 6, 6, 6, 6, 6};
int last_switch = 0;

//seven 3 bit octal numbers stacked into three 8 bit bytes
#define mux_byte_1  mux[0] | mux[1]<<3 | (mux[2] & B00000011)<<6
#define mux_byte_2  (mux[2] & B00000100)>>2 | mux[3]<<1 | mux[4]<<4 | (mux[5] & B0000001)<<7
#define mux_byte_3  (mux[5] & B00000110)>>1 | mux[6]<<2

char bitnumber[16];

//==============================================================================================================
//==============================================================================================================
void setup() {
  // put your setup code here, to run once:
  bitnumber[0] = B00000001;
  bitnumber[1] = B00000010;
  bitnumber[2] = B00000100;
  bitnumber[3] = B00001000;
  bitnumber[4] = B00010000;
  bitnumber[5] = B00100000;
  bitnumber[6] = B01000000;
  bitnumber[7] = B10000000;

  pinMode(shift_data, OUTPUT);
  pinMode(shift_clock, OUTPUT);
  pinMode(shift_load_LED, OUTPUT);
  pinMode(shift_load_Mux, OUTPUT);
  pinMode(shift_load_Switches, OUTPUT);

  pinMode(read_SW1, INPUT);
  pinMode(read_SW2, INPUT);

  digitalWrite(shift_load_LED, HIGH);       //reset all lines high
  digitalWrite(shift_load_Switches, HIGH);  //load signals are low going pulses
  digitalWrite(shift_load_Mux, HIGH);

  update_mux();                             //intitialize mux to all open circuits
}
//==============================================================================================================
//==============================================================================================================

//==============================================================================================================
//==============================================================================================================
void loop() {
  menu_polling();   //run main menu
}
//==============================================================================================================
//==============================================================================================================

//==============================================================================================================
// move shift register to output register in 74HC595
// low going pulses
int load_LED()
{ digitalWrite(shift_load_LED, LOW);
  digitalWrite(shift_load_LED, HIGH);
}
int load_Switches()
{ digitalWrite(shift_load_Switches, LOW);
  digitalWrite(shift_load_Switches, HIGH);
}
int load_Mux()
{ digitalWrite(shift_load_Mux, LOW);
  digitalWrite(shift_load_Mux, HIGH);
}
//==============================================================================================================

//==============================================================================================================
// shift in zeros
int shift_zeros()
{ triple_shift(0, 0, 0);
}
//==============================================================================================================


//==============================================================================================================
void LEDs_reverse()
{ for (int i = 0; i < 8; i++)
  { triple_shift(menu_state << 1, bitnumber[7 - i], 0);
    load_LED();
    delay(40);
  }
}
//==============================================================================================================

//==============================================================================================================
//read switches
// returns 0 if no switch
// returns 1 to 12 for key press
// returns 16 if bank 1 key and bank 2 key pressed together

// Board layout:

// --- ---  ---  ---     --- --- --- --- --- --- --- ---
// |9| |10| |11| |12|    |1| |2| |3| |4| |5| |6| |7| |8|
// --- ---  ---  ---     --- --- --- --- --- --- --- ---

//checks for a key press
//then scrolls to figure out which switch
int Read_switches()
{ int which_switch = 0;
  shift_zeros();
  load_Switches();                //set all lines low
  if (digitalRead(read_SW1) == 0) //detect switch in bank 1 (pedals)
  { for (int i = 0; i < 8; i++)
    { shiftOut (shift_data, shift_clock, MSBFIRST, bitnumber[i]);
      load_Switches();
      if (digitalRead(read_SW1) != 0)
      { which_switch = 8 - i;     //1 thru 8
        break;
      }
    }
  }
  if (digitalRead(read_SW2) == 0) //detect switch in bank 2 (menu)
  { for (int i = 0; i < 8; i++)
    { shiftOut (shift_data, shift_clock, MSBFIRST, bitnumber[i]);
      load_Switches();
      if (digitalRead(read_SW2) != 0)
      { which_switch = 12 - i;    //9 thru 12
        break;
      }
    }
  }
  if (digitalRead(read_SW1) == 0 && digitalRead(read_SW2) == 0)
  { which_switch = 16;
  }
  return which_switch;
}
//==============================================================================================================

//==============================================================================================================
void update_mux()
{ triple_shift(mux_byte_3, mux_byte_2, mux_byte_1);
  load_Mux();
}
//==============================================================================================================


//==============================================================================================================
//controls mux[0] only
void manual_mode()
{ switch_state = Read_switches();
  if (switch_state != 0)
  { triple_shift(menu_state << 1, bitnumber[switch_state - 1], 0);
    load_LED();
    mux[0] = switch_state - 1;
    update_mux();
  }
}

//==============================================================================================================
//displays switch value in binary on 4 menu LEDs
void display_switches()
{ switch_state = Read_switches();
  if (switch_state != 0)
  { triple_shift(switch_state << 1, bitnumber[switch_state - 1], 0);
    load_LED();
  }
}

//==============================================================================================================
//starts with guitar clean (guitar -> amp)
//each button press inserts pedal in any order
//
void pedal_order()
{ switch_state = Read_switches();
  if (switch_state != 0)
  { LED_buffer = LED_buffer | bitnumber[switch_state - 1];
    triple_shift(menu_state << 1, LED_buffer, 0);
    load_LED();
    mux[last_switch] = switch_state - 1;  //connect to pedal input selected
    mux[switch_state] = 7;                //connect    pedal output to amp
    last_switch = switch_state;
    update_mux();
    while(Read_switches()!=0)             //debounce
    {}
  }
}

//==============================================================================================================
//main menu polling
//
void menu_polling()
{ for (int i = 0; i < 8; i++)
  { triple_shift(menu_state, bitnumber[i], 0);  
    load_LED();                                       //flash LEDs to show operational 
    delay(40);                                        //and waiting for menu selection
    switch_state = Read_switches();
    if (switch_state == 9)                            //menu '+' key
    { menu_state = 1;
      while (Read_switches() != 12)                   //until 'menu' key pressed
      { manual_mode();
      }
      menu_state = 0;
    }
    if (switch_state == 10)                            //menu '-' key
    { last_switch = 0;
      int_mux();
      LED_buffer = 0;
      menu_state = 2;
      triple_shift(menu_state << 1, LED_buffer, 0);
      load_LED();
      while (Read_switches() != 0)                    //debounce
      {}
      while (Read_switches() != 12)                   //until 'menu' key pressed
      { // LEDs_reverse();
        pedal_order();
      }
      LED_buffer = 0;
      menu_state = 0;
    }
    if (switch_state == 11)                           //menu '!' key
    { menu_state = 3;
      while (Read_switches() != 12)                   //until 'menu' key pressed
      { display_switches();
      }
      menu_state = 0;
    }
  }
}


//==============================================================================================================
//initial mux to guitar -> amp
void int_mux()
{ for (int i = 1; i < 7; i++)
  { mux[i] = 6; //turn off all mux
  }
  mux[0] = 7;  //except [0], send guitar straight through (clean)
  update_mux();
}

//==============================================================================================================
//shift three bytes into shift registers
void triple_shift(byte byte3, byte byte2, byte byte1)
{ shiftOut (shift_data, shift_clock, MSBFIRST, byte3);
  shiftOut (shift_data, shift_clock, MSBFIRST, byte2);
  shiftOut (shift_data, shift_clock, MSBFIRST, byte1);
}

