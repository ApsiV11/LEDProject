//Kirjastojen sisällyttäminen
#include <fix_fft.h>

#include <FastLED.h>

//Vakioiden määrittäminen
#define LED_PIN1     3
#define LED_PIN2     6
#define LED_PIN3     8
#define LED_PIN4     11
#define LED_PIN5     13
#define MIC_PIN      A0

#define NUM_LEDS1    108
#define NUM_LEDS2    84
#define NUM_LEDS3    60
#define NUM_LEDS4    36
#define NUM_LEDS5    12
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS  32

//Lediarrayt
CRGB leds1[NUM_LEDS1];
CRGB leds2[NUM_LEDS2];
CRGB leds3[NUM_LEDS3];
CRGB leds4[NUM_LEDS4];
CRGB leds5[NUM_LEDS5];

//Ledien ohjaamiseen tarvittavia muuttujia
int lamount[5]={NUM_LEDS1, NUM_LEDS2, NUM_LEDS3, NUM_LEDS4, NUM_LEDS5};
int speeds[5]={-1, -1, -1, -1, -1};
int timer=0;
int deleted_colors[5]={-1, -1, -1, -1, -1};

//FFT-muunnoksen arrayt
char re[128], im[128];
int data[64];
int max_amps[2]={-1,-256};

//Määritellään väreille rakenne
struct color {
  int r;
  int g;
  int b;
};
typedef struct color Color;

//Väriarrayt
Color colors[5];
Color temp_colors[5];

//Funktioiden määrittelyt
void updateLeds(int u_colors);

void calculateSpeeds();

void calculateColors();

void getMaxAmps();

Color getColor(int m, int v, int ind);

void setColor(Color *c, int r, int g, int b);

//Setup

void setup() {
  //Lisätään ledit kirjastolle
  LEDS.addLeds<LED_TYPE, LED_PIN1, COLOR_ORDER>(leds1, NUM_LEDS1);
  LEDS.addLeds<LED_TYPE, LED_PIN2, COLOR_ORDER>(leds2, NUM_LEDS2);
  LEDS.addLeds<LED_TYPE, LED_PIN3, COLOR_ORDER>(leds3, NUM_LEDS3);
  LEDS.addLeds<LED_TYPE, LED_PIN4, COLOR_ORDER>(leds4, NUM_LEDS4);
  LEDS.addLeds<LED_TYPE, LED_PIN5, COLOR_ORDER>(leds5, NUM_LEDS5);

  FastLED.setBrightness(BRIGHTNESS);
  
  delay(1000);
}

//Itse ohjelma
void loop(){

  //Nollataan data
  max_amps[1]=-256;
  
  for(byte i=0; i<64; i++){
    data[i]=0;
  }

  //Kerätään 128 näytettä ääniaallosta
  for(byte i=0; i<128; i++){
    int sample = analogRead(A0);
    re[i]=(sample/4)-128;
    im[i]=0;
  }

  //Suoritetaan Fast Fourier Transformation-muutos
  fix_fft(re, im, 7, 0);

  //Lasketaan kuuluvien taajuuksien magnitudit
  for(byte i=0; i<64; i++){
    data[i]=data[i]+sqrt(re[i]*re[i]+im[i]*im[i]);
  }

  //Valitaan taajuus, jolla on suurin amplitudi
  data[0]=0;
  getMaxAmps();

  //Määritetään värit
  calculateColors();
  
  //Päivitetään väri vain, jos ledinauha on päivitetty loppuun
  for(int i=0;i<5;i++){
    if(speeds[i]<1){
      colors[i]=temp_colors[i];
    }
  }

  //Lasketaan kuinka monta lediä värjätään uusilla väreillä
  calculateSpeeds();

  //Lasketaan, kuinka monta ledinauhaa päivitetään värillä
  int u_colors=0;
  for(int i=0;i<5;i++){
    if(colors[i].r!=0 || colors[i].g!=0 || colors[i].b!=0){
      u_colors++;
    }
  }

  //Jos päivitettäviä nauhoja on liikaa, poistetaan värin päivitys joistain nauhoista.
  if(u_colors>2 && timer<0){
    timer=0;
    for(int i=0; i<5; i++){
      deleted_colors[i]=-1;
    }
    for(int u=u_colors-2;u>0;u--){
      while(1){
        int i=random(0,5);
        deleted_colors[i]=1;
        if(colors[i].r==0 && colors[i].g==0 && colors[i].b==0){
          continue;
        }
        else{
          colors[i].r=0;
          colors[i].g=0;
          colors[i].b=0;
          speeds[i]=round(pow(i+1,2));
          timer=timer+speeds[i];
          break;
        }
      }
    }
  }

  else if(u_colors>2){
    for(int i=0;i<5;i++){
      if(deleted_colors[i]==1){
        colors[i].r=0;
        colors[i].g=0;
        colors[i].b=0;
      }
    }
  }
  
  //Päivitetään ledit
  updateLeds(u_colors);

  FastLED.show();
  delay(10);
}

//Metodi ledien päivittämistä varten
void updateLeds(int u_colors){
  //Käydään läpi kaikki ledit ja asetetaan uusi väri
  for(int i=0;i<5;i++){
    int change=0;

    //Muutosnopeus riippuu päivitettävien värien määrästä
    if(u_colors>0){
      change=round((5-i)*1.5/u_colors);
    }

    //Muutosnopeus saa olla maksimissaan päivitettävien ledien määrä
    else if(change>speeds[i]){
      change=speeds[i];
    }

    if(change<=0){
      change=5-i;
    }

    //Päivitetään ledit
    
    //Tämän olisi voinut tehdä vain yhdellä silmukalla, jos oltaisiin käytetty kaksiulottiesta arrayta.
    //Tällöin Arduinon RAM-muisti ei kuitenkaan riittänyt, joten koodi piti tehdä näin.
    //Suorittamisnopeudessa ei pitäisi olla juurikaan eroa
    switch(i){
      case 0:

      //Silmukka ledinauhan lopusta alkuun päin
      for(int x=lamount[i]-1;x>-1;x--){

        //Jos indeksin ja muutosnopeuden erotus on suurempi kuin nolla
        //Tällöin ledin väri määräytyy erotuksen indeksissä olevan ledin väristä
        if(x-change>=0){
          leds1[x]=leds1[x-change];
        }

        //Muuten asetetaan uusi väri
        else{
          leds1[x].setRGB(colors[i].r, colors[i].g, colors[i].b);
        }
      }
      break;

      case 1:
      for(int x=lamount[i]-1;x>-1;x--){
        if(x-change>=0){
          leds2[x]=leds2[x-change];
        }
        else{
          leds2[x].setRGB(colors[i].r, colors[i].g, colors[i].b);
        }
      }
      break;

      case 2:
      for(int x=lamount[i]-1;x>-1;x--){
        if(x-change>=0){
          leds3[x]=leds3[x-change];
        }
        else{
          leds3[x].setRGB(colors[i].r, colors[i].g, colors[i].b);
        }
      }
      break;

      case 3:
      for(int x=lamount[i]-1;x>-1;x--){
        if(x-change>=0){
          leds4[x]=leds4[x-change];
        }
        else{
          leds4[x].setRGB(colors[i].r, colors[i].g, colors[i].b);
        }
      }
      break;

      case 4:
      for(int x=lamount[i]-1;x>-1;x--){
        if(x-change>=0){
          leds5[x]=leds5[x-change];
        }
        else{
          leds5[x].setRGB(colors[i].r, colors[i].g, colors[i].b);
        }
      }
      break;
    }

    speeds[i]=speeds[i]-change;
  }
  timer=timer-1;
}

//Metodi samanaikaisten päivitysten määrää varten
void calculateSpeeds(){
  for(int i=0;i<5;i++){
    if(max_amps[1]>(8-i) && speeds[i]<1){
      speeds[i]=round(10-i*2+max_amps[1]);
    }
  }
}

//Ledin värin laskemista varten metodi
void calculateColors(){
  for(int i=0;i<5;i++){
    Color c;
    //Jos taajuuden amplitudi on tarpeeksi suuri, niin lasketaan väri
    if(max_amps[0]>0 && max_amps[1]>(8-i)){
      temp_colors[i]=getColor(max_amps[0], max_amps[1], i+1);
    }

    //Muuten sammutetaan ledi
    else{
      setColor(&c, 0, 0, 0);
      temp_colors[i]=c;
    }
  }
}

//Apumetodit alkaa

//Metodi vallitsevan taajuuden määrittämiseksi
void getMaxAmps(){
  for(byte i=0; i<64; i++){
    if(data[i]>5 && data[i]>max_amps[1]){
      max_amps[0]=i;
      max_amps[1]=data[i];
    } 
  }
}

//Funktio ledin värin asettamista varten
Color getColor(int m, int v, int ind){
  int r, g, b;
  Color c;

  //Vaihe vaihtuu, joka 30 sekunti
  //Näin saadaan vaihtelevuutta väreihin
  int phase=round(floor(millis()/30000))%5;
  int i=round((m+v)-phase*ind)%64;
  if(i<9){
    r=255;
    g=0;
    b=28*i+random(0,32);
  }
  else if(i<18){
    r=255-(i-9)*28-random(0,32);
    g=0;
    b=255;
  }
  else if(i<27){
    r=0;
    g=(i-18)*28+random(0,32);
    b=255;
  }
  else if(i<37){
    r=255;
    g=255;
    b=255;
  }
  else if(i<46){
    r=0;
    g=255;
    b=255-(i-37)*28-random(0,32);
  }
  else if(i<55){
    r=(i-46)*28+random(0,32);
    g=255;
    b=0;
  }
  else if(i<64){
    r=255;
    g=255-(i-55)*28-random(0,32);
    b=0;
  }
  setColor(&c, r, g, b);
  
  return c;
}

//Asetetaan väri muuttujiin
void setColor(Color *c, int r, int g, int b) {
  c->r = r;
  c->g = g;
  c->b = b;
}

//Apufunktiot loppuu
