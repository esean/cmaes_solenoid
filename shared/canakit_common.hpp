#include <stdio.h>
#include <unistd.h>  /* sleep(1) */
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#ifndef false
#define false 0
#endif
#ifndef true
#define true 1
#endif

enum comm_type { CANAKIT_COMM, DIGI_COMM };

///////////////
// CANAKIT
///////////////

// button
#define CANA_BUTTON_1       1   // closing relay connects button

// modem
#define CANA_MODEM_MIC      1   // mic
#define CANA_MODEM_GND      2   // ground
#define CANA_MODEM_RIGHT    3   // ring
#define CANA_MODEM_LEFT     4   // tip

// usb
#define CANA_USB_GND        1   // silver
#define CANA_USB_D_POS      2   // green
#define CANA_USB_D_NEG      3   // white
#define CANA_USB_VCC        4   // red

// Dual-Canakit switcher/commander unit
// Switcher (top) unit: left=connection to commander, right(default)=unconnected
#define CANA_SWITCHER_USB_VCC       1   // red 
#define CANA_SWITCHER_USB_GND       2   // black
#define CANA_SWITCHER_USB_D_POS     3   // green
#define CANA_SWITCHER_USB_D_NEG     4   // white
// Commander (bottom) unit: left=USB, right(default)=modem
// USB:
#define CANA_COMMANDER_USB_VCC      1   // red
#define CANA_COMMANDER_USB_GND      2   // black
#define CANA_COMMANDER_USB_D_POS    3   // green
#define CANA_COMMANDER_USB_D_NEG    4   // white
// MODEM:
#define CANA_COMMANDER_MODEM_MIC    1   // silver/mic
#define CANA_COMMANDER_MODEM_GND    2   // red/ground
#define CANA_COMMANDER_MODEM_RIGHT  3   // green/ring
#define CANA_COMMANDER_MODEM_LEFT   4   // blue/tip

///////////////
// DIGI
///////////////
// Each array correpsonds to a different set of commands
// [254, 100 - 107, 1 - 4] means turn ON Relays  1 - 8 on Banks 1 - 4
// [254, 108 - 115, 1 - 4] means turn OFF Relays 1 - 8 on Bank 1 - 4
// [254, 24] means just get status of Relay
// [254, 33] just gets alive status ("U") from relay board

// port-pin/code xlation
#define DIGI_OFF_INDEX_START    100
#define DIGI_ON_INDEX_START     108
#define XLATE_NUM_OFF(b)        (DIGI_OFF_INDEX_START-1+(b))
#define XLATE_NUM_ON(b)         (DIGI_ON_INDEX_START-1+(b))

// switcher toggles between default NC to energised USB-cable at NO
#define DIGI_SWITCHER_USB_VCC       1   // red
#define DIGI_SWITCHER_USB_GND       2   // black
#define DIGI_SWITCHER_USB_D_POS     3   // green
#define DIGI_SWITCHER_USB_D_NEG     4   // white
#define DIGI_SWITCHER_MODEM_MIC     1   // red
#define DIGI_SWITCHER_MODEM_GND     2   // black
#define DIGI_SWITCHER_MODEM_RIGHT   3   // green
#define DIGI_SWITCHER_MODEM_LEFT    4   // white

// USB:
#define DIGI_COMMANDER_USB_VCC      5   // red
#define DIGI_COMMANDER_USB_GND      6   // black
#define DIGI_COMMANDER_USB_D_POS    7   // green
#define DIGI_COMMANDER_USB_D_NEG    8   // white
// MODEM:
#define DIGI_COMMANDER_MODEM_MIC    5   // silver/mic
#define DIGI_COMMANDER_MODEM_GND    6   // red/ground
#define DIGI_COMMANDER_MODEM_RIGHT  7   // green/ring
#define DIGI_COMMANDER_MODEM_LEFT   8   // blue/tip

void ex_program(int sig);
int popen_cmd(const char * cmd);
void do_all_relays(int which_state);
void do_one_relay(int which_num, int which_state);
int check_board_id(const char * id, char * tbuf);
void do_presses(int which_num, int howMany, double d1, double d2);
int does_tty_exist(char * ttyname);
int does_digi_exist(char * ipaddr);
int get_board_id(char * id);

void turn_all_on(void);
void turn_all_off(void);
