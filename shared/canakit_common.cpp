#include "canakit_common.hpp"

#define DBG 0
//#define DBG 1

enum comm_type current_comm = CANAKIT_COMM;
const char devnm_def[] = "/dev/tty.usbmodemfd1271";
const char canakit_appnm[] = "mxRelay.kermit";
const char digi_appnm[] = "mxDigi.rb";
char devnm[256];
char retBuf[512];
int init_ok = true; // set to false at app start

// for digi
int switcher_port;
char ip[256];
extern char ip[];

// All IDs
const char * CANA_MODEM_ID  = "05";    // the ID programmed to canakit w/ 'SETIID' command
const char * CANA_USB_ID    = "0A";    // the ID programmed to canakit w/ 'SETIID' command
const char * CANA_BUTTON_ID = "0F";    // the ID programmed to canakit w/ 'SETIID' command


//=========================================================
void ex_program(int sig)
{
    printf("!!! Caught signal: %d ... !!\n", sig);
    if (init_ok == true)
        turn_all_off();
    signal(SIGINT, SIG_DFL);
    kill(getpid(), SIGINT);
}
//=========================================================
int popen_cmd(const char * cmd)
{
    FILE *pf;
    int done = 0;
    char lbuf[128];
    int cnt = 0;

#if DBG==1
    printf("DEBUG:cmd=%s\n",cmd);
#endif
    if (!(pf = popen(cmd,"r")))
    {
      fprintf(stderr, "ERROR: Could not open pipe for output.\n");
      return 1;
    }

    // Grab data from process execution
    // Normally data reads back like this, so need (2) '::' terms
    //      ::[cmd]
    //      [stuff]
    //      ::
    memset(retBuf,0,sizeof(retBuf));
    while (done < 2)
    {
        memset(lbuf,0,sizeof(lbuf));
        if (!fgets(lbuf, sizeof(lbuf) , pf))
            break;
        strcat(retBuf,lbuf);
        if (strstr((const char *)lbuf,(const char *)"::"))
            done++;
        if (++cnt > 1000)
        {
            fprintf(stderr,"WARNING: timeout!\n");
            break;
        }
    }

    if (pclose(pf) != 0)
    {
        fprintf(stderr," ERROR: Failed to close command stream \n");
        return 1;
    }
    return 0;
}
//=========================================================
void do_all_relays(int which_state)
{
    if (which_state == 1)
        turn_all_on();
    else
        turn_all_off();
}
//=========================================================
void do_one_relay(int which_num, int which_state)
{
    char cmd[128];
    if (current_comm == CANAKIT_COMM)
    {
        if (which_state == 1)
#ifdef WINDOWS
            (void)sprintf(cmd,"echo -e \"REL%d.%s\\r\\n\" > %s",which_num,"ON",devnm);
#else
            (void)sprintf(cmd,"%s -p %s REL%d.%s",canakit_appnm,devnm,which_num,"ON");
#endif
        else
#ifdef WINDOWS
            (void)sprintf(cmd,"echo -e \"REL%d.%s\\r\\n\" > %s",which_num,"OFF",devnm);
#else
            (void)sprintf(cmd,"%s -p %s REL%d.%s",canakit_appnm,devnm,which_num,"OFF");
#endif
    }
    else if (current_comm == DIGI_COMM)
    {
        if (which_state == 1)
            (void)sprintf(cmd,"%s -p %s %d %d %d",digi_appnm,ip,254,XLATE_NUM_ON(which_num),switcher_port);
        else
            (void)sprintf(cmd,"%s -p %s %d %d %d",digi_appnm,ip,254,XLATE_NUM_OFF(which_num),switcher_port);
    }
    //printf("CMD:%s\n",cmd);
    popen_cmd(cmd);
}
//=========================================================
int check_board_id(const char * id, char * tbuf)
{
    if (current_comm == DIGI_COMM)
    {
        fprintf(stderr,"\nERROR:check_board_id(%s,%s): You don't want to be calling this!\n",id,tbuf);    
        return 0;
    }

    if ( ! tbuf )
    {
        fprintf(stderr,"ASSERT:check_board_id tbuf == NULL!\n");
        return 0;
    }
    if (get_board_id(tbuf))
    {
        if (strstr(tbuf,id))
        {
            return 1;
        }
    }
    return 0;
}
//=========================================================
int get_board_id(char * id)
{
	char * ptr=0;
    char cmd[128];
    char needle[128];
    int retcnt = 0;

    if (current_comm == DIGI_COMM)
    {
        fprintf(stderr,"\nERROR:get_board_id(%s): You don't want to be calling this!\n",id);    
        return 0;
    }

    if ( ! id )
    {
        fprintf(stderr,"ASSERT:get_board_id id == NULL!\n");
        return 0;
    }
#ifdef WINDOWS
    (void)sprintf(cmd,"echo -e \"ABOUT\\r\\n\" > %s",devnm);
#else
    (void)sprintf(cmd,"%s -p %s ABOUT",canakit_appnm,devnm);
#endif
    (void)sprintf(needle,"ID: ");
    while (retcnt < 3)
    {
        (void)popen_cmd(cmd);
        if (!(ptr = strstr(retBuf,needle)))
            retcnt++;
        else
		{
			ptr+=4;
			strncpy(id,ptr,2);
            id[2] = '\0';
			return 1;
		}
    }
    return 0;
}
//=========================================================
void do_presses(int which_num, int howMany, double d1, double d2)
{
    int i;
    if (howMany < 1)
    {
        //fprintf(stderr,"WARNING: do_presses(%d,%f,%f)\n",howMany,d1,d2);
        return;
    }

    for (i=0; i<howMany; i++)
    {
        do_one_relay(which_num,1);
        usleep(d1);
        do_one_relay(which_num,0);
        usleep(d2);
    }
}
//=========================================================
int does_tty_exist(char * ttyname)
{
    int devtty;
    if (current_comm == DIGI_COMM)
    {
        fprintf(stderr,"\nERROR:does_tty_exist(%s): You don't want to be calling this!\n",ttyname);
        return 0;
    }

    if ((devtty = open (ttyname, O_RDWR | O_NONBLOCK)) < 0)
        return 0;
    close(devtty);
    return 1;
}
//=========================================================
int does_digi_exist(char * ipaddr)
{
	char * ptr=0;
    char cmd[128];
    char needle[5];
    int retcnt = 0;
    if (current_comm == CANAKIT_COMM)
    {
        fprintf(stderr,"\nERROR:does_digi_exist(%s): You don't want to be calling this!\n",ipaddr);
        return 0;
    }

    if ( ! ipaddr )
    {
        fprintf(stderr,"ASSERT:does_digi_exist ip == NULL!\n");
        return 0;
    }
    (void)sprintf(cmd,"%s -p %s %d %d",digi_appnm,ipaddr,254,33);
    (void)sprintf(needle,"55"); // 0x55 is 'U'
    while (retcnt < 3)
    {
        (void)popen_cmd(cmd);
        if (!(ptr = strstr(retBuf,needle)))
            retcnt++;
        else
			return 1;
    }
    return 0;
}

//=========================================================
// <random.c>
//=========================================================
void turn_all_on(void)
{
    if (current_comm == DIGI_COMM)
    {
        fprintf(stderr,"\nERROR:turn_on_all(): You probably don't want to do this! Turn on switch and commander in correct sequence instead!\n");
        //return;
    }

    char cmd[128];
    if (current_comm == CANAKIT_COMM)
#ifdef WINDOWS
    	(void)sprintf(cmd,"echo -e \"RELS.ON\\r\\n\" > %s",devnm);
#else
    	(void)sprintf(cmd,"%s -p %s RELS.ON",canakit_appnm,devnm);
#endif
    else
        (void)sprintf(cmd,"%s -p %s %d %d %d",digi_appnm,ip,254,130,switcher_port);
    popen_cmd(cmd);
}
//=========================================================
void turn_all_off(void)
{
    char cmd[128];
    if (current_comm == CANAKIT_COMM)
#ifdef WINDOWS
        (void)sprintf(cmd,"echo -e \"RELS.OFF\\r\\n\" > %s",devnm);
#else
        (void)sprintf(cmd,"%s -p %s RELS.OFF",canakit_appnm,devnm);
#endif
    else
        (void)sprintf(cmd,"%s -p %s %d %d %d",digi_appnm,ip,254,129,switcher_port);
    popen_cmd(cmd);
}
//=========================================================
// </random.c>
//=========================================================

