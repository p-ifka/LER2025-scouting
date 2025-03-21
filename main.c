#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
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
  int scores[64]; // list of int identifiers for score actions 
  int sPos; // the next free pos of scores[64] 
  int conditions[16]; // list of int identifiers for conditions
  int cPos; // next free pos of conditions[16]
};
struct GameInfo ginfo;
bool isauto; //whether added actions will be counted as done in autonomous

  
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
  if(sizeof(buttons) > 0) {
    for(i=0;i<sizeof(buttons)/sizeof(buttons[0]);i++) {
      struct Button b = buttons[i];
      if((x >= b.fromX && x <= b.toX) && (y >= b.fromY && y <= b.toY)) {
	lastbid = b.id;
	lastbt = 15;
	b.onClicked(b.id);
	return;
      }
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
  int i;
  ginfo.teamNumber = 0;
  ginfo.matchNumber = 0;
  for(i=0;i<sizeof(ginfo.scores)/sizeof(ginfo.scores[0]);i++) {
    ginfo.scores[i] = -1;
  }
  ginfo.sPos = 0;
  for(i=0;i<sizeof(ginfo.conditions)/sizeof(ginfo.conditions[0]);i++) {
    ginfo.conditions[i] = -1;
  }
  ginfo.cPos = 0;
}

void addScore(int type) {
  ginfo.scores[ginfo.sPos] = type+isauto;
  ++ginfo.sPos;
}

void undoAddScore() {
  ginfo.scores[ginfo.sPos-1] = -1;
  --ginfo.sPos;
}

void addCondition(int type) {
  ginfo.conditions[ginfo.cPos] = type;
  ++ginfo.cPos;
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


void btnClicked(int id) {
  if(id == 'A') {
    isauto = !isauto;
  } else if (id == 'u') {
    undoAddScore();
  } else if (id == '1') { // L1
    addScore(1); // L1 occupies score ids 1 and 2, 1=not auto, 2=auto
  } else if (id == '2') {
    addScore(3); // L2 occupies score ids 3 and 4 ...
  } else if (id == '3') {
    addScore(5); // etc...
  } else if (id == '4') {
    addScore(7);
  } else if (id == 'a') {
    addScore(9);
  } else if (id == 'p') {
    addScore(11);
  } else if (id == 'n') {
    addScore(13);
  }
	      
}

int getButton(int id) {
  for(int i=0; i<buttonsN;i++) {
    if(buttons[i].id == id) {
      return i;
    }
  }
  return -1; 
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

void renderButton(struct Button b) {
  if(lastbid == b.id && lastbt > 0) {
    CNFGColor ( UI_SELECTED );
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

	CNFGSetupFullscreen( "Test Bench", 0 );

	
	/* initialize layouts */
	CNFGGetDimensions( &screenx, &screeny );
	vLayout = screeny/20;
	hLayout = screenx/6;

	createTextBox('T', 1, 1, 5, 1, 15, 15, "", 5); //team#  DONT change maxchars to 4
	createTextBox('M', 1, 2, 5, 1, 15, 15, "", 3); //match# 

	createButton('A', 0, 3, 3, 1, 5, 5, "auto:");
	createButton('u', 3, 3, 3, 1, 5, 5, "undo last");

	
	createButton('1', 0, 5, 2, 1, 5, 10, "L1");
	createButton('2', 2, 5, 2, 1, 5, 10, "L2");
	createButton('3', 4, 5, 2, 1, 5, 10, "L3");

	createButton('a', 0, 6, 2, 1, 5, 10, "rmv Algae");
	createButton('p', 2, 6, 2, 1, 5, 10, "Processor");
	createButton('n', 4, 6, 2, 1, 5, 10, "Net");
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


	  char* lastScore = scoreStr(ginfo.scores[ginfo.sPos-1]);
	  setPen(hLayout/15, vLayout* 8.5);
	  CNFGDrawText(lastScore, vLayout/15);

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
