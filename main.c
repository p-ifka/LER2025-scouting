// standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <strings.h>

// stuff for UDP
#include <unistd.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// stuff for android
#include "os_generic.h"
#include <GLES3/gl3.h>
#include <asset_manager.h>
#include <asset_manager_jni.h>
#include <android_native_app_glue.h>
#include <android/log.h>
#include <android/sensor.h>
#include "CNFGAndroid.h"


#define CNFG3D
#define CNFG_IMPLEMENTATION
#include "CNFG.h"

unsigned frames = 0;
unsigned long iframeno = 0;

volatile int suspended;

short screenx, screeny;
int lastbuttonx = 0;
int lastbuttony = 0;
int lastbid = 0; // id of last button pressed
int lastbt = 0;  // last button pressed highlight timer

int lastmotionx = 0;
int lastmotiony = 0;
int lastmask = 0;

int lastkey;
int lastkeydown;



int vLayout;
int hLayout;

/* const int BACKGROUND_COLOR = 0x300000ff; */
/* const int FOREGROUND_COLOR = 0xffffffff; */
const long FOREGROUND_COLOR = 0xe4e4efff;
const long BACKGROUND_COLOR = 0x181818ff;

const long UI_NORMAL= 0x484848ff;
const long UI_SELECTED = 0x484848fff;

const long UI_TRUE_BG = 0x73c936ff;
const long UI_TRUE_FG = 0x8c36c9ff;
const long UI_FALSE_BG = 0xf43841ff;
const long UI_FALSE_FG = 0x0bc7beff;


void MakeNotification( const char * channelID, const char * channelName, const char * title, const char * message ) {
  static int id;
  id++;

  const struct JNINativeInterface * env = 0;
  const struct JNINativeInterface ** envptr = &env;
  const struct JNIInvokeInterface ** jniiptr = gapp->activity->vm;
  const struct JNIInvokeInterface * jnii = *jniiptr;

  jnii->AttachCurrentThread( jniiptr, &envptr, NULL);
  env = (*envptr);

  jstring channelIDStr = env->NewStringUTF( ENVCALL channelID );
  jstring channelNameStr = env->NewStringUTF( ENVCALL channelName );

  // Runs getSystemService(Context.NOTIFICATION_SERVICE).
  jclass NotificationManagerClass = env->FindClass( ENVCALL "android/app/NotificationManager" );
  jclass activityClass = env->GetObjectClass( ENVCALL gapp->activity->clazz );
  jmethodID MethodGetSystemService = env->GetMethodID( ENVCALL activityClass, "getSystemService", "(Ljava/lang/String;)Ljava/lang/Object;");
  jstring notificationServiceName = env->NewStringUTF( ENVCALL "notification" );
  jobject notificationServiceObj = env->CallObjectMethod( ENVCALL gapp->activity->clazz, MethodGetSystemService, notificationServiceName);

  // create the Notification channel.
  jclass notificationChannelClass = env->FindClass( ENVCALL "android/app/NotificationChannel" );
  jmethodID notificationChannelConstructorID = env->GetMethodID( ENVCALL notificationChannelClass, "<init>", "(Ljava/lang/String;Ljava/lang/CharSequence;I)V" );
  jobject notificationChannelObj = env->NewObject( ENVCALL notificationChannelClass, notificationChannelConstructorID, channelIDStr, channelNameStr, 3 ); // IMPORTANCE_DEFAULT
  jmethodID createNotificationChannelID = env->GetMethodID( ENVCALL NotificationManagerClass, "createNotificationChannel", "(Landroid/app/NotificationChannel;)V" );
  env->CallVoidMethod( ENVCALL notificationServiceObj, createNotificationChannelID, notificationChannelObj );

  env->DeleteLocalRef( ENVCALL channelNameStr );
  env->DeleteLocalRef( ENVCALL notificationChannelObj );

  // Create the Notification builder.
  jclass classBuilder = env->FindClass( ENVCALL "android/app/Notification$Builder" );
  jstring titleStr = env->NewStringUTF( ENVCALL title );
  jstring messageStr = env->NewStringUTF( ENVCALL message );
  jmethodID eventConstructor = env->GetMethodID( ENVCALL classBuilder, "<init>", "(Landroid/content/Context;Ljava/lang/String;)V" );
  jobject eventObj = env->NewObject( ENVCALL classBuilder, eventConstructor, gapp->activity->clazz, channelIDStr );
  jmethodID setContentTitleID = env->GetMethodID( ENVCALL classBuilder, "setContentTitle", "(Ljava/lang/CharSequence;)Landroid/app/Notification$Builder;" );
  jmethodID setContentTextID = env->GetMethodID( ENVCALL classBuilder, "setContentText", "(Ljava/lang/CharSequence;)Landroid/app/Notification$Builder;" );
  jmethodID setSmallIconID = env->GetMethodID( ENVCALL classBuilder, "setSmallIcon", "(I)Landroid/app/Notification$Builder;" );

  // You could do things like setPriority, or setContentIntent if you want it to do something when you click it.

  env->CallObjectMethod( ENVCALL eventObj, setContentTitleID, titleStr );
  env->CallObjectMethod( ENVCALL eventObj, setContentTextID, messageStr );
  env->CallObjectMethod( ENVCALL eventObj, setSmallIconID, 17301504 ); // R.drawable.alert_dark_frame

  // eventObj.build()
  jmethodID buildID = env->GetMethodID( ENVCALL classBuilder, "build", "()Landroid/app/Notification;" );
  jobject notification = env->CallObjectMethod( ENVCALL eventObj, buildID );

  // NotificationManager.notify(...)
  jmethodID notifyID = env->GetMethodID( ENVCALL NotificationManagerClass, "notify", "(ILandroid/app/Notification;)V" );
  env->CallVoidMethod( ENVCALL notificationServiceObj, notifyID, id, notification );

  env->DeleteLocalRef( ENVCALL notification );
  env->DeleteLocalRef( ENVCALL titleStr );
  env->DeleteLocalRef( ENVCALL activityClass );
  env->DeleteLocalRef( ENVCALL messageStr );
  env->DeleteLocalRef( ENVCALL channelIDStr );
  env->DeleteLocalRef( ENVCALL NotificationManagerClass );
  env->DeleteLocalRef( ENVCALL notificationServiceObj );
  env->DeleteLocalRef( ENVCALL notificationServiceName );
}
struct CheckBox {
  int id;
  int fromX;
  int fromY;
  int toX;
  int toY;
  char* label;
  int labelSize;
  bool value;
};
struct Button {
  int id;
  int fromX;
  int fromY;
  int toX;
  int toY;
  char* label;
  int labelSize;
  void (*onClicked)(int);
};
struct TextBox {
  int id;
  int fromX;
  int fromY;
  int toX;
  int toY;
  char* label;
  int labelSize; // font size ( proportional to screen size l/labelSize)
  int maxChars;
};
struct GameInfo {
  int teamNumber; // number of the team
  int matchNumber; // match being recorded
  int autoType;
  int autoAmount;
  int climbType;
  int inf[9]; // {brokedown, won, l1, l2, l3, l4, algae, processor, net}
};
struct GameInfo ginfo;
bool isauto; //whether added actions will be counted as done in autonomous
int gsData[2048];
int games = 0;

struct CheckBox checkBoxes[32];
int checkBoxesN = 0;

struct Button buttons[32];
int buttonsN = 0;

struct TextBox textBoxes[32];
int focusedTextBox = -1; // index in textBoxes of the currently focused textbox, assigned to -1 when no box is focused
int textBoxesN = 0;

char* strappend(const char *orig, char c) {
  size_t sz = strlen(orig);
  char *str = malloc(sz + 2);
  strcpy(str, orig);
  str[sz] = c;
  str[sz + 1] = '\0';
  return str;
}

char* strbackspace(const char *orig) {
  size_t sz = strlen(orig);
  if(sz==0) {return "";}; // this function works fine without this and i have no idea why
  char *str = malloc(sz);
  strcpy(str, orig);
  str[sz-1] = '\0';
  return str;
}


/*called every time a key is pressed or released*/
void HandleKey( int keycode, int bDown ) {
  if(bDown == 1) {
    if(keycode == 67) { //backspace
      char* new = strbackspace(textBoxes[focusedTextBox].label);
      textBoxes[focusedTextBox].label = new;
    } else if(keycode >= 48 && keycode <= 57 ) { // 0-9
      if(textBoxes[focusedTextBox].maxChars > strlen(textBoxes[focusedTextBox].label)) {
	char num[6];
	sprintf(num, "%d", keycode-48);
	char* new = strappend(textBoxes[focusedTextBox].label, num[0]);
	textBoxes[focusedTextBox].label = new;
      }

    }
  }

  lastkey = keycode;
  lastkeydown = bDown;
  if( keycode == 4 ) { AndroidSendToBack( 1 ); focusedTextBox = 0;}
}
void HandleButton( int x, int y, int button, int bDown ) {
  /**
   * run every time the user presses the touchscreen
   **/
  if(bDown == 1) {
    int i;
    for(i=0;i<checkBoxesN;i++) {
      struct CheckBox c = checkBoxes[i];
      if((x >= c.fromX && x <= c.toX) && (y >= c.fromY && y <= c.toY)) {
	lastbid = c.id;

	checkBoxes[i].value = !c.value;
      }
    }
    for(i=0;i<sizeof(buttons)/sizeof(buttons[0]);i++) {
      struct Button b = buttons[i];
      if((x >= b.fromX && x <= b.toX) && (y >= b.fromY && y <= b.toY)) {
	lastbid = b.id;
	lastbt = 15;
	b.onClicked(b.id);
	return;
      }
    }
    for(i=0;i<textBoxesN;i++) {
      struct TextBox t = textBoxes[i];
      if((x >= t.fromX && x <= t.toX) && (y >= t.fromY && y <= t.toY)) {
	lastbid = 2;
	focusedTextBox = i;
	AndroidDisplayKeyboard(1);
	return;
      }
    }
    focusedTextBox = -1;
    AndroidDisplayKeyboard(0);

    lastbid = 0;
    lastbuttonx = x;
    lastbuttony = y;

  }
}

void HandleMotion( int x, int y, int mask )
{
  lastmask = mask;
  lastmotionx = x;
  lastmotiony = y;
}

extern struct android_app * gapp;

static int keyboard_up;
void AndroidDisplayKeyboard(int pShow);

int HandleDestroy() { return 0; }

void HandleSuspend() { suspended = 1; }

void HandleResume() { suspended = 0; }

void setPen(int x, int y) {
  CNFGPenX = x;
  CNFGPenY = y;
}

void initGinfo() {
  ginfo.teamNumber = 0;
  ginfo.matchNumber = 0;
  ginfo.autoType = 0;
  ginfo.autoAmount = 0;
  ginfo.climbType = 0;
  for(int i=0;i<sizeof(ginfo.inf)/sizeof(ginfo.inf[0]);i++) {
    ginfo.inf[i] = 0;
  }
}



/* int generateOutput(int u) { */
/*   if(ginfo.teamNumber == 0 || ginfo.matchNumber == 0) { */
/*     return -1; */
/*   } */
/*   int size = 3 + ginfo.iPos; */
/*   int o[size]; */
/*   o[0] = size; */
/*   o[1] = ginfo.matchNumber; */
/*   o[2] = ginfo.teamNumber; */
/*   for(int i = 0;i<ginfo.iPos;i++) { */
/*     o[i+3] = ginfo.inf[i]; */
/*   } */
/*   return o; */
/* } */
char* generateOutput() {
  /**
   * generate output buffer for the current match stored in ginfo, write it to the next
   * avaliable position of gsData
   **/
  struct GameInfo inf = ginfo; // assign ginfo to local variable
  if(inf.teamNumber <= 0 || inf.matchNumber <= 0) { // check validity of match data
    return "team/matchnumber invalid";
  }
  /* create match data array */
  int size = 14;
  int o[size];
  int i;
  o[0] = inf.matchNumber;
  o[1] = inf.teamNumber;
  o[2] = inf.autoType;
  o[3] = inf.autoAmount;
  o[4] = inf.climbType;
  for(i=0;i<sizeof(ginfo.inf)/sizeof(ginfo.inf[0]);i++) {
    o[5+i] = ginfo.inf[i];
  }

  /* get index to start at (first non--1 value)*/
  int start = 1;
  for(int i=0;i<sizeof(gsData)/sizeof(gsData[0]);i++) {
    if(gsData[i] != -1) {
      start = i+1;
    }
  }
  /* fill space of length 'size' with match data, starting at 'start' */
  for(int i=0;i<size;i++) {
    gsData[start+i] = o[i];
  }
  ++games;
  return "OK";
}

char* sendData(char* ip) {

  gsData[0] = games;
  int sz = 0;
  /* get size of data */
  for(int i=0;i<sizeof(gsData)/sizeof(gsData[0]);i++) {
    if(gsData[i] == -1) {
      sz = i;
      break;
    }
  }
  /* create msgBuffer with ^size */
  int msgBuffer[sz];
  for(int i=0;i<sz;i++) {
    msgBuffer[i] = -1;
  }
  for(int i=0;i<sz;i++) {
    msgBuffer[i] = gsData[i];
  }

  int sockfd;
  struct sockaddr_in servaddr;

  // create connection
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_addr.s_addr = inet_addr(ip);
  servaddr.sin_port = htons(5000);
  servaddr.sin_family = AF_INET;
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  // create UDP connection, this will store the server address for future use
  // (struct sockaddr*)NULL will refer to servaddr
  if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    return "ERR";
  }
  // send msgbuffer to address, recieve message back from server
  sendto(sockfd, msgBuffer, 1000, 0, (struct sockaddr*)NULL, sizeof(servaddr));
  /* recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)NULL, NULL); // second NULL refers to sizeof(servaddr) used above */

  close(sockfd);
  return "OK";

}


char* scoreStr(int sc) {
  char s[32];
  switch (sc) {
  case 1:
    return "L1";
    break;
  case 2:
    return "L1Auto";
    break;
  case 3:
    return "L2";
    break;
  case 4:
    return "L2Auto";
    break;
  case 5:
    return "L3";
    break;
  case 6:
    return "L3Auto";
    break;
  case 7:
    return "L4";
    break;
  case 8:
    return "L4Auto";
    break;
  case 9:
    return "rmv Algae";
    break;
  case 10:
    return "rmv AlgeaAuto";
    break;
  case 11:
    return "Processor";
    break;
  case 12:
    return "ProcessorAuto";
    break;
  case 13:
    return "Net";
    break;
  case 14:
    return "NetAuto";
    break;
  default:
    return "none";




  }
}

/* void centerText(char* text, int fontSize) { */
/*   int penX = screenx/2-(strlen(text)*fontSize*1.5); */
/*   char wrapc; */
/*   int wraps; */
/*   setPen(penX, screeny/2); */
/*   CNFGColor(FOREGROUND_COLOR); */
/*   CNFGDrawText(text, fontSize); */
/* } */

/* /\* void lineNum(int lines) { *\/ */
/*   int lineSize = screeny/lines; */
/*   int i; */
/*   int ys[lines]; */
/*   for(i=0;i<lines;i++) { */
/*     int l = i * lineSize; */
/*     setPen(0, l); */
/*     char num[3]; */
/*     sprintf(num, "%d", i); */
/*     CNFGDrawText(num, lineSize/15); */
/*   } */
/* } */


void createTextBox(
		   int id, // unique ID of the textbox
		   int xP, // start y position along 'vLayout'
		   int yP, // start x position along 'hLayout'
		   int sizeX, // number of 'hLayout' positions to stretch
		   int sizeY, // number of 'vLayout' positions to stretch
		   int marginX, // margin on each side of the rectangle in pixels
		   int marginY, // margin on the top and bottom of the rectangle in pixels
		   char* label, // initial label
		   int maxChars) // maximum # of characters that can be entered
{
  struct TextBox b;
  // get spot on hLayout at xP add margin
  b.fromX = (hLayout*xP)+marginX;
  b.toX = (hLayout*(xP+sizeX))-marginX;
  b.fromY = (vLayout*yP)+marginY;
  b.toY = (vLayout*(yP+sizeY))-marginY;

  b.id = id;
  b.label = label;
  b.labelSize = vLayout/10;
  b.maxChars = maxChars;

  textBoxes[textBoxesN] = b;
  ++textBoxesN;
}
void renderTextBox(struct TextBox t) {

  /* int fromX = (screenx/t.sizeX)*(t.iX/100); */
  /* int fromY = lines*t.line; */
  if(textBoxes[focusedTextBox].id == t.id) {
    CNFGColor(UI_SELECTED);
  } else {
    CNFGColor(UI_NORMAL);
  }

  CNFGTackRectangle(t.fromX, t.fromY, t.toX, t.toY);
  int xCenter = ((t.fromX + t.toX) / 2)-strlen(t.label)*t.labelSize*1.5;
  int yCenter = ((t.fromY + t.toY) / 2)-t.labelSize;
  setPen(xCenter, yCenter);
  CNFGColor( FOREGROUND_COLOR );
  CNFGDrawText(t.label, t.labelSize);
}

int getTextBox(int id) {
  for(int i=0;i<textBoxesN;i++) {
    if(textBoxes[i].id == id) {
      return i;
    }
  }
  return -1;
}

int getButton(int id) {
  for(int i=0; i<buttonsN;i++) {
    if(buttons[i].id == id) {
      return i;
    }
  }
  return -1;
}

int getCheckBox(int id) {
  for(int i=0; i<checkBoxesN;i++) {
    if(checkBoxes[i].id == id) {
      return i;
    }
  }
}

void btnClicked(int id) {
  if(id == 'A') {
    isauto = !isauto;
  } else if (id == 'u') {
    /* undoAddScore(); */
  } else if (id == '1') { // L1
    if(isauto) { ginfo.autoType = 1; isauto = false; return;}
    ++ginfo.inf[2];
  } else if (id == '2') {
    if(isauto) { ginfo.autoType = 2; isauto = false; return;}
    ++ginfo.inf[3];
  } else if (id == '3') {
    if(isauto) { ginfo.autoType = 3; isauto = false; return;}
    ++ginfo.inf[4];
  } else if (id == '4') {
    if(isauto) { ginfo.autoType = 4; isauto = false; return;}
    ++ginfo.inf[5];
  } else if (id == 'a') {
    ++ginfo.inf[6];
  } else if (id == 'p') {
    ++ginfo.inf[7];
  } else if (id == 'n') {
    ++ginfo.inf[8];
  } else if (id == 'g') {
    ginfo.inf[9] = 2708;
    ginfo.matchNumber = atoi(textBoxes[getTextBox('M')].label);
    ginfo.teamNumber = atoi(textBoxes[getTextBox('T')].label);
    ginfo.inf[0] = checkBoxes[getCheckBox('b')].value;
    ginfo.inf[1] = checkBoxes[getCheckBox('w')].value;
    if(checkBoxes[getCheckBox('d')].value == true) {
      ginfo.climbType = 2;
    } else if(checkBoxes[getCheckBox('s')].value == true) {
      ginfo.climbType = 1;
    } else {
      ginfo.climbType = 0;
    }
    buttons[getButton('g')].label = generateOutput();
  } else if(id == 'S') {
    if(textBoxes[getTextBox('i')].label != "") {
      sendData(textBoxes[getTextBox('i')].label);
    } else {
      /* sendData("192.168.200.79"); */
      /* sendData("10.106.42.116"); */
      sendData("192.168.1.112");
    }
  }

}
void createCheckBox(int id, int xP, int yP, int sizeX, int sizeY, int marginX, int marginY, char*label, bool defState) {
  struct CheckBox b;
  b.fromX = (hLayout*xP)+marginX;
  b.toX = (hLayout*(xP+sizeX))-marginX;

  b.fromY = (vLayout*yP)+marginY;
  b.toY = (vLayout*(yP+sizeY))-marginY;

  b.id = id;
  b.label = label;
  b.labelSize = vLayout/10;

  b.value = defState;

  checkBoxes[checkBoxesN] = b;
  ++checkBoxesN;
}


void createButton(int id, int xP, int yP, int sizeX, int sizeY, int marginX, int marginY, char* label) {
  struct Button b;
  b.fromX = (hLayout*xP)+marginX;
  b.toX = (hLayout*(xP+sizeX))-marginX;

  b.fromY = (vLayout*yP)+marginY;
  b.toY = (vLayout*(yP+sizeY))-marginY;

  b.id = id;
  b.label = label;
  b.labelSize = vLayout/10;
  b.onClicked = &btnClicked;

  buttons[buttonsN] = b;
  ++buttonsN;
}

void renderCheckBox(struct CheckBox b) {
  CNFGColor( UI_NORMAL );
  CNFGTackRectangle(b.fromX, b.fromY, b.toX, b.toY);
  int xCenter = ((b.fromX + b.toX) / 2)-strlen(b.label)*b.labelSize*1.5;
  int yCenter = ((b.fromY + b.toY) / 2)-b.labelSize;
  if(b.value) {
    CNFGColor( UI_TRUE_BG );
  } else {
    CNFGColor( UI_FALSE_BG );
  }
  CNFGTackRectangle(b.fromX, b.fromY, b.toX, b.fromY+15);

  setPen(xCenter, yCenter);
  CNFGColor( FOREGROUND_COLOR );
  CNFGDrawText(b.label, b.labelSize);
}

void renderButton(struct Button b) {
  if(lastbid == b.id && lastbt > 0) {
    CNFGColor( UI_SELECTED );
  } else {
    CNFGColor( UI_NORMAL );
  }

  CNFGTackRectangle(b.fromX, b.fromY, b.toX, b.toY);
  int xCenter = ((b.fromX + b.toX) / 2)-strlen(b.label)*b.labelSize*1.5;
  int yCenter = ((b.fromY + b.toY) / 2)-b.labelSize;
  setPen(xCenter, yCenter);
  CNFGColor( FOREGROUND_COLOR );
  CNFGDrawText(b.label, b.labelSize);
}


int main( int argc, char ** argv) {
  int i, x, y;
  double ThisTime;
  double LastFPSTime = OGGetAbsoluteTime();
  double LastFrameTime = OGGetAbsoluteTime();
  double SecToWait;

  initGinfo();
  for(int i=0;i<sizeof(gsData)/sizeof(gsData[0]);i++) {
    gsData[i] = -1;
  }

  CNFGSetupFullscreen( "Test Bench", 0 );


  /* initialize layouts */
  CNFGGetDimensions( &screenx, &screeny );
  vLayout = screeny/20;
  hLayout = screenx/6;

  createTextBox('T', 1, 1, 5, 1, 15, 15, "", 5); //team#  DONT change maxchars to 4
  createTextBox('M', 1, 2, 5, 1, 15, 15, "", 3); //match#
  createTextBox('m', 1, 3, 5, 1, 15, 15, "", 2); //auto scores#
  createTextBox('i', 1, 4, 5, 1, 15, 15, "", 30); //send ip

  createButton('A', 0, 6, 3, 1, 5, 5, "auto:");
  createButton('u', 3, 6, 3, 1, 5, 5, "undo last");


  createButton('1', 0, 7, 2, 1, 5, 10, "L1");
  createButton('2', 2, 7, 2, 1, 5, 10, "L2");
  createButton('3', 4, 7, 2, 1, 5, 10, "L3");
  createButton('4', 0, 8, 2, 1, 5, 10, "L4");

  createButton('a', 2, 8, 2, 1, 5, 10, "rmv Algae");
  createButton('p', 4, 8, 2, 1, 5, 10, "Processor");
  createButton('n', 2, 9, 2, 1, 5, 10, "Net");

  createCheckBox('b', 0, 10, 3, 1, 5, 10, "broke down", false);
  createCheckBox('w', 3, 10, 3, 1, 5, 10, "won", false);

  createCheckBox('P', 0, 11, 2, 1, 5, 10, "parked", false);
  createCheckBox('s', 2, 11, 2, 1, 5, 10, "shallow", false);
  createCheckBox('d', 4, 11, 2, 1, 5, 10, "deep", false);


  createButton('g', 0, 18, 6, 1, 5, 10, "save match");
  createButton('S', 0, 19, 6, 1, 5, 10, "upload data");
  while( 1 ) {
    int i, pos;
    float f;
    iframeno++;
    RDPoint pto[3];

    CNFGHandleInput();

    if( suspended ) { usleep(50000); continue; }

    CNFGClearFrame();
    CNFGGetDimensions( &screenx, &screeny );

    CNFGBGColor = BACKGROUND_COLOR;
    CNFGColor(FOREGROUND_COLOR);
    // -- render -- //

    // checkboxes
    for(i=0;i<checkBoxesN;i++) {
      renderCheckBox(checkBoxes[i]);
    }

    // textboxes
    for(i=0;i<textBoxesN;i++) {
      renderTextBox(textBoxes[i]);
    }

    // buttons
    for(i=0;i<buttonsN;i++) {
      renderButton(buttons[i]);
    }

    setPen(hLayout/15, vLayout*1.5);
    CNFGDrawText("team#", vLayout/15);
    setPen(hLayout/15, vLayout*2.5);
    CNFGDrawText("match#", vLayout/15);
    setPen(hLayout/15, vLayout*3.5);
    CNFGDrawText("auto\namount", vLayout/15);
    setPen(hLayout/15, vLayout*4.5);
    CNFGDrawText("ip", vLayout/15);



    /* char* lastScore = scoreStr(ginfo.inf[ginfo.iPos-1]); */
    /* setPen(hLayout/15, vLayout* 8.5); */
    /* CNFGDrawText(lastScore, vLayout/15); */

    char g[50];
    sprintf(g, "auto: %d, games %d, %d", ginfo.autoType, games, gsData[0]);
    setPen(hLayout/15, vLayout*12.5);
    CNFGDrawText(g, vLayout/15);


    char o[99];
    for(i=0;i<sizeof(gsData)/sizeof(gsData[0]);i++) {
      if(gsData[i] != -1) {
	char oo[5];
	sprintf(oo, "%d:%d ", i, gsData[i]);
	strcat(o, oo);
      }
    }
    setPen(hLayout/15, vLayout*15.5);
    CNFGDrawText(o, vLayout/15);

    vLayout = screeny/20;
    hLayout = screenx/3;


    char* isAutoLabel = "auto: true";
    if(isauto == false) { isAutoLabel = "auto: false"; }

    buttons[getButton('A')].label = isAutoLabel;


    /* CNFGColor( 0x303030ff ); */
    /* CNFGTackRectangle(10, 3 * l, screenx-10, 4*l); */
    /* struct Button testButton = Button{fromX:15, fromY:5*l, toX:screenx-15, toY:6*l, }; */
    /* struct Button tb; */
    /* tb.fromX=15; */
    /* tb.fromY=5*l; */
    /* tb.toX=screenx-15; */
    /* tb.toY=6*l; */
    /* tb.label="button text"; */
    /* tb.labelSize=l/15; */
    /* tb.onClicked = &onClickTest; */
    /* renderButton(tb); */
    /* buttons[0] = tb; */


    /* CNFGColor( 0xffffffff ); */
    /* setPen(15, (3.5 * l)-(l/15*1.5)); */
    /* CNFGColor( 0x123450ff ); */
    /* CNFGDrawText("button text", l/15); */

    /* centerText("centered text", 10); */
    frames++;
    if(lastbt > 0) {--lastbt;}
    CNFGSwapBuffers();

    ThisTime = OGGetAbsoluteTime();
    if( ThisTime > LastFPSTime + 1 ) {
      printf( "FPS: %d\n", frames );
      frames = 0;
      LastFPSTime+=1;
    }
  }

  return(0);
}
