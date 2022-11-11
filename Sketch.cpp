
#define F_CPU 16000000UL

//importing related libraries
#include <avr/io.h>
#include <util/delay.h>
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

//LCD
#define LCD_DATA PORTL //PORTL selected as LCD DATA port
#define ctrl DDRL //PORTL selected as command port

#define en PL1 //enable command port
#define rs PL0 //resistor select command port

//function declarations

void openfile(String filename);
void start_play_withfilter(String filename);
ISR(TIMER0_COMPA_vect );
void pitch_shift();
void start_play_pitch(String filename, byte factor);
ISR(TIMER1_COMPA_vect );
//////////////
void create_wav_format(String F_name);
void ADC_Setup();
void ADC_run_timer();
ISR(TIMER2_COMPA_vect );
void stop_ADC();
void do_conversion();
//////////////
void LCD_cmd(unsigned char cmd);
void init_LCD(void);
void LCD_write(unsigned char data);
void LCD_clear(void);
void LCD_Print (char *str);
void LCD_Printpos (char row, char pos, char *str);
//////File Man
void playFile(String fileName);
void stopPlay();
void startRecord(const char *fileName);
void stopRecord(const char *fileName);
void deleteFile(String fileName);
void readFile(String fileName);
void gallery();
int getCount(File dir);
void fileList(File dir);
void createFile(String fileName);
void writeLine(String fileName, String text);
void updateMenu();
void recordAction();
String generateName();
void mainAction();
void updateOptions( );
void opAction(String f_name);
void action1();
void action2();
void action3();


File data_file;
File file;

// initializing main parameters
int menu = 1;
int subMenu = 0;
int fileCount = 0;

//ADC variables
bool stat=0; //state changing variable
uint16_t ADC_out; //ADC result

//Header data
//sampling rate 8000
// bits per sample 8
char Header[44] = {/*RIFF Section*/0x52,0x49,0x46,0x46,0x00,0x00,0x00,0x00,0x57,0x41,0x56,0x45,/*Format Section*/0x66,0x6D,0x74,0x20,0x10,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x40,0x1F,0x00,0x00,0x40,0x1F,0x00,0x00,0x01,0x00,0x08,0x00,/*Data Header*/0x64,0x61,0x74,0x61,0x00,0x00,0x00,0x00};
boolean is_rec;

#define cs_pin 53

//pitch_shift
byte pitch_factor=0;
short ocr1a_val;

//low pass filter
volatile int preval=0;

boolean which_effect;// select effect

int main() {
  DDRA = 0xFF; //set PORTA as output
  DDRF = DDRF & ~(1<<0); //set A0 as input
  DIDR0 = 0b00000001;
  
  

  //PUSH Buttons
  DDRC = DDRC & ~(1<<6); //OK button
  DDRC = DDRC & ~(1<<4); //BACK button
  DDRC = DDRC & ~(1<<7); //UP button
  DDRC = DDRC & ~(1<<5); //DOWN button
  PORTC = 0xF0; //enable pull up resistors
  if(!SD.begin(cs_pin)){//Initializing SD card
    while(1);//Try again
  }
  //start LCD
  init_LCD();
  //welcome message
  LCD_cmd(0xC2);
  LCD_Print(" WELCOME!!...");
  _delay_ms(300);
  LCD_clear();
  
  updateMenu();
  
  fileCount = getCount(SD.open("/")); //count the file in SD
  
  //start_play_pitch("sith.wav",1);
  //start_play_withfilter("sith.wav");
  
  while(1) {
	  if (~PINC & (1<<5))//check for DB press
	  {
		  menu++;
		  updateMenu();
		  _delay_ms(100);
		  while (~PINC & (1<<5));
	  }
	  if (~PINC & (1<<7)) //check for UB press
	  {
		  menu--;
		  updateMenu();
		  _delay_ms(100);
		  while (~PINC & (1<<7));
	  }
	  if (~PINC & (1<<6))  //check for SB press
	  {
		  mainAction();
		  updateMenu();
		  _delay_ms(100);
		  while (~PINC & (1<<6));
	  }
	  
  }
  
  

}



//FileManager code--------------------------------------------------------------

//playing a WAV file
void playFile(String fileName)
{
	start_play_pitch(fileName,0);
}

//stop playing a WAV file
void stopPlay()
{
	TIMSK0=0; //set counter to 0
	TIMSK1=0;
	PORTA=0; //set outputs to LOW
}

//record and save WAV file
void startRecord(const char *fileName)
{
	
	is_rec=true; //Turn ON ADC
	create_wav_format(fileName);

}

void stopRecord(const char *fileName)
{

	is_rec = false; //stop ADC
}

//delete a file
void deleteFile(String fileName)
{
	
	//checking for the file..
	if (SD.exists(fileName))
	{
		SD.remove(fileName);
		
	}

}

//read a file
void readFile(String fileName)
{
	data_file = SD.open(fileName);
	if (data_file)
	{
		while (data_file.available())
		{
		}
		data_file.close();
		
	}
}

//Gallery
void gallery()
{
	File dir = SD.open("/");
	String fileNames[fileCount];
	dir.rewindDirectory();
	int index = 0;
	//getting the file name in to the fileNames array
	while (true)
	{
		File entry =  dir.openNextFile();
		if (!entry)
		break;
		fileNames[index] = entry.name();
		entry.close();
		index++;
	}
	
		LCD_cmd(0xC0);//row 2
		int n1 = fileNames[0].length(); //convert string to char
		char zero[n1+1];
		strcpy(zero,fileNames[0].c_str());
		LCD_Print(zero);
		LCD_cmd(0x90); //row 3
		int n2 = fileNames[1].length();
		char one[n2+1];
		strcpy(one,fileNames[1].c_str());
		LCD_Print(one);
		LCD_cmd(0xD0); //row 4
		int n3 = fileNames[2].length();
		char two[n3+1];
		strcpy(two,fileNames[2].c_str());
		LCD_Print(two);
	
}

//calculate the file count
int getCount(File dir)
{
	int count = 0;
	
	while (true)
	{
		File entry =  dir.openNextFile();
		if (!entry)
		break;
		entry.close();
		count++;
	}
	return count;
}

// print files 
void fileList(File dir)
{
	fileCount = getCount(dir); // get the file count and update the global variable
	String fileNames[fileCount];
	dir.rewindDirectory();
	int index = 0;
	while (true)
	{
		File entry =  dir.openNextFile();
		if (!entry)
		break;
		fileNames[index] = entry.name();
		entry.close();
		index++;
	}
	String tempFile = "temp.txt";
	if (SD.exists(tempFile)) // for removing irremovable temp files to avoid overwriting
	{SD.remove(tempFile); 
	}
	createFile(tempFile);
	writeLine(tempFile,String(index));
	for (int i=0; i<index; i++)
	{
		writeLine(tempFile,fileNames[i]);
	}
}

//creating a file
void createFile(String fileName)
{
	data_file = SD.open(fileName, FILE_WRITE);
	data_file.close();
}

//writing in a file
void writeLine(String fileName, String text)
{
	data_file = SD.open(fileName, FILE_WRITE);
	if (data_file)
	{
		data_file.close();
		
	}	
}

//Display code for LCD-----------------------------------------------------
void updateMenu() {
	switch (menu) {
		case 0:
		menu = 3;
		LCD_clear();
		LCD_cmd(0x80);
		LCD_Print("Record");
		LCD_cmd(0xC0);
		LCD_Print("Gallery");
		LCD_cmd(0x90);
		LCD_Print(">About Device<");
		break;
		case 1:
		LCD_clear();
		LCD_cmd(0x80);
		LCD_Print(">Record<");
		LCD_cmd(0xC0);
		LCD_Print("Gallery");
		LCD_cmd(0x90);
		LCD_Print("About Device");

		break;
		case 2:
		LCD_clear();
		LCD_cmd(0x80);
		LCD_Print("Record");
		LCD_cmd(0xC0);
		LCD_Print(">Gallery<");
		LCD_cmd(0x90);
		LCD_Print("About Device");
		break;
		case 3:
		LCD_clear();
		LCD_cmd(0x80);
		LCD_Print("Record");
		LCD_cmd(0xC0);
		LCD_Print("Gallery");
		LCD_cmd(0x90);
		LCD_Print(">About Device<");
		break;
		case 4:
		menu = 1;
		LCD_clear();
		LCD_cmd(0x80);
		LCD_Print(">Record<");
		LCD_cmd(0xC0);
		LCD_Print("Gallery");
		LCD_cmd(0x90);
		LCD_Print("About Device");
		break;
	}
}

void recordAction()
{
	String tempString = generateName();
	int len = tempString.length() + 1;
	char recName[len];
	tempString.toCharArray(recName,len);
	LCD_clear();
	LCD_cmd(0x80);
	LCD_Print("  RECORDING... ");
	LCD_cmd(0xC0);
	LCD_Print(recName);
	LCD_cmd(0x90);
	LCD_Print("   Press OK to");
	LCD_cmd(0xD0);
	LCD_Print("      STOP ");
	
	startRecord(recName);  //start recording
	_delay_ms(200); // to stop jumping in to stop recording when pressed OK to record from the record sub menu
	stopRecord(recName);   //stop recording
	LCD_clear();
	LCD_cmd(0x80);
	LCD_Print("  RECORDING... ");
	LCD_cmd(0xC0);
	LCD_Print("    STOPPED!!   ");
	LCD_cmd(0x90);
	LCD_Print(" Press BACK for ");
	LCD_cmd(0xD0);
	LCD_Print("   Main menu   ");
	while(PINC & (1<<4)); //waiting for back key press
}

// generate a suitable name
String generateName()
{
	String tempName;
	File dir = SD.open("/");
	String nameList[fileCount];
	int index = 0; // index for assigning strings to the array
	//getting the file name in to the NameList array
	while (true)
	{
		File entry =  dir.openNextFile();
		if (!entry)
		break;
		nameList[index] = entry.name(); // assigning the names into the array
		entry.close();
		index++;
	}
	//searching for generate a name that doesn't exist in SD
	int num = 1;
	bool found_name;
	while (true)
	{
		found_name = false;
		tempName = "REC_" + String(num) + ".wav";
		for (int i=0; i<fileCount; i++)
		{
			if(nameList[i].equalsIgnoreCase(tempName))
			{
				found_name = true;
				break;
			}
			
		}
		num++;
		if(!found_name)
		return tempName;
	}
}

void mainAction()
{
	switch (menu)
	{
		case 1:
		action1();
		break;
		case 2:
		action2();
		break;
		case 3:
		action3();
		break;
	}
}

//
int opMenu = 1;
void updateOptions( )
{
	switch (opMenu)
	{
		case 0:
		opMenu = 3;
		LCD_clear();
		LCD_cmd(0x80);
		LCD_Print(" -- OPTIONS -- ");
		LCD_cmd(0xC0);
		LCD_Print("Play");
		LCD_cmd(0x90);
		LCD_Print("Use_effect");
		LCD_cmd(0xD0);
		LCD_Print("> Delete");
		break;
		case 1:
		LCD_clear();
		LCD_cmd(0x80);
		LCD_Print(" -- OPTIONS -- ");
		LCD_cmd(0xC0);
		LCD_Print("> Play");
		LCD_cmd(0x90);
		LCD_Print("Use_effect");
		LCD_cmd(0xD0);
		LCD_Print("Delete");
		break;
		case 2:
		LCD_clear();
		LCD_cmd(0x80);
		LCD_Print(" -- OPTIONS -- ");
		LCD_cmd(0xC0);
		LCD_Print("Play");
		LCD_cmd(0x90);
		LCD_Print("> Use_effect");
		LCD_cmd(0xD0);
		LCD_Print("Delete");
		break;
		case 3:
		LCD_clear();
		LCD_cmd(0x80);
		LCD_Print(" -- OPTIONS -- ");
		LCD_cmd(0xC0);
		LCD_Print("Play");
		LCD_cmd(0x90);
		LCD_Print("Use_effect");
		LCD_cmd(0xD0);
		LCD_Print("> Delete");
		break;
		case 4:
		opMenu = 1;
		LCD_clear();
		LCD_cmd(0x80);
		LCD_Print(" -- OPTIONS -- ");
		LCD_cmd(0xC0);
		LCD_Print("> Play");
		LCD_cmd(0x90);
		LCD_Print("Use_effect");
		LCD_cmd(0xD0);
		LCD_Print("Delete");
		break;
	}
}

void opAction(String f_name)
{
	switch (opMenu)
	{
		case 1:
		{
		LCD_clear();
		LCD_cmd(0x80);
		LCD_Print("   Playing... ");
		LCD_cmd(0xC0);
		int n4 = f_name.length();
		char f_n_n4[n4+1];
		strcpy(f_n_n4,f_name.c_str());
		LCD_Print(f_n_n4);
		//playing..
		playFile(f_name); // call to play function
		LCD_cmd(0x90);
		LCD_Print("  Press OK to");
		LCD_cmd(0xD0);
		LCD_Print("      STOP ");
		while(PINC & (1<<4)) // waiting for back button
		{
			if ((~PINC & (1<<6)))  //OK button
			{
				//stop playing....
				stopPlay();
				LCD_clear();
				LCD_cmd(0x80);
				LCD_Print("  Stopped! ");
				LCD_cmd(0xC0);
				int n5 = f_name.length();
				char f_n_n5[n5+1];
				strcpy(f_n_n5,f_name.c_str());
				LCD_Print(f_n_n5);
				LCD_cmd(0x90);
				LCD_Print(" Press BACK for ");
				LCD_cmd(0xD0);
				LCD_Print("     Options   ");
				//while (PINC & (1<<4));
			}
			
		}
		
		while (~PINC & (1<<4)); // to avoid pressing the back button multiple times
		break;
		}
		case 2: //----------------------- Effects
		LCD_clear();
		LCD_cmd(0x80);
		LCD_Print(" -- Effects -- ");
		LCD_cmd(0x90);
		LCD_Print("UP-PITCH_CHANGE");
		LCD_cmd(0xD0);
		LCD_Print("DOWN-LP_FILTER");
		while(1){
			if(~PINC & (1<<7)){
				which_effect=1;
				break;
			}
			if(~PINC & (1<<5)){
				which_effect=0;
				break;
			}
		}
		
		if(which_effect){
			start_play_pitch(f_name,1);//playing with effects..
			}else{
			start_play_withfilter(f_name);//play with low pass filter
		}
		LCD_clear();
		LCD_cmd(0x80);
		LCD_Print("    Playing");
		LCD_cmd(0xC0);
		LCD_Print("      with");
		LCD_cmd(0x90);
		LCD_Print("    Effects...");
		LCD_cmd(0xD0);
		LCD_Print("TO STOP PRESS OK");
		
		while(PINC & (1<<4)) // waiting for back button
		{

			if (~PINC & (1<<6))  //OK button
			{
				stopPlay(); //stop playing....
				LCD_clear();
				LCD_cmd(0xC0);
				LCD_Print("   PRESS BACK");
				LCD_cmd(0x90);
				LCD_Print("   FOR OPTIONS");
			}
			
		}
		while (~PINC & (1<<4)); // to avoid pressing the back button multiple times
		break;
		
		case 3:
		{
		LCD_clear();
		LCD_cmd(0x80);
		LCD_Print("  Deleted! ");
		LCD_cmd(0xC0);
		int n6 = f_name.length();
		char f_n_n6[n6+1];
		strcpy(f_n_n6,f_name.c_str());
		LCD_Print(f_n_n6);
		deleteFile(f_name);//deleting..
		LCD_cmd(0x90);
		LCD_Print(" Press BACK for ");
		LCD_cmd(0xD0);
		LCD_Print("   Main menu   ");
		while(PINC & (1<<4));// waiting for back button
		_delay_ms(300);
		break;
		}
	}
}

void action1()
{
	LCD_clear();
	LCD_cmd(0x80);
	LCD_Print("  > Record <  ");
	LCD_cmd(0xC0);
	LCD_Print(" Press OK to ");
	LCD_cmd(0x90);
	LCD_Print("    start    ");
	_delay_ms(300); // stop jump in to recording when pressed OK to enter to the record sub menu
	while(1)
	{
		if (~PINC & (1<<6))
		{
			recordAction();
			return;
		}
		else if (~PINC & (1<<4)) //what should do if press back
		return;
	}

}

// Gallery Menu
void action2()
{
	const char row_pos[4] = {0x80,0xC0,0x90,0xD0}; //Array with LCD row ID
	File dir = SD.open("/");
	fileCount = getCount(dir); // get the file count and update the global variable
	String fileNames[fileCount];
	dir.rewindDirectory();
	int index = 0; // index for assigning strings to the array
	//getting the file name in to the fileNames array
	while (true)
	{
		File entry =  dir.openNextFile();
		if (!entry)
		break;
		fileNames[index] = entry.name(); // assigning the names into the array
		entry.close();
		index++;
	}
	//update the gallery menu
	int pointerLine = 1;
	int x = 0; //variable for scroll the file list
	int displayIndex;
	int upCount = 0;
	int downCount = 2;
	
	LCD_clear();
	LCD_cmd(0x80);
	LCD_Print(" --- Gallery --- ");
	/*
	int n1 = fileNames[0].length(); // code snippet to convert string to char
	char zero[n1+1];
	strcpy(zero,fileNames[0].c_str());
	*/
	for (int i=0;i<3;i++)
	{
		LCD_cmd(row_pos[i+1]);
		if(pointerLine==i+1){
		int v1 = fileNames[i].length();
		char ch_v1[v1+3];
		strcpy(ch_v1,(">>" + fileNames[i]).c_str()); 
		LCD_Print(ch_v1);
		}
		else{
			int v11 = fileNames[i].length();
			char ch_v11[v11+1];
			strcpy(ch_v11,(fileNames[i]).c_str());
			LCD_Print(ch_v11);
		}
	}
	
	while(PINC & (1<<4)) // waiting for back button
	{
		
		// going down on the list
		if (~PINC & (1<<5))
		{
			
			if (upCount < 2)
			upCount++;
			if (pointerLine >= fileCount) // to connect the bottom of the menu to the top
			{
				pointerLine = 0;
				x = 0;
			}
			pointerLine++;
			if (pointerLine > 3 && downCount == 0)
			x++;

			if (downCount > 0)
			downCount--;
			//update the gallery menu
			LCD_clear();
			LCD_cmd(0x80);
			LCD_Print(" --- Gallery --- ");
			displayIndex = 1;  // only used for identify the printing line
			for (int i=x;i<3+x;i++)
			{
				LCD_cmd(row_pos[displayIndex]);
				if (pointerLine == i+1){
					int v2 = fileNames[i].length();
					char ch_v2[v2+3];
					strcpy(ch_v2,(">>" + fileNames[i]).c_str());
					LCD_Print(ch_v2);
				}
				else{
					int v22 = fileNames[i].length();
					char ch_v22[v22+1];
					strcpy(ch_v22,(fileNames[i]).c_str());
					LCD_Print(ch_v22);
				}
				displayIndex++;
			}
			
			while (~PINC & (1<<5)); // to avoid auto pressing the button
		}

		// going up on the list
		if (~PINC & (1<<7))
		{
			
			
			if (downCount < 2)
			downCount++;
			if (upCount == 0) //&& pointerLine < fileCount - 3
			x--;
			if (upCount > 0)
			upCount--;
			if (pointerLine == 1) // to connect the bottom of the menu to the top
			{
				pointerLine = fileCount+1;
				x = fileCount - 3;
				upCount += 2;
			}
			pointerLine--;
			//update the gallery menu
			LCD_clear();
			LCD_cmd(0x80);
			LCD_Print(" --- Gallery --- ");
			displayIndex = 1;  // only used for identify the printing line
			for (int i=x;i<3+x;i++)
			{
				LCD_cmd(row_pos[displayIndex]);
				if (pointerLine == i+1){
					int v3 = fileNames[i].length();
					char ch_v3[v3+3];
					strcpy(ch_v3,(">>" + fileNames[i]).c_str());
					LCD_Print(ch_v3);
				}
				else{
					int v33 = fileNames[i].length();
					char ch_v33[v33+1];
					strcpy(ch_v33,(fileNames[i]).c_str());
					LCD_Print(ch_v33);
				}
				displayIndex++;
			}
			while (~PINC & (1<<7)); // to avoid auto pressing the button
		}

		// OK button for Gallery
		_delay_ms(300);
		if (~PINC & (1<<6))
		{
			updateOptions();
			while (~PINC & (1<<6)); // to avoid pressing the back button multiple times
			// gallery control
			while(PINC & (1<<4)) // waiting for back button
			{
				if (~PINC & (1<<5))
				{
					opMenu++;
					updateOptions();
					_delay_ms(100);
					while (~PINC & (1<<5));
				}
				if (~PINC & (1<<7))
				{
					opMenu--;
					updateOptions();
					delay(100);
					while (~PINC & (1<<7));
				}
				//Select button for Options Menu
				if (~PINC & (1<<6))
				{
					
					while (~PINC & (1<<6));  // to avoid pressing the back button multiple times
					opAction(fileNames[pointerLine-1]);  // pass the selected file name as the variable
					if (opMenu == 3)
					{break;
						} // stop updating options menu
					updateOptions();
					
				}
			}
			if (opMenu == 3)
			{Serial.println("Going to Main menu after deleting");break;} //to reupdate the gallery after deleting a file
			
			while (~PINC & (1<<4)); // to avoid pressing the back button multiple times
			Serial.println("Back from options");
			opMenu = 1; //reset the options menu
			//update the gallery menu after pressing back
			LCD_clear();
			LCD_cmd(0x80);
			LCD_Print(" --- Gallery --- ");
			displayIndex = 1;  // only used for identify the printing line
			for (int i=x;i<3+x;i++)
			{
				LCD_cmd(row_pos[displayIndex]);
				if (pointerLine == i+1){
					int v4 = fileNames[i].length();
					char ch_v4[v4+3];
					strcpy(ch_v4,(">>" + fileNames[i]).c_str());
					LCD_Print(ch_v4);
				}
				else{
					int v44 = fileNames[i].length();
					char ch_v44[v44+1];
					strcpy(ch_v44,(fileNames[i]).c_str());
					LCD_Print(ch_v44);
				}
				displayIndex++;
			}
			while (~PINC & (1<<6)); // to avoid auto pressing the button
		}
		
		//End of waiting block..
	}
}


void action3()
{
	
	LCD_clear();
	LCD_cmd(0x80);
	LCD_Print("> About Device <");
	LCD_cmd(0xC0);
	LCD_Print(" Voice Recorder");
	LCD_cmd(0x90);
	LCD_Print("      v1.0      ");
	LCD_cmd(0xD0);
	LCD_Print(" We are Group 21");
	
	while(PINC & (1<<4));
	
}

//effects code---------------------------------------------------------------
void openfile(String filename){
  data_file = SD.open(filename,FILE_READ);
  data_file.seek(60); //pass the header
}

void start_play_withfilter(String filename){
 openfile(filename);
 noInterrupts();// disable interrupts
 TCCR0B = (1<<WGM02); // CTC Mode
 TCCR0B = (1 << CS01); // Set preScaler to divide by 8
 TIMSK0 = (1 << OCIE0A); // Call ISR when TCNT0 = OCRA0
 OCR0A = 150; // set sample play rate to 8000Hz_124
 interrupts(); // Enable interrupts to generate waveform!
 
}

ISR(TIMER0_COMPA_vect) { // Called when TCNT0 == OCR0A
  if(data_file.available()){
    int v = (int)data_file.read();
    PORTA = 0.3*preval + 0.3*(v);//write data to PORTA using leaky integrator
    preval = v;
  }else{
  PORTA=0;
  data_file.close();
  TIMSK0=0;
  }
    TCNT0 = 0;//set counter again to zero
}

void pitch_shift(){ // function for pitch shifting
  switch(pitch_factor){
    case 4 : ocr1a_val=250; break;
    case 3 : ocr1a_val=200; break;
    case 0 : ocr1a_val=170; break;
    case 1 : ocr1a_val=100; break;
    case 2 : ocr1a_val=90; break; 
  }
}

void start_play_pitch(String filename,byte factor){
 pitch_factor=factor;
 openfile(filename);
 pitch_shift();//take the pitch factor
 noInterrupts();// disable interrupts
 TCCR1B = (1<<WGM12); // CTC Mode
 TCCR1B = (1 << CS11); // Set preScaler to divide by 8
 TIMSK1 = (1 << OCIE1A); // Call ISR when TCNT1 = OCRA1
 OCR1A = ocr1a_val; // set sample play rate to 8000Hz_124
 interrupts(); // Enable interrupts to generate waveform!
 
}

ISR(TIMER1_COMPA_vect) { // Called when TCNT1 == OCR1A
  if(data_file.available()){
    PORTA= data_file.read();//write data to PORTA
  }else{
  PORTA=0;
  data_file.close();
  TIMSK1=0;
  }
    TCNT1 = 0;//set counter again to zero
}

//Code related to LCD----------------------------------------------------
void init_LCD(void){
	DDRL = 0b11111111;
	_delay_ms(20);
	LCD_cmd(0x02); //initializing 4 bit mode of 20x4 LCD
	LCD_cmd(0x28); 
	LCD_cmd(0x0c); 
	LCD_cmd(0x06); // make increment in cursor
	LCD_cmd(0x01); // clear LCD
	_delay_ms(2);
	//initialization complete
}

//function to send commands
void LCD_cmd(unsigned char cmd){
	LCD_DATA = (LCD_DATA & 0x0F) | (cmd & 0xF0); //last 4 bits
	LCD_DATA &= ~ (1<<rs); //set for commands
	LCD_DATA |= (1<<en);
	_delay_us(1);
	LCD_DATA &= ~ (1<<en);
	_delay_us(200);
	LCD_DATA = (LCD_DATA & 0x0F) | (cmd << 4); // first 4 bits of the cmd
	LCD_DATA |= (1<<en);
	_delay_us(1);
	LCD_DATA &= ~ (1<<en);
	_delay_ms(2);
	return;

}

//function to send data
void LCD_write(unsigned char data){
	LCD_DATA = (LCD_DATA & 0x0F) | (data & 0xF0); //last 4 bits
	LCD_DATA |= (1<<rs); //set for data
	LCD_DATA |= (1<<en);
	_delay_us(1);
	LCD_DATA &= ~ (1<<en);
	_delay_us(200);
	LCD_DATA = (LCD_DATA & 0x0F) | (data << 4); // first 4 bits of the cmd
	LCD_DATA |= (1<<en);
	_delay_us(1);
	LCD_DATA &= ~ (1<<en);
	_delay_ms(2);
	return;
}

void LCD_clear(void){
	LCD_cmd (0x01);		//Clear LCD
	_delay_ms(2);			//Wait to clean LCD
	LCD_cmd (0x80);		//Move to Position Line 1, Position 1

}

//print the string on display
void LCD_Print (char *str)
{
	int i;
	for(i=0; str[i]!=0; i++)// loop to print the string
	{
		LCD_DATA = (LCD_DATA & 0x0F) | (str[i] & 0xF0); //last 4 bits
		LCD_DATA |= (1<<rs);
		LCD_DATA|= (1<<en);
		_delay_us(1);
		LCD_DATA &= ~ (1<<en);
		_delay_us(200);
		LCD_DATA = (LCD_DATA & 0x0F) | (str[i] << 4); //first 4 bits
		LCD_DATA |= (1<<en);
		_delay_us(1);
		LCD_DATA &= ~ (1<<en);
		_delay_ms(2);
	}
}

//Write on a specific location
void LCD_Printpos (char row, char pos, char *str)
{
	if (row == 0 && pos<16)
	LCD_cmd((pos & 0x0F)|0x80);
	else if (row == 1 && pos<16)
	LCD_cmd((pos & 0x0F)|0xC0);
	else if (row==2 && pos<16)
	LCD_cmd((pos&0x0F)|0x94);
	else if(row==3 && pos<16)
	LCD_cmd((pos & 0x0F)|0xD4);
	LCD_Print(str);

}


//for the recording and saving----------------------------------------------------------



//creating_WAV_file_format

void create_wav_format(String F_name)
{
	uint32_t Samples=0; //initialize sample count
	file = SD.open(F_name,FILE_WRITE); //opening the file in write mode

	//First write the header
	for(uint8_t pos=0; pos<44; pos++){
		file.write(Header[pos]);
	}
	//record from ADC
	ADC_Setup();
	ADC_run_timer();
	_delay_ms(200);//to stop double press
	while((PINC & (1<<6))){ //& digitalRead(selectButton)
		_delay_us(1);
		if(stat){
			Samples += 1;
			do_conversion();
			stat=0;
			file.write(ADC_out);
		}
	}
	stop_ADC();
	
	file.close();
	//completing the header
	file = SD.open(F_name,O_RDWR);
	file.seek(40);// go to data part size position

	for(uint8_t pos=0; pos<4; pos++){
		file.write( ( Samples>>(8*pos) ) & 0xFF);
	}

	file.seek(4); //wave file size position

	for(uint8_t pos=0; pos<4; pos++){
		file.write( ( (Samples+44) >>(8*pos) ) & 0xFF);
	}

	file.close();
	Samples =0;
}

void ADC_Setup(void){
	noInterrupts();
	ADMUX = 0b01100000; //REF-->AVCC,Left adjusted, A0
	ADCSRA = 0b10000110; //Enable, PreScale-->64-->250kHz

	interrupts();
	
}

void ADC_run_timer(void){
	noInterrupts();// disable interrupts
	TCCR2B = (1<<WGM21); // CTC Mode
	TCCR2B = (1 << CS21); // Set PreScaler to divide by 8
	TIMSK2 = (1 << OCIE2A); // Call ISR when TCNT2 = OCRA2
	OCR2A = 240; // set sample taking rate to 8000Hz
	interrupts(); // Enable interrupts
}

ISR(TIMER2_COMPA_vect){
	stat=1;//set ADC to get the sample
	TCNT2 = 0;
}

void stop_ADC(void){
	noInterrupts();// disable interrupts
	TIMSK2 = 0x00;
	ADCSRA = 0x00;
}

void do_conversion(void){
	noInterrupts();
	ADCSRA = ADCSRA | (1<<ADSC); //set single conversion
	//polling
	while(ADCSRA & (1 << ADSC)); //wait till the conversion is done
	interrupts();
	ADC_out = ADCH; //output read from data register

	ADCSRA |= 1 << ADIF; //clearing the ADC Interrupt Flag
}
