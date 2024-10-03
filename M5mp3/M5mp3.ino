#include <SPI.h>
#include <FS.h>
#include <Wire.h>
#include <SD.h>
#include "M5Cardputer.h"
#include "Audio.h"
#include "font.h"
#include <ESP32Time.h>  // https://github.com/fbiego/ESP32Time  verison 2.0.6
M5Canvas sprite(&M5Cardputer.Display);
M5Canvas spr(&M5Cardputer.Display);

// microSD card
#define SD_SCK 40
#define SD_MISO 39
#define SD_MOSI 14
#define SD_CS 12

// I2S
#define I2S_DOUT 42
#define I2S_BCLK 41
#define I2S_LRCK 43

#define AUDIO_FILENAME_01 "/song.mp3"
Audio audio;


unsigned short grays[18];
unsigned short gray;
unsigned short light;
int n = 0;
int m=0;
int volume=10;
int bri=0;
int brightness[5]={50,100,150,200,250};
bool isPlaying = true;
bool stoped=false;
bool nextS=0;
bool volUp=0;
int g[14]={0}; int graphSpeed=0;
int textPos=60;
int sliderPos=0;


// Task handle for audio task
TaskHandle_t handleAudioTask = NULL;
ESP32Time rtc(0);

#define MAX_FILES 100

// Array to store file names
String audioFiles[MAX_FILES];
int fileCount = 0;

void resetClock()
{
   rtc.setTime(0, 0, 0, 17, 1, 2021);
}

void setup() {
    Serial.begin(115200);
    Serial.println("hi");
    resetClock();

    // Initialize M5Cardputer and SD card
    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setBrightness(brightness[bri]);
    sprite.createSprite(240,135);
    spr.createSprite(86,16);

    SPI.begin(SD_SCK, SD_MISO, SD_MOSI);
    if (!SD.begin(SD_CS)) {
        Serial.println(F("ERROR: SD Mount Failed!"));
    }
    listFiles(SD, "/", MAX_FILES);

    // Initialize audio output
    audio.setPinout(I2S_BCLK, I2S_LRCK, I2S_DOUT);
    audio.setVolume(volume); // 0...21
    audio.connecttoFS(SD,audioFiles[n].c_str()); 

    int co = 214;
    for (int i = 0; i < 18; i++) {
    grays[i] = M5Cardputer.Display.color565(co, co, co+40);
    co = co - 13;
    }

    // Create tasks and pin them to different cores
    xTaskCreatePinnedToCore(Task_TFT, "Task_TFT", 20480, NULL, 2, NULL, 0);            // Core 0
    xTaskCreatePinnedToCore(Task_Audio, "Task_Audio", 10240, NULL, 3, &handleAudioTask, 1); // Core 1
}

void loop() {
    
}

void draw()
{
        if(graphSpeed==0) {
        gray=grays[15];
        light=grays[11];
        sprite.fillRect(0,0,240,135,gray);
        sprite.fillRect(4,8,130,122,BLACK);

        sprite.fillRect(129,8,5,122,0x0841);

        sliderPos=map(n,0,fileCount,8,110);
        sprite.fillRect(129,sliderPos,5,20,grays[2]);
        sprite.fillRect(131,sliderPos+4,1,12,grays[16]);

        sprite.fillRect(4,2,50,2,ORANGE);
        sprite.fillRect(84,2,50,2,ORANGE);
        sprite.fillRect(190,2,45,2,ORANGE);
        sprite.fillRect(190,6,45,3,grays[4]);
        sprite.drawFastVLine(3,9,120,light);
        sprite.drawFastVLine(134,9,120,light);
        sprite.drawFastHLine(3,129,130,light);
       
        sprite.drawFastHLine(0,0,240,light);
        sprite.drawFastHLine(0,134,240,light);

        sprite.fillRect(139,0,3,135,BLACK);
        sprite.fillRect(148,14,86,42,BLACK);
        sprite.fillRect(148,59,86,16,BLACK);

        sprite.fillTriangle(162,18,162,26,168,22,GREEN);
        sprite.fillRect(162,30,6,6,RED);
        
        sprite.drawFastVLine(143,0,135,light);
        
        sprite.drawFastVLine(238,0,135,light);
        sprite.drawFastVLine(138,0,135,light);
        sprite.drawFastVLine(148,14,42,light);
        sprite.drawFastHLine(148,14,86,light);

        //buttons
        for(int i=0;i<4;i++)
        sprite.fillRoundRect(148+(i*22),94,18,18,3,grays[4]);

        //button icons
        sprite.fillRect(220,104,8,2,grays[13]);
        sprite.fillRect(220,108,8,2,grays[13]);
        sprite.fillTriangle(228,102,228,106,231,105,grays[13]);
        sprite.fillTriangle(220,106,220,110,217,109,grays[13]);

        if(!stoped)
        {
          sprite.fillRect(152,104,3,6,grays[13]);
          sprite.fillRect(157,104,3,6,grays[13]);
        }
        else
        {
        sprite.fillTriangle(156,102,156,110,160,106,grays[13]);
        }
        
        //volume bar
        sprite.fillRoundRect(172,82,60,3,2,YELLOW);
        sprite.fillRoundRect(155+((volume/5)*17),80,10,8,2,grays[2]);
        sprite.fillRoundRect(157+((volume/5)*17),82,6,4,2,grays[10]);
       
        // brightness
        sprite.fillRoundRect(172,124,30,3,2,MAGENTA);
        sprite.fillRoundRect(172+(bri*5),122,10,8,2,grays[2]);
        sprite.fillRoundRect(174+(bri*5),124,6,4,2,grays[10]);

        //BATTERY
        sprite.drawRect(206,119,28,12,GREEN);
        sprite.fillRect(234,122,3,6,GREEN);

        //graph
        for(int i=0;i<14;i++){ 
        if(!stoped)  
        g[i]=random(1,5);
        for(int j=0;j<g[i];j++)
        sprite.fillRect(172+(i*4),50-j*3,3,2,grays[4]);
        }
        
        sprite.setTextFont(0);
        sprite.setTextDatum(0);


        if(n<5)
        for(int i=0;i<10;i++)
        {
        if(i==n) sprite.setTextColor(WHITE,BLACK); else  sprite.setTextColor(GREEN,BLACK); 
        if(i<fileCount)
        sprite.drawString(audioFiles[i].substring(1,20), 8, 10+(i*12));
        }

        int yos=0;
        if(n>=5)
        for(int i=n-5;i<n-5+10;i++)
        {
        if(i==n) sprite.setTextColor(WHITE,BLACK); else  sprite.setTextColor(GREEN,BLACK); 
        if(i<fileCount)
        sprite.drawString(audioFiles[i].substring(1,20), 8, 10+(yos*12));
        yos++;
        }

        sprite.setTextColor(grays[1],gray);
        sprite.drawString("WINAMP", 150, 4);
        sprite.setTextColor(grays[2],gray);
        sprite.drawString("LIST", 58, 0);
        sprite.setTextColor(grays[4],gray);
        sprite.drawString("VOL", 150, 80);
        sprite.drawString("LIG", 150, 122);
       
        if(isPlaying){
        sprite.setTextColor(grays[8],BLACK);
        sprite.drawString("P", 152,18);
        sprite.drawString("L", 152,27);
        sprite.drawString("A", 152,36);
        sprite.drawString("Y", 152,45);
        }else
        { sprite.setTextColor(grays[8],BLACK);
        sprite.drawString("S", 152,18);
        sprite.drawString("T", 152,27);
        sprite.drawString("O", 152,36);
        sprite.drawString("P", 152,45);}

        sprite.setTextColor(GREEN,BLACK); 
        sprite.setFreeFont(&DSEG7_Classic_Mini_Regular_16);
        if(!stoped)
        sprite.drawString(rtc.getTime().substring(3,8), 172, 18);
        sprite.setTextFont(0);

        int percent=0;
        if(analogRead(10)>2390)
        percent=100;
        else if(analogRead(10)<1400)
        percent=1;
        else
        percent=map(analogRead(10),1400,2390,1,100);
       
        sprite.setTextDatum(3);
        sprite.drawString(String(percent)+"%", 220, 121);

        sprite.setTextColor(BLACK,grays[4]);
        sprite.drawString("B",220,96); 
        sprite.drawString("N",198,96); 
        sprite.drawString("P",176,96); 
        sprite.drawString("A",154,96); 

        sprite.setTextColor(BLACK,grays[5]);
        sprite.drawString(">>",202,103); 
        sprite.drawString("<<",180,103); 
        
        spr.fillSprite(BLACK);
        spr.setTextColor(GREEN,BLACK);
        if(!stoped)  
        spr.drawString(audioFiles[n].substring(1,audioFiles[n].length()),textPos,4);
        textPos=textPos-2; if(textPos<-300) textPos=90;
        spr.pushSprite(&sprite,148,59);
        
        sprite.pushSprite(0,0);
        }
        graphSpeed++; if(graphSpeed==4) graphSpeed=0;
}

void Task_TFT(void *pvParameters) {
    while (1) {
        M5Cardputer.update();
        // Check for key press events
        if (M5Cardputer.Keyboard.isChange()) {
            if (M5Cardputer.Keyboard.isKeyPressed('a')) {
                isPlaying = !isPlaying;
                stoped=!stoped; } // Toggle the playback state

            if (M5Cardputer.Keyboard.isKeyPressed('v')) {
                isPlaying=false;
                volUp=true;
                volume=volume+5;
                if(volume>20) volume=5;} 

            if (M5Cardputer.Keyboard.isKeyPressed('l')) {
              bri++; if(bri==5) bri=0;
              M5Cardputer.Display.setBrightness(brightness[bri]);
            }
            
             if (M5Cardputer.Keyboard.isKeyPressed('n')) {
              resetClock();
              isPlaying=false;
              textPos=90;
              n++;
              if(n>=fileCount) n=0;
               nextS=1; } 

                 if (M5Cardputer.Keyboard.isKeyPressed('p')) {
              resetClock();
              isPlaying=false;
              textPos=90;
              n--;
              if(n<0) n=fileCount-1;
               nextS=1; } 

              if (M5Cardputer.Keyboard.isKeyPressed(';')) {
              n--;
              if(n>=fileCount)
               n=0;
              } 

              if (M5Cardputer.Keyboard.isKeyPressed('.')) {
              n++;
              if(n<0) 
              n=fileCount-1;
              } 

                if (M5Cardputer.Keyboard.isKeyPressed(KEY_ENTER)) {
              resetClock();
              stoped=false;
              isPlaying=false;
              textPos=90;
              nextS=1;
              } 



                 if (M5Cardputer.Keyboard.isKeyPressed('b')) {
              resetClock();
              isPlaying=false;
              textPos=90;
              n=random(0,fileCount);
              nextS=1; } 
        }
       draw();  
        vTaskDelay(40 / portTICK_PERIOD_MS);  // Adjust the delay for responsiveness
    }
}

void Task_Audio(void *pvParameters) {
    while (1) {
      
        if(volUp)
        {
          audio.setVolume(volume); 
          isPlaying=1;
          volUp=0;
        }

        if(nextS)
        {
           audio.stopSong();
           audio.connecttoFS(SD,audioFiles[n].c_str()); 
           isPlaying=1;
           nextS=0;
        }

        if (isPlaying) {
            while (isPlaying) {
                if(!stoped)
                audio.loop();  // This keeps the audio playback running
                vTaskDelay(10 / portTICK_PERIOD_MS);  // Add a small delay to prevent task hogging
            }
        } else {
            isPlaying=true;
        }
    }
}

// Function to play a song from a given URL or file path
void playSong(const char* source) {
  //audio.stopSong(); // Stop any current playback
  audio.connecttohost(source); // Open and play the new song from a URL
}

// Function to stop playback
void stopSong() {
   audio.stopSong(); 
}

// Function to open and prepare a song (without autoplay)
void openSong(const char* source) {
  audio.connecttohost(source); // Opens the source, similar to play
}

void listFiles(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file && fileCount < MAX_FILES) {
    if (file.isDirectory()) {
      Serial.print("DIR : ");
      Serial.println(file.name());
      if (levels) {
        listFiles(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("FILE: ");
      Serial.print(file.name());
      audioFiles[fileCount] = "/"+String(file.name());
      fileCount++;
      
    }
    file = root.openNextFile();
  }
}

void audio_eof_mp3(const char *info) {
    resetClock();
    Serial.print("eof_mp3     ");
    Serial.println(info);
    n++; if(n==fileCount) n=0;
    audio.connecttoFS(SD,audioFiles[n].c_str()); 
}
