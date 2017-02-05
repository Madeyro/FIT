/*
* Production line robot running on uCOS-II under FITkit
* Author: Maros Kopec    xkopec44@stud.fit.vutbr.cz
* Date: 16-12-2016
*
* This file cointains 15% of other authors work
* Based on:
* uC/OS-II pro platformu FITkit
* Autori: Karel Koranda     xkoran01@stud.fit.vutbr.cz
*         Vladimir Bruzek   xbruze01@stud.fit.vutbr.cz
*         Tomas Jilek       xjilek02@stud.fit.vutbr.cz
* Datum: 26-11-2010
*
*/


#include "includes.h"
#include <signal.h>

#include <fitkitlib.h>
#include <lcd/display.h>
#include <keyboard/keyboard.h>

#if FITKIT_VERSION == 1
#include <msp430x16x.h>
#elif FITKIT_VERSION == 2
#include <msp430x261x.h>
#include <stdio.h>
#include <stdbool.h>
#else
#error Bad and unsupported version of FITKit!
#endif

// clock set for FITkit
#define ACLK            BIT6
#define SMCLK           BIT5

#define OS_TASK_DEF_STK_SIZE 128
#define TASK_STK_SIZE OS_TASK_DEF_STK_SIZE    // size of stack for each task (byte)

#if FITKIT_VERSION == 2

OS_STK TaskStartStk[TASK_STK_SIZE];           // starting task
OS_STK TaskKeyStk[TASK_STK_SIZE];             // keyboard task
OS_STK TaskDisplayStk[TASK_STK_SIZE];         // display task
OS_STK TaskBeltsStk[TASK_STK_SIZE];           // feed belt task
OS_STK TaskControlStk[TASK_STK_SIZE];         // robot control

#endif

// functions needed because of fitkitlib
void fpga_initialized() {}

void print_user_help() {}

unsigned char decode_user_cmd(char *cmd_ucase, char *cmd)
{
  return(CMD_UNKNOWN);
}


/*
* Tasks definitions
*
* It is important to state for wich version is app written
*/
#if FITKIT_VERSION == 2

/*
* IDLE_STATUS           arm A is over Feed Belt FB
* A_OVER_L              arm A is over Press L
* A_OVER_DB             arm A is over Deposit belt DB
* A_PARAELLELLY_BELTS   arm A is parallelly with Feed belt FB and Deposit belt DB
*/
enum position {
  IDLE_STATUS = 0,
  A_OVER_L = 1,
  A_OVER_DB = 2,
  A_PARAELLELLY_BELTS = 3
};

/*
* 0 A hand over Feeding belt FB
* 1 A hand over Press L
* 2 A hand over Deposit belt DB
* 3 A hand parallelly with FB, DB
*/
int POS = IDLE_STATUS;                // position of arm

// generates with press of button '#'
bool IN = false;                      // sensor indicating the addition of new material

bool FREEA = true;                    // arm A is free
bool FREEB = true;                    // arm B is free

bool CCW = true;                      // rotate counterclock-wise - default

int STEPS = 0;                        // rotating by  x*90 degrees; allowed values = 0, 1, 2, 3, 4

bool CANGETFB = false;                // new amterial is at the end of Feed belt FB
bool CANPUTDB = true;                 // store finished amterial on Deposit belt
bool FREEPRESS = true;                // press is free
bool FINISHEDPRESS = false;           // finished product is in press
int DEPOSITED = 0;                    // stored finished products
bool TOIDLE = false;                  // after 5 s of not working go to idle status position
bool FIRSTSTART = true;               // robot have just been turned on
bool LIGHTA = false;                  // LED6 signaling arm A is occupied
bool LIGHTB = false;                  // LED5 signaling arm B is occupied

int beltSecs = 2;                     // frequency of material on FB; seconds part
int beltMilisecs = 500;               // frequency of material on FB; miliseconds part

/*
* Keyboard Task
*
* Signalize addition of the new material when pressed ked '#'.
*/
void keyboardTask(void *param) {
  param = param;
  char ch;

  keyboard_init(); // initialize keyboard

  for (;;)
  {
    // term_send_str_crlf("Keyboard task started");
    ch = key_decode(read_word_keyboard_4x4()); // decode key (character)
    switch (ch) {
      case '#': // key '#' pressed
        if (IN) { // invert IN value
          IN = false;
          term_send_str_crlf("Stopped generating new material; set IN = false");
          OSTaskResume(7); // resume belts task with priority 7
        }
        else {
          IN = true;
          term_send_str_crlf("Generating new material; set IN = true");
          OSTaskResume(7); // resume belts task with priority 7
        }
        break;
      default:
        break;
    }
   OSTimeDlyHMSM(0,0,0,100); // 100 mili sec delay
  }
}


/*
* Diplay task
*
*/
void displayTask(void *param) {
  char deposited[4] = "";
  LCD_init(); // initialize LCD

  for (;;) {

    LCD_clear();

    // first line
    LCD_append_string("POS:");
    delay_ms(2);
    LCD_append_char((unsigned char) (POS + '0'));
    delay_ms(2);
    LCD_append_string("     ");
    delay_ms(2);
    LCD_append_string("L:");
    delay_ms(2);
    if (FREEPRESS) {
      LCD_append_string("FREE");
      delay_ms(2);
    }
    else if (FINISHEDPRESS) {
      LCD_append_string("DONE");
      delay_ms(2);
    }
    else {
      LCD_append_string("WORK");
      delay_ms(2);
    }

    // second line
    LCD_append_string("IN:");
    delay_ms(2);
    if (IN) {
      LCD_append_char('1');
    }
    else {
      LCD_append_char('0');
    }
    delay_ms(2);
    LCD_append_string("   ");
    delay_ms(2);
    LCD_append_string("STORE:");
    sprintf(deposited, "%03d", DEPOSITED);
    LCD_append_string(deposited);
    delay_ms(2);

    // visualize arms with LEDs
    LIGHTA = FREEA ? false : true;  // light if occupied
    LIGHTB = FREEB ? false : true;  // light if occupied
    delay_ms(2);
    set_led_d6(LIGHTA);
    delay_ms(2);
    set_led_d5(LIGHTB);
    delay_ms(2);

    OSTimeDlyHMSM(0,0,0,500); // 500 mili sec delay
  }
}


/*
* Deposit and Feed belt
*
*/
void beltsTask(void *param) {

  for (;;) {
    if (IN) { // Signal IN is true, material is generating
      if (FIRSTSTART) { // right after start, the first amterial must arrive after 50s
        OSTimeDlyHMSM(0,0,50,0);
        FIRSTSTART = false;
      }
      else {
        OSTimeDlyHMSM(0,0,beltSecs,beltMilisecs);
      }
      CANGETFB = true;
      term_send_str_crlf("Material at the end of FB");
      OSTimeDlyHMSM(0,0,0,100);
    }
    else { // Signal IN is false, after 5s the robot is instructed to return to starting position
      OSTimeDlyHMSM(0,0,5,0);
      if (!FIRSTSTART) {
        TOIDLE = true;
        if (POS != IDLE_STATUS) {
          term_send_str_crlf("Idle Status");
        }
      }
    }
  }
}


/*
* Control task
*
*/
void controlTask(void *param) {
  for (;;) {

    // Control flags
    if (TOIDLE) { // return to starting position
      TOIDLE = false;
      if (POS != IDLE_STATUS) {
        switch (POS) {
        case A_OVER_L:
          CCW = false;
          STEPS = 1;
          break;
        case A_OVER_DB:
          CCW = false;
          STEPS = 2;
          break;
        case A_PARAELLELLY_BELTS:
          CCW = true;
          STEPS = 1;
          break;
        default:
          break;
        }
      }
    }
    if (POS == IDLE_STATUS) { // robot is in idle status at the start
      if (CANGETFB && FREEA) { // new material is ready at the end of FB to be taken to press
        OSTimeDlyHMSM(0,0,1,0); // 1 sec delay
        CANGETFB = false;
        FREEA = false;
        term_send_str_crlf("Got new material - A");
        CCW = true;
        STEPS = 1;
      }
    }
    else if (POS == A_OVER_L) {
      if (CANGETFB && FREEB) { // new material is ready at the end of FB to be taken to press
        OSTimeDlyHMSM(0,0,1,0); // 1 sec delay
        CANGETFB = false;
        FREEB = false;
        term_send_str_crlf("Got new material - B");
        if (FREEA && FREEPRESS) { // rotate if arm A is free
          CCW = true;
          STEPS = 1;
        }
      }
      if (!FREEA && FREEPRESS) { // put material on press
        OSTimeDlyHMSM(0,0,1,0); // 1 sec delay
        FREEPRESS = false;
        FREEA = true;
        term_send_str_crlf("Put material on press - A");
      }
      if (FINISHEDPRESS && FREEA) { // get product from press
        OSTimeDlyHMSM(0,0,1,0); // 1 sec delay
        FINISHEDPRESS = false;
        FREEPRESS = true;
        FREEA = false;
        CCW = true;
        STEPS = 1;
        term_send_str_crlf("Got product from press - A");
      }
    }
    else if (POS == A_OVER_DB) { // arm is placing finished product at Deposit blet
      if (CANPUTDB && !FREEA) { // start of the Deposit belt is free
        OSTimeDlyHMSM(0,0,1,0); // 1 sec delay
        FREEA = true;
        term_send_str_crlf("Put product on DB - A");
        DEPOSITED++;
        term_send_str("Finished products = ");
        term_send_num(DEPOSITED);
        term_send_crlf();
        CCW = false;
        STEPS = 1;
      }
      if (!FREEB && FREEPRESS) { // put amterial on press
        OSTimeDlyHMSM(0,0,1,0); // 1 sec delay
        FREEPRESS = false;
        FREEB = true;
        term_send_str_crlf("Put material on press - B");
        CCW = false;
        STEPS = 1;
      }

    }

    // Press
    if (!FREEPRESS && !FINISHEDPRESS) { // not free
      term_send_str_crlf("Starting press");
      OSTimeDlyHMSM(0,0,0,100); // 100 mili sec delay
      FINISHEDPRESS = true;
      term_send_str_crlf("Finished");
    }

    // Rotate
    if (STEPS != 0) {
      term_send_str("Rotating from ");
      term_send_num(POS);
      term_send_crlf();
      if (CCW) {
        OSTimeDlyHMSM(0,0,0,10); // 10 mili sec delay
        POS = abs((POS + STEPS) % 4); // rotate arms counter clockwise
        STEPS = 0;
      }
      if (!CCW) {
        OSTimeDlyHMSM(0,0,0,10); // 10 mili sec delay
        POS = abs((POS - STEPS) % 4); // rotate arms clockwise
        STEPS = 0;
      }
      term_send_str("New position ");
      term_send_num(POS);
      term_send_crlf();
    }


    OSTimeDlyHMSM(0,0,1,0); // 1 mili sec delay
  }
}


/*
* Start task
*
* Creates another tasks in OS and initialize statistical task
*/
void startTask(void *param) {

  OSTaskCreate(controlTask, (void *)0, (void *)&TaskControlStk[TASK_STK_SIZE - 1], 6);
  OSTaskCreate(beltsTask, (void *)0, (void *)&TaskBeltsStk[TASK_STK_SIZE - 1], 7);
  OSTaskCreate(keyboardTask, (void *)0, (void *)&TaskKeyStk[TASK_STK_SIZE - 1], 8);
  OSTaskCreate(displayTask, (void *)0, (void *)&TaskDisplayStk[TASK_STK_SIZE - 1], 9);

  term_send_str_crlf("Idle Status. Waiting for material.");

  for (;;) {
    OSTimeDlyHMSM(0,0,1,0); // 1 sec delay
  }

}

#endif

/*
 * Main function
 */
int main() {

#if FITKIT_VERSION == 2
   // set frequency of material on FB
   beltSecs = 5;
   beltMilisecs = 0;

   // HW initialization
   initialize_hardware();
#endif

   // OS and tasks initialization
    OSInit();

#if FITKIT_VERSION == 2
  // Start task
  OSTaskCreate(startTask, (void *)0, (void*)&TaskStartStk[TASK_STK_SIZE - 1], 5);
#endif

    // Watchdog start
    WDTCTL = WDTPW | WDTTMSEL | WDTCNTCL; // Timer mode, predeleni 32768
    IE1 |= WDTIE;

    // Allow general interrupt
    _EINT();

    // Start multitasking
    OSStart();

   return 0;
}
