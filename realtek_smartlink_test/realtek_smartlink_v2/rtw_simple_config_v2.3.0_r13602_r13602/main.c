#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include <linux/if.h>
#include <sys/ioctl.h>
#include <netinet/ether.h>
#include <pthread.h>
#include <signal.h>
#include "sc_common.h"
#ifdef PLATFORM_MSTAR
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#endif

#define USB_WIFI

//#define DEFAULT_WLAN_BAND "bgn" //abgn
#define _FW_UNDER_SURVEY	0x0800
#define BUF_SIZ 10240

#define DEST_MAC0 0x01
#define DEST_MAC1 0x00
#define DEST_MAC2 0x5e

static void ProcessPacket(unsigned char* , int);
int collect_scanres();
void doRecvfrom();

char g_bssid_str[64], g_bssid[ETH_ALEN], g_ch[5], g_ssid[64], g_sec[8], g_psk[128];

u8 g_abort = FALSE, g_reinit;
char ifName[IFNAMSIZ];
const unsigned char bc_mac[6]={0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
const char *flag_str[] = {	"SC_SCAN", "SC_SAVE_PROFILE", "SC_DEL_PROFILE",
				"SC_RENAME", "SC_SUCCESS_ACK" };
char g_wpafile_path[512];

void wifi_do_scan(void);
void wifi_disable(void);
void wifi_enable(int normal);

//   =====================	Custimize part	====================================

//const u8 def_chPlan[]= {1,6,11,2,3,4,5,7,8,9,10,12,13,36,40,44,48,52,56,60,64,100,104,108,112,116,132,136,140,149,153,157,161,165};
const u8 def_chPlan[]= {1, 6, 11};
#define MAX_CHNUM		50

#define DEFAULT_IF		"wlan0"
#define DEFAULT_WPAFILE_PATH	"./wpa.conf"
#define DEFAULT_DEVICE_NAME	"RTK_SC_DEV"
#define DEFAULT_PIN		"57289961"

#define	PATTERN_WEP		"[WEP]"
#define	PATTERN_WPA		"[WPA]"
#define	PATTERN_WPA2		"[WPA2]"
#define	PATTERN_WPS		"[WPS]"
#define	PATTERN_IBSS		"[IBSS]"
#define	PATTERN_ESS		"[ESS]"
#define	PATTERN_P2P		"[P2P]"
#define FLAG_WPA		0x0001
#define FLAG_WPA2		0x0002
#define FLAG_WEP		0x0004
#define FLAG_WPS		0x0008
#define FLAG_IBSS		0x0010
#define FLAG_ESS		0x0020
#define FLAG_P2P		0x0040

#define MAX_LINE_LEN		1024
#define MAX_SCAN_TIMES		10

#define SIG_SCPBC		SIGUSR1


#ifdef PLATFORM_MSTAR
int MSTAR_SYSTEM(const char * cmdstring)
{
    pid_t pid;
    int status;
 
    if (cmdstring == NULL) {
        return (1);
    }
 
    if ((pid = fork())<0) {
        status = -1;
    } else if (pid == 0) {
        execl("/sbin/busybox", "sh", "-c", cmdstring, (char *)0);	//Aries , ms918
        //execl("/system/xbin/busybox", "sh", "-c", cmdstring, (char *)0);
        exit(127);
    } else {
        sleep(1);
        while (waitpid(pid, &status, 0) < 0){
            if (errno != EINTR) {
                status = -1;
                break;
            }
        }
    }
 
    return status;
}
#endif

inline void RTK_SYSTEM(const char *cmd)
{
	DEBUG_PRINT("shell:  %s\n", cmd);
#ifdef	PLATFORM_MSTAR
	MSTAR_SYSTEM(cmd);
#else
	system(cmd);
#endif
}

/*
description:	the way of system to connect to tartget AP.
*/
int connect_ap(void)
{
	char cmdstr[200];
	
#ifndef ANDROID_ENV
	//  reinit module status
	wifi_disable();
	wifi_enable(1);
#ifdef CONFIG_IOCTL_CFG80211
	sprintf(cmdstr, "wpa_supplicant -i %s -c %s -Dnl80211 &", ifName, g_wpafile_path);
#else
	//sprintf(cmdstr, "wpa_supplicant -i %s -c %s -Dwext &", ifName, g_wpafile_path);
	sprintf(cmdstr, "wpa_supplicant -i %s -c %s -Dwext &", ifName, "./wpa_supplicant.conf");
#endif
	RTK_SYSTEM(cmdstr);
	sleep(5);
#else
#if defined(USB_WIFI)
	sprintf(cmdstr, "rmmod %s", USB_WIFI_MODULE_NAME);
	RTK_SYSTEM(cmdstr);
	sprintf(cmdstr, "rmmod %s", OTG_RM_MODULE_NAME);
	RTK_SYSTEM(cmdstr);
	sprintf(cmdstr, "rmmod %s", WIFI_POWER_MODULE_NAME);
	RTK_SYSTEM(cmdstr);
#else
	sprintf(cmdstr, "rmmod %s", SDIO_WIFI_MODULE_NAME);
	RTK_SYSTEM(cmdstr);
	sprintf(cmdstr, "rmmod %s", WIFI_POWER_MODULE_NAME);
	RTK_SYSTEM(cmdstr);
#endif
	RTK_SYSTEM("svc wifi enable");
#endif
	return 0;
}

/*
description:	the way of system to get IP address.
*/
int request_ip(void)
{
	char cmdstr[250];
	
#if 0
	sprintf(cmdstr, "ifconfig %s 192.168.0.130", ifName);
	RTK_SYSTEM(cmdstr);
	sleep(3);
#else
	sprintf(cmdstr, "./station_connect.sh %s \"%s\" \"%s\"", g_sec, g_ssid, g_psk);
	printf("%s ---- %s\n\n", __func__, cmdstr);
	RTK_SYSTEM(cmdstr);

	strcpy(cmdstr, "udhcpc -i wlan0");
	RTK_SYSTEM(cmdstr);
#endif
	return 0;
}

/*
input:	@fd		: socket
	@buf		: received control request.
output:	@ctrl_cmd		: AP's ssid
	@accept_control	: <"on"/"off"> , AP's security
description:	parse the received control request and send corresponding response, 
	return 1 if receive a SC_SUCCESS_ACK, otherwise return 0. 
*/
static int ctrlmsg_handler(int fd, char *buf, int *ctrl_cmd, int *accept_control)
{
	int status = rtk_sc_valid_ctrlframe(buf);
	int flag;
	char dev_name[64];

	flag = rtk_sc_get_ctrlmsg_value(buf, CTRLMSG_DATAID_FLAG);
	DEBUG_PRINT("sockfd_ctrl!!  flag=%s \n", flag_str[flag]);
	switch(flag){
	case SC_SAVE_PROFILE:
		if(! (*accept_control))
			break;
		*ctrl_cmd = SC_SAVE_PROFILE;
		*accept_control = 0;
		rtw_sc_send_response_msg(fd, buf, SC_RSP_SAVE, status);
		break;
	case SC_DEL_PROFILE:
		if(! (*accept_control))
			break;
		*accept_control = 0;
		*ctrl_cmd  = SC_DEL_PROFILE;
//		reinit = SC_REINIT_WLAN;
		rtw_sc_send_response_msg(fd, buf, SC_RSP_DEL, status);
		break;
	case SC_RENAME:
		if(! (*accept_control))
			break;
		*accept_control = 0;
		*ctrl_cmd = SC_RENAME;
		rtk_sc_get_ctrlmsg_string(buf, CTRLMSG_DATAID_DEVICE_NAME, dev_name);
		rtk_sc_set_string_value(SC_DEVICE_NAME, dev_name);
		rtw_sc_send_response_msg(fd, buf, SC_RSP_RENAME, status);
		break;
	case SC_SUCCESS_ACK:
		DEBUG_PRINT("SC_SUCCESS_ACK! \n");
		return 1;
		break;
	default:
		DEBUG_PRINT("invalid request\n");
		rtw_sc_send_response_msg(fd, buf, SC_RSP_INVALID, 0);
		break;
	}
	return 0;
}

static void do_ctrl_cmd(int ctrl_cmd)
{
	char cmdstr[200];
	
	switch(ctrl_cmd) {
	case SC_SAVE_PROFILE:
		collect_scanres();
		break;
	case SC_DEL_PROFILE:
//		if(reinit == SC_REINIT_SYSTEM)
//			RTK_SYSTEM("reboot");
//		else if(reinit == SC_REINIT_WLAN) {
		/*      discard reinit system, always reinit wlan	*/
		sprintf(cmdstr, "rm -rf %s&", g_wpafile_path);
		RTK_SYSTEM(cmdstr);		
		RTK_SYSTEM("killall wpa_supplicant");
		RTK_SYSTEM("killall ping");

#if defined(USB_WIFI)
		sprintf(cmdstr, "rmmod %s", USB_WIFI_MODULE_NAME);
		RTK_SYSTEM(cmdstr);
		sprintf(cmdstr, "rmmod %s", OTG_RM_MODULE_NAME);
		RTK_SYSTEM(cmdstr);
		sprintf(cmdstr, "rmmod %s", WIFI_POWER_MODULE_NAME);
		RTK_SYSTEM(cmdstr);	
#else
		sprintf(cmdstr, "rmmod %s", SDIO_WIFI_MODULE_NAME);
		RTK_SYSTEM(cmdstr);
		sprintf(cmdstr, "rmmod %s", WIFI_POWER_MODULE_NAME);
		RTK_SYSTEM(cmdstr);	
#endif
		exit(EXIT_SUCCESS);
		break;
	case SC_RENAME:
		break;
	}
}
//=====================		Custimize part ending	====================================





char *survey_info_path()
{
	static char path[200];

	memset(path, 0, 200);
#if defined(USB_WIFI)
	sprintf(path, "/proc/net/%s/%s/survey_info", PROC_USB_MODULE_PATH, ifName);
#else
	sprintf(path, "/proc/net/%s/%s/survey_info", PROC_SDIO_MODULE_PATH, ifName);
#endif
	return path;
}


/*
input:	@before		: string before unicode procedure.

description:	process unicode(ex chinese word)  SSID of AP,
*/

void wifi_do_scan(void)
{
	char cmdstr[250], fwstats[200], readline[MAX_LINE_LEN];
	FILE *fd = NULL;
	char *ch = NULL;
	int fwstatus = 0, i;
	
#if defined(USB_WIFI)
	sprintf(cmdstr,"echo 1 > /proc/net/%s/%s/survey_info\n",
			PROC_USB_MODULE_PATH, ifName);
#else
	sprintf(cmdstr,"echo 1 > /proc/net/%s/%s/survey_info\n",
			PROC_SDIO_MODULE_PATH, ifName);
#endif
	RTK_SYSTEM(cmdstr);

	//  poll /proc/../fwstatus and wait for scan done. wait 10sec at most.
#if defined(USB_WIFI)
	sprintf(fwstats,"/proc/net/%s/%s/fwstate", PROC_USB_MODULE_PATH, ifName);
#else
	sprintf(fwstats,"/proc/net/%s/%s/fwstate", PROC_SDIO_MODULE_PATH, ifName);
#endif
	for (i = 0; i< 10; i++) {
		sleep(1);
		fd = fopen(fwstats, "r");
		if( fd == NULL ) {
			printf("file[%s] open can not create file !!! \n", fwstats);
			return;
		}
		memset(readline, 0, sizeof(char)*MAX_LINE_LEN );
		fgets(readline, MAX_LINE_LEN , fd);
		fclose(fd);	
		if (strlen(readline) == 0)
			continue;
		if ((ch = strstr(readline, "=")) != NULL)
			fwstatus = (int)strtol(ch+1, NULL, 0);
		if ((fwstatus & _FW_UNDER_SURVEY) == 0)
			break;
	}
}

void wifi_disable(void)
{
	char cmdstr[250];
#ifdef ANDROID_ENV
	RTK_SYSTEM("svc wifi disable");
	RTK_SYSTEM("sleep 5");
#else
	RTK_SYSTEM("killall wpa_supplicant");

#if defined(USB_WIFI)
    sprintf(cmdstr, "rmmod %s", USB_WIFI_MODULE_NAME);
    RTK_SYSTEM(cmdstr);
    sprintf(cmdstr, "rmmod %s", OTG_RM_MODULE_NAME);
    RTK_SYSTEM(cmdstr);
    sprintf(cmdstr, "rmmod %s", WIFI_POWER_MODULE_NAME);
    RTK_SYSTEM(cmdstr);
#else   
    sprintf(cmdstr, "rmmod %s", SDIO_WIFI_MODULE_NAME);
    RTK_SYSTEM(cmdstr);
    sprintf(cmdstr, "rmmod %s", WIFI_POWER_MODULE_NAME);
    RTK_SYSTEM(cmdstr);
#endif

#endif
}

void wifi_enable(int normal)
{
	char cmdstr[250];

#if defined(USB_WIFI)
	if (!normal) {
		sprintf(cmdstr, "insmod %s", WIFI_POWER_MODULE_NAME);
		RTK_SYSTEM(cmdstr);
	}
	sprintf(cmdstr, "insmod %s", OTG_MODULE_NAME);
	RTK_SYSTEM(cmdstr);
	sleep(2);
	if (normal) {
		sprintf(cmdstr, "insmod %s", NORMAL_USB_WIFI_MODULE_NAME);
	} else {
		sprintf(cmdstr, "insmod %s", SMARTLINK_USB_WIFI_MODULE_NAME);
	}
	RTK_SYSTEM(cmdstr);
#else   
	if (!normal) {
		sprintf(cmdstr, "insmod %s", WIFI_POWER_MODULE_NAME);
		RTK_SYSTEM(cmdstr);
	}
	if (normal) {
		sprintf(cmdstr, "insmod %s", NORMAL_SDIO_WIFI_MODULE_NAME);
	} else {
		sprintf(cmdstr, "insmod %s", SMARTLINK_SDIO_WIFI_MODULE_NAME);
	}
	RTK_SYSTEM(cmdstr);
#endif	

	sleep(4);

	//interface UP and enter monitor mode
	sprintf(cmdstr,"ifconfig %s up\n",ifName);
	RTK_SYSTEM(cmdstr);
	sleep(2);
}

char *ssid_rework(const char *before)
{
	char *ch2, *ch1, *ch3;
	unsigned int i, idx = 0;
	static char after[32 * 4 + 1];

	memset(after, 0, 32 * 4 + 1);
	if (before == NULL)
		return after;
	// process unicode SSID, without '"'
	if (((ch1 = strstr(before, "\\x")) != NULL)
		&&((ch2 = strstr(ch1+1, "\\x")) != NULL)
		&&((ch3 = strstr(ch2+1, "\\x")) != NULL)
		&&(ch1 == before)
		&&((ch2-ch1) == 4)
		&&((ch3-ch2) == 4)) {
		for (i = 0; i < strlen(before); i++) {
			if ((before[i]=='\\') && (before[i+1]=='x')) {
				i++;
				continue;
			}
			after[idx++] = before[i];
		}
	}else {	// ASCII SSID, need '"'
		after[0] ='"';
		strcpy(after+1, before);
		after[strlen(before) + 1] = '"';
	}
	return after;
}

/*
input:	@filepath		: path of wpa_supplicant config file which need to be stored.
	@ssid		: AP's ssid
	@cfgfile_type	: the security type of AP
	
description:	store the result as  wpa_supplicant config file  in given file path,
*/
static int store_cfgfile(const char *filepath, char *ssid, int cfgfile_type)
{
	FILE *fd;
	char commset[]={"update_config=1\nctrl_interface=%s\neapol_version=1\nfast_reauth=1\n"};	
	char WPAstr[]={"ap_scan=1\nnetwork={\n\tssid=%s\n\tscan_ssid=1\n\tpsk=\"%s\"\n}\n"};
	char OPENstr[]={"ap_scan=1\nnetwork={\n\tssid=%s\n\tkey_mgmt=NONE\n\tscan_ssid=1\n}\n"};
	char WEPstr[]={"ap_scan=1\nnetwork={\n\tssid=%s\n\tkey_mgmt=NONE\n\twep_key0=\"%s\"\n\twep_tx_keyidx=0\n\tscan_ssid=1\n}\n"};
	char CmdStr[2048], passwd[65];

	memset(passwd, 0, 65);
	fd=fopen(filepath, "w+");
	if( fd == NULL ) {
		printf("file open can not create file !!! \n");
		return -ENOENT;
	}
	sprintf(CmdStr, commset, ifName);
	fprintf(fd,"%s", CmdStr);
	switch (cfgfile_type) {
	case CFGFILE_OPEN :
		sprintf(CmdStr, OPENstr, ssid_rework(ssid));
		break;
	case CFGFILE_WPA :
		rtk_sc_get_string_value(SC_PASSWORD, passwd);
		sprintf(CmdStr, WPAstr, ssid_rework(ssid), passwd);
		strcpy(g_psk, passwd);
		break;
	case CFGFILE_WEP :
		rtk_sc_get_string_value(SC_PASSWORD, passwd);
		sprintf(CmdStr, WEPstr, ssid_rework(ssid), passwd);
		strcpy(g_psk, passwd);
		break;
	default :
		return -EINVAL;
		break;
	}
	DEBUG_PRINT("%s\n",CmdStr);
	fprintf(fd,"%s", CmdStr);
	fclose(fd);
	return 0;
}

void printf_encode(char *txt, size_t maxlen, const u8 *data, size_t len)
{
	char *end = txt + maxlen;
	size_t i;

	for (i = 0; i < len; i++) {
		if (txt + 4 > end)
			break;

		switch (data[i]) {
		case '\"':
			*txt++ = '\\';
			*txt++ = '\"';
			break;
		case '\\':
			*txt++ = '\\';
			*txt++ = '\\';
			break;
		case '\e':
			*txt++ = '\\';
			*txt++ = 'e';
			break;
		case '\n':
			*txt++ = '\\';
			*txt++ = 'n';
			break;
		case '\r':
			*txt++ = '\\';
			*txt++ = 'r';
			break;
		case '\t':
			*txt++ = '\\';
			*txt++ = 't';
			break;
		default:
			if (data[i] >= 32 && data[i] <= 127) {
				*txt++ = data[i];
			} else {
				txt += snprintf(txt, end - txt, "\\x%02x",
						   data[i]);
			}
			break;
		}
	}

	*txt = '\0';
}
const char* wpa_ssid_txt(const u8 *ssid, size_t ssid_len)
{
	static char ssid_txt[32 * 4 + 1];

	if (ssid == NULL) {
		ssid_txt[0] = '\0';
		return ssid_txt;
	}

	printf_encode(ssid_txt, sizeof(ssid_txt), ssid, ssid_len);
	return ssid_txt;
}
/*
input:	@scan_res		: path of scan_result file
	@target_bssid	: bssid of target AP
output:	@ssid		: AP's ssid, do nothing if strlen() is not zero.
	@flag		: indicate property of WPA, WPA2, WPS, IBSS, ESS, P2P.
	@channel		: AP's channel.
	
description:	parse scan_result to get the information of corresponding AP by BSSID.
*/
static int parse_scanres(char *scan_res, char *target_bssid, char *channel, 
			 char *ssid, u16 *flag)
{
	char readline[MAX_LINE_LEN];
	char *ch, *idx, *bssid, *flag_s;
	FILE* fp = NULL;
	int found = -EAGAIN;

	fp = fopen(scan_res, "r");
	if (!fp) {
	    printf("%s:Unable to open file [%s] \n", __func__, scan_res);
	    return -ENOENT;
	}
	if (strlen(target_bssid) < 6) {
		printf("%s:Error !! invalid BSSID [%s] \n", __func__, target_bssid);
		return -EINVAL;
	}
	while (! feof(fp)) {
		if (!fgets(readline, MAX_LINE_LEN , fp))
			break;
		idx = strtok(readline, " \r\n");
		if (!idx)
			continue;
		bssid = strtok(NULL, " \r\n");
		if (!bssid || (strlen(bssid) < 17) 
			|| (strcmp(bssid, target_bssid) != 0))
			continue;
		// Channel
		ch = strtok(NULL, " \r\n");
		if (!ch)
			continue;
		strcpy(channel, ch);
		// RSSI
		ch = strtok(NULL, " \r\n");
		if (!ch)
			continue;
		// SdBm
		ch = strtok(NULL, " \r\n");
		if (!ch)
			continue;
		// Noise
		ch = strtok(NULL, " \r\n");
		if (!ch)
			continue;
		// age
		ch = strtok(NULL, " \r\n");
		if (!ch)
			continue;
		// flag
		flag_s = strtok(NULL, " \r\n");
		if (!flag_s)
			continue;		
		if ((ch = strstr(flag_s, PATTERN_WPA)) != NULL)
			*flag |= FLAG_WPA;
		if ((ch = strstr(flag_s, PATTERN_WPA2)) != NULL)
			*flag |= FLAG_WPA2;
		if ((ch = strstr(flag_s, PATTERN_WEP)) != NULL)
			*flag |= FLAG_WEP;
		if ((ch = strstr(flag_s, PATTERN_WPS)) != NULL)
			*flag |= FLAG_WPS;
		if ((ch = strstr(flag_s, PATTERN_IBSS)) != NULL)
			*flag |= FLAG_IBSS;
		if ((ch = strstr(flag_s, PATTERN_ESS)) != NULL)
			*flag |= FLAG_ESS;
		if ((ch = strstr(flag_s, PATTERN_P2P)) != NULL)
			*flag |= FLAG_P2P;
		// SSID
		if (strlen(ssid) == 0) {
			ch = strtok(NULL, " \t\r\n");
			if (ch) {
				strcpy(ssid, wpa_ssid_txt((const u8*)ch, strlen(ch)));
			}else {
				printf("Warming!! No SSID, please consult"
					" with the vendor!!\n");
				return FALSE;
			}
		}
		found = TRUE;
		DEBUG_PRINT("bssid = [%s], ssid = [%s], flag=%04X \n", 
					bssid, ssid, *flag);
		break;
	}
	fclose(fp);
	return found;
}

int collect_scanres(void)
{
#ifdef ANDROID_ENV
	char cmdstr[250];
#endif
	char bssid_str[64], bssid[ETH_ALEN], ch[5], ssid[64];
	int ret = -100, i;
	u16 flag = 0;
	
	memset(ssid, 0, 64);
	memset(bssid, 0, ETH_ALEN);
	memset(bssid_str, 0, 64);
	memset(ch, 0, 5);
	
	rtk_sc_get_string_value(SC_BSSID, bssid);
	rtk_sc_get_string_value(SC_SSID, ssid);
	sprintf(bssid_str, "%02x:%02x:%02x:%02x:%02x:%02x", 
			(u8)bssid[0], (u8)bssid[1], (u8)bssid[2],
			(u8)bssid[3], (u8)bssid[4], (u8)bssid[5]);
	printf("%s() target_bssid=[%s], ssid=[%s] \n",__func__, bssid_str, ssid);
	for (i = 0; i < MAX_SCAN_TIMES; i++) {
		wifi_do_scan();
		if ((parse_scanres(survey_info_path(), bssid_str, ch, ssid, &flag))!=TRUE)
			continue;
		if ((flag & FLAG_WPA) || (flag & FLAG_WPA2))
			ret = store_cfgfile(g_wpafile_path, ssid, CFGFILE_WPA);
		else if (flag & FLAG_WEP)
			ret = store_cfgfile(g_wpafile_path, ssid, CFGFILE_WEP);
		else
			ret = store_cfgfile(g_wpafile_path, ssid, CFGFILE_OPEN);
		break;
	}

	strcpy(g_bssid_str, bssid_str);
	strcpy(g_bssid, bssid);
	strcpy(g_ch, ch);
	strcpy(g_ssid, ssid);
	if ((flag & FLAG_WPA) || (flag & FLAG_WPA2))
		strcpy(g_sec, "wpa");
	else if (flag & FLAG_WEP)
		strcpy(g_sec, "wep");
	else
		strcpy(g_sec, "open");

#ifdef ANDROID_ENV
	sprintf(cmdstr, "cat %s > /data/misc/wifi/wpa_supplicant.conf", g_wpafile_path);
	RTK_SYSTEM(cmdstr);
	RTK_SYSTEM("cat /data/misc/wifi/wpa_supplicant.conf");
	RTK_SYSTEM("chmod 777 /data/misc/wifi/wpa_supplicant.conf");
#endif
	if (ret == -100)
		printf("Error!! bssid:%s not found \n", bssid_str);
	return ret;
}

#if defined(SIMPLE_CONFIG_PBC_SUPPORT)
//  monitor HW button (ex: gpio pin) to capture pbc event.
void* pbc_monitor()
{
	while (1) {
	//	if (hw SC_PBC event be triggered) {
	//		kill(getpid(), SIG_SCPBC);
	//		printf("pbc_monitor  trigger \n");	
	//	}
		sleep(1);
	}
	return NULL;
}
#endif

/*
	description : open & parse the file (scan_res), collect the ap channel list and channel number
	@ap_chlist : A channel list of all AP's channel by parsing scan result
	@ap_chcnt  : How many channels in @ap_chlist
	Note : same channel may be showed several times because the source not be sorted.
*/
static int get_ap_ch_list(char *scan_res, u8 *ap_chlist, int *ap_chcnt)
{
	char readline[MAX_LINE_LEN];
	char *ch, *value;
	FILE* fp = NULL;
	int  channel, last_channel = 0, linecnt = 0;

	fp = fopen(scan_res, "r");
	if (!fp) {
	    perror("Error opening file");
	    return -ENOENT;
	}
	if ((!ap_chlist) || (!ap_chcnt))
		return -EINVAL;
	*ap_chcnt = 0;
	while (!feof(fp)) {
		if (!fgets(readline, MAX_LINE_LEN , fp))
			break;
		linecnt++;
		if ((ch = strtok(readline, " \r\n"))!=NULL 
			&& ((ch = strtok(NULL, " \r\n"))!=NULL)
			&& ((value = strtok(NULL, " \r\n"))!=NULL)) {

			if ((strcmp(value, "ch") != 0)
				&& (channel = atoi(value))
				&& (last_channel != channel)) {
				ap_chlist[*ap_chcnt] = channel;
				last_channel = channel;
				(*ap_chcnt)++;
			}
		}
	}
	if (linecnt < 2) {
		DEBUG_PRINT("%s: scan result empty \n", scan_res);
		return -EPERM;
	}
	fclose(fp);
	return 0;
}
/* 
	1. Perform site-survey procedure for 3 times.
	2. Parse scan result to collect all AP's channel in air
	3. Intersect channel list in step2 and default channel plan,  
	    then we got a sniffer channel list.
*/
void probe_chplan(u8 *chlist, int *chnum)	
{
	u8 ap_chlist[MAX_CHNUM];
	int i, j, ap_chcnt = 0 , chidx=0;
	int def_chnum = sizeof(def_chPlan)/sizeof(def_chPlan[0]);

	wifi_do_scan();
	memset(ap_chlist, 0, MAX_CHNUM*sizeof(u8));
        if (get_ap_ch_list(survey_info_path(), ap_chlist, &ap_chcnt)==0) {
		for (i=0; i<def_chnum; i++) {
			for (j=0; j<ap_chcnt; j++) {
				if (ap_chlist[j] == def_chPlan[i]) {
					chlist[chidx++] = def_chPlan[i];
					break;
				}
			}
		}
		*chnum = chidx;
	} else {
		memcpy(chlist, def_chPlan, sizeof(u8)*def_chnum);
		*chnum = def_chnum;
	}

	if(g_sc_debug == 2) {
		for (i=0; i<ap_chcnt; i++)
			printf("ap_chlist[%d]:%d \n", i, ap_chlist[i]);
		for (i=0; i<*chnum; i++)
			printf("probe_chplan[%d]:%d \n", i, chlist[i]);
	}
}

void wifi_monitor_mode_onoff(u8 onoff, const char *ifName)
{
	char cmdstr[200];


	memset(cmdstr, 0, sizeof(char)*200);
/*	
	if (onoff)
		sprintf(cmdstr,"echo on > /proc/net/%s/%s/monitor", PROC_MODULE_PATH, ifName);
	else
		sprintf(cmdstr,"echo off > /proc/net/%s/%s/monitor", PROC_MODULE_PATH, ifName);
*/

	//iwconfig wlanX mode monitor
	//iw dev wlanX set type monitor
#ifdef	CONFIG_IOCTL_CFG80211
	if (onoff)
		sprintf(cmdstr,"iw dev %s set type monitor\n", ifName);
	else
		sprintf(cmdstr,"iw dev %s set type managed\n", ifName);
#else
	if (onoff)
		sprintf(cmdstr,"iwconfig %s mode monitor\n", ifName);
	else
		sprintf(cmdstr,"iwconfig %s mode managed\n", ifName);
#endif
	RTK_SYSTEM(cmdstr);
}

static int get_rtheader_len(u8 *buf, size_t len)
{
	struct ieee80211_radiotap_header *rt_header;
	u16 rt_header_size;

	rt_header = (struct ieee80211_radiotap_header *)buf;
	/* check the radiotap header can actually be present */
	if (len < sizeof(struct ieee80211_radiotap_header))
		return -EILSEQ;
	 /* Linux only supports version 0 radiotap format */
	 if (rt_header->it_version)
		return -EILSEQ;
	rt_header_size = le16toh(rt_header->it_len);
	 /* sanity check for allowed length and radiotap length field */
	if (len < rt_header_size)
		return -EILSEQ;
	return rt_header_size;
}


void doRecvfrom(void)
{
	int NumTotalPkts;
	int sockopt;
	ssize_t numbytes;
	struct ifreq ifopts;	/* set promiscuous mode */
	u8 buf[BUF_SIZ], *pkt;
	int rt_header_len = 0;

	//Create a raw socket that shall sniff
	/* Open PF_PACKET socket, listening for EtherType ETHER_TYPE */
	if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1) {
		printf("listener: socket"); 
		return;
	}
	/* Set interface to promiscuous mode - do we need to do this every time? */
	strncpy(ifopts.ifr_name, ifName, IFNAMSIZ-1);
	ioctl(sockfd, SIOCGIFFLAGS, &ifopts);

	//memcpy(g_sc_ctx.dmac, ifopts.ifr_hwaddr.sa_data, ETH_ALEN);


	ifopts.ifr_flags |= IFF_PROMISC;
	ioctl(sockfd, SIOCSIFFLAGS, &ifopts);
	/* Allow the socket to be reused - incase connection is closed prematurely */
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof sockopt) == -1) {
		printf("setsockopt");
		close(sockfd);
		exit(EXIT_FAILURE);
	}
	/* Bind to device */
	if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, ifName, IFNAMSIZ-1) == -1)	{
		printf("SO_BINDTODEVICE");
		close(sockfd);
		exit(EXIT_FAILURE);
	}
	
	NumTotalPkts=0;
	while(1) {
		numbytes = recvfrom(sockfd, buf, BUF_SIZ, 0, NULL, NULL);
		//printf("listener: got packet %d bytes , total packet Num :%d \n", numbytes,NumTotalPkts);
		if (numbytes < 0) {
			printf("Recvfrom error , failed to get packets\n");
			goto thr_exit;
		}
		if( SC_STATUS_DONE == rtk_sc_get_value(SC_STATUS) )
			goto thr_exit;

		// Get length of Radiotap header 
		if ((rt_header_len = get_rtheader_len(buf, (size_t)numbytes)) < 1)
			continue;
		//Now process the packet
		pkt = (u8 *)buf + rt_header_len;
#ifdef CONFIG_NOFCS
		ProcessPacket(pkt, numbytes - rt_header_len - 4);
#else
		ProcessPacket(pkt, numbytes - rt_header_len);
#endif
		NumTotalPkts++;
	}
thr_exit:
	close(sockfd);
	return ;
}

void reset_driver_stage(const char* ifName)
{
//	char cmdstr[200];

	wifi_monitor_mode_onoff(FALSE, ifName);
}

static void sig_handler(int sig)
{
	DEBUG_PRINT("%s: catch signal -- %d \n", __func__, sig);
	switch(sig) {
	case SIGINT:
		g_abort = TRUE;
		RTK_SYSTEM("killall wpa_supplicant");
		RTK_SYSTEM("killall ping");
		reset_driver_stage(ifName);
		exit(EXIT_FAILURE);
		break;
	case SIG_SCPBC:
		g_reinit = TRUE;
		printf("sig_handler()  SIG_SCPBC  \n");
		break;
	default:
		break;
	}
}

static void print_usage(void)
{
	printf("\n\nUsage: \n\trtw_simple_config \t-i<ifname> -c<wpa_cfgfile> -n<dev_name> -p <pincode> \n\t\t\t\t-m <phone_mac> [-dD] [-v]");
	printf("\n\nOPTIONS:\n\t");
	printf("-i = interface name\n\t");
	printf("-c = the path of wpa_supplicant config file\n\t");
	printf("-p = pincode\n\t");
	printf("-d = enable debug message, -D = more message.\n\t");
	printf("-n = device name\n\t");
	printf("-m = filtering MAC, only accept the configuration frame which's from this MAC address\n\t");
	printf("-v = version\t\n\n");
	printf("example:\n\t");
	printf("rtw_simple_config -i wlan0 -c ./wpa_conf -n RTKSC_SAMPLE -D\n\t");
	printf("rtw_simple_config -i wlan0 -c ./wpa_conf -p 14825903 -n RTKSC_SAMPLE\n\n");
	return;
}

int parse_argv(int argc, char **argv)
{
	int opt;

	/*	initial variable by default valve	*/
	rtk_sc_set_string_value(SC_DEVICE_NAME, DEFAULT_DEVICE_NAME);
	memset(ifName, 0, IFNAMSIZ);
	strcpy(ifName, DEFAULT_IF);
	rtk_sc_set_string_value(SC_PIN, DEFAULT_PIN);
	rtk_sc_set_string_value(SC_DEFAULT_PIN, DEFAULT_PIN);
	strcpy(g_wpafile_path, DEFAULT_WPAFILE_PATH);
	rtk_sc_set_macaddr_filter("00:00:00:00:00:00");
	g_sc_debug = 0;

	while ((opt = getopt(argc, argv, "i:c:n:p:P:m:hdDv")) != -1) {
		switch (opt) {
		case 'n':	// device name
			rtk_sc_set_string_value(SC_DEVICE_NAME, optarg);
			break;
		case 'i':	// wlan interface
			strcpy(ifName, optarg);
			break;
		case 'c':	// wpa_supplicant config file path
			strcpy(g_wpafile_path, optarg);
			break;
		case 'p':	// pincode
			rtk_sc_set_string_value(SC_PIN, optarg);
			break;
		case 'm':	// mac address filter
			if (rtk_sc_set_macaddr_filter(optarg)) {
				printf("Invalid MAC address -- %s\n",optarg);
				return -EINVAL;
			}
			break;
		case 'd':	// enable debug message
			g_sc_debug = 1;
			break;
		case 'D':	// enable move debug message
			g_sc_debug = 2;
			break;
		case 'v':
			printf("%s -- %s\n", argv[0], PROGRAM_VERSION);
			exit(EXIT_SUCCESS);
		case 'h':
		default: /* '?' */
			print_usage();
			return -EINVAL;
		}
	}

	if(g_sc_debug) {
		char dbg_str[256];

		DEBUG_PRINT("========option parse========\n");
		rtk_sc_get_string_value(SC_PIN, dbg_str);
		DEBUG_PRINT("pincode = %s\n", dbg_str);
		rtk_sc_get_string_value(SC_DEVICE_NAME, dbg_str);
		DEBUG_PRINT("device name = %s\n",  dbg_str);
		DEBUG_PRINT("ifName = %s\n",  ifName);
		DEBUG_PRINT("g_wpafile_path = %s\n",  g_wpafile_path);
		DEBUG_PRINT("========================\n");
	}
	return 0;
}

int check_ip_timeout(const int timeout)
{
	int sockfd_ack, ret = 0; 
	unsigned int self_ipaddr=0;
	char smac[6];
	int i;

	if ((sockfd_ack = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		perror("socket");
		return errno;
	}
	
	for (i=0; i<timeout; i++) {
		ret = rtk_sc_get_addr(sockfd_ack, smac, &self_ipaddr);
		if (self_ipaddr != 0) {
			close(sockfd_ack);
			return 1;
		}
		sleep(1);
	}
	close(sockfd_ack);
	return ret;
}

int control_handler(void)
{
	int accept_control = 1, on = 1, flag, stop_comfirm = 0;
	int sockfd_scan, sockfd_ctrl;			// socket descriptors
	struct sockaddr_in device_addr;     		// my address information
	struct sockaddr sender; 			// connector’s address information
	socklen_t sendsize = sizeof(sender);
	int sc_run_time, ctrl_cmd, sc_config_success = 0;
	ssize_t numbytes;
	fd_set fds;
	int max_fd, selret;
	char buf[256];
	struct timeval timeout;

	if ((sockfd_scan = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		perror("create scan socket fail\n");
		exit(EXIT_FAILURE);
	}

	setsockopt( sockfd_scan, SOL_SOCKET, SO_BROADCAST|SO_REUSEADDR, &on, sizeof(on) );

	bzero(&device_addr,sizeof(struct sockaddr_in)); 
	device_addr.sin_family = AF_INET;         		// host byte order
	device_addr.sin_port = htons(SCAN_PORT);     		// short, network byte order
	device_addr.sin_addr.s_addr = INADDR_ANY;		// automatically fill with my IP

	// bind the socket with the address
	if (bind(sockfd_scan, (struct sockaddr *)&device_addr, sizeof(struct sockaddr)) == -1) 
	{
		perror("bind scan socket fail\n");
		close(sockfd_scan);
		exit(EXIT_FAILURE);
	}

	if ((sockfd_ctrl = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		perror("create control socket fail\n");
		exit(EXIT_FAILURE);
	}
	setsockopt( sockfd_scan, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );

	bzero(&device_addr,sizeof(struct sockaddr_in)); 
	device_addr.sin_family = AF_INET;         		// host byte order
	device_addr.sin_port = htons(ACK_DEST_PORT);     	// short, network byte order
	device_addr.sin_addr.s_addr = INADDR_ANY;		// automatically fill with my IP

	// bind the socket with the address
	if (bind(sockfd_ctrl, (struct sockaddr *)&device_addr, sizeof(struct sockaddr)) == -1) 
	{
		perror("bind control socket fail\n");
		close(sockfd_scan);
		exit(EXIT_FAILURE);
	}

	sc_run_time=0;
	ctrl_cmd = SC_SCAN;
	while (1) {
		sc_run_time++;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		FD_ZERO(&fds);
		FD_SET(sockfd_scan, &fds);
		FD_SET(sockfd_ctrl, &fds);
		max_fd = (sockfd_ctrl > sockfd_scan) ? sockfd_ctrl : sockfd_scan;
		selret = select(max_fd+1, &fds, NULL, NULL, &timeout);
#if defined(SIMPLE_CONFIG_PBC_SUPPORT)
		if (g_reinit)
			return SC_REINIT_WLAN;
#endif
		if (!stop_comfirm)
			rtk_sc_send_socketack();

		if (selret && FD_ISSET(sockfd_scan, &fds)) {
			stop_comfirm = 1;
			memset(buf, 0, 256);
			if ((numbytes = recvfrom(sockfd_scan, buf, 256, 0,
						   &sender, &sendsize)) == -1) {
				fprintf(stderr,"Receive failed!!!\n");
				close(sockfd_scan);
				exit(EXIT_FAILURE);
			}
			flag = rtk_sc_get_scanmsg_value(buf, SCANMSG_DATAID_FLAG);
			DEBUG_PRINT("sockfd_scan !! pMsg->flag=%s\n", flag_str[flag]);
			switch(flag) {
			case SC_SUCCESS_ACK:
				DEBUG_PRINT("receive config success ack\n");
				ctrl_cmd = SC_SCAN;
				break;
			case SC_SCAN:
				rtw_sc_send_scan_ack(sockfd_scan, buf, SC_RSP_SCAN);
				break;
			default:
				printf("invalid request\n");
				break;
			}
		}
		
		/* Note: APK would continue sends several ctrl_message for one click on GUI, so we 
		    don't do action in receiving each message, we just keep it in "ctrl_cmd" then do_ctrl_cmd() 
		    after ctrl_message sending finish (receive a SC_SUCCESS_ACK message)			*/
		
		if (selret && FD_ISSET(sockfd_ctrl, &fds)) {
			stop_comfirm = 1;
			memset(buf, 0, 256);
			if ((numbytes = recvfrom(sockfd_ctrl, buf, 256, 0,
						   &sender, &sendsize)) == -1) {
				fprintf(stderr,"Receive failed!!!\n");
				close(sockfd_ctrl);
				exit(EXIT_FAILURE);
			}
			if(g_sc_debug == 2) {
				BYTEDUMP("received ctrl_msg: ", buf, (u32)numbytes);
				printf("the length is %d\n", 
					rtk_sc_get_ctrlmsg_value(buf, CTRLMSG_DATAID_LENGTH));
			}
			sc_config_success = ctrlmsg_handler(sockfd_ctrl, buf, &ctrl_cmd, &accept_control);
		}
		else {
			if(sc_config_success == 1) {
				do_ctrl_cmd(ctrl_cmd);
				ctrl_cmd = SC_SCAN;
				sc_config_success = 0;
			}
			accept_control = 1;
		}
	}
}

int main(int argc, char *argv[])
{
	int err, chidx = 0;
	char cmdstr[256];
	pthread_t tid[2];
	u8 chPlan[MAX_CHNUM];
	int chnum = sizeof(def_chPlan)/sizeof(def_chPlan[0]);

	//   Environment  init
	rtk_sc_init();
	if ( parse_argv(argc, argv))
		return -EINVAL;
	memcpy(chPlan, def_chPlan, chnum*sizeof(u8));
	memset(cmdstr,'\0',sizeof(cmdstr));
	signal(SIGINT, sig_handler);
	sprintf(cmdstr, "rm -rf %s&", g_wpafile_path);
	RTK_SYSTEM(cmdstr);
#if defined(SIMPLE_CONFIG_PBC_SUPPORT)
	signal(SIG_SCPBC, sig_handler);
	err = pthread_create(&(tid[1]), NULL, &pbc_monitor, NULL);
#endif

sc_reinit:
	wifi_disable();
	wifi_enable(0);

	if (chidx == 0)
		probe_chplan(chPlan, &chnum);

	wifi_monitor_mode_onoff(TRUE, ifName);
	rtk_sc_set_value(SC_DURATION_TIME, 120);
 	if (!g_reinit) {
		err = pthread_create(&(tid[0]), NULL,(void *)&doRecvfrom, NULL);
		printf("after pthread_create...\n");
		if (err != 0)
			printf("\ncan't create thread :[%s]", strerror(err));
		else
			printf("\n doRecvfrom Thread created successfully\n");
	}
	g_reinit = FALSE;
	rtk_restart_simple_config();

	while (1) {
#if defined(SIMPLE_CONFIG_PBC_SUPPORT)
		if (g_reinit)
			goto sc_reinit;
#endif
		// switch channel & bandwidth
#if defined(USB_WIFI)
		sprintf(cmdstr,"echo %d 0 0 > /proc/net/%s/%s/monitor \n", 
			chPlan[chidx++], PROC_USB_MODULE_PATH, ifName);
#else
		sprintf(cmdstr,"echo %d 0 0 > /proc/net/%s/%s/monitor \n", 
			chPlan[chidx++], PROC_SDIO_MODULE_PATH, ifName);
#endif
		RTK_SYSTEM(cmdstr);
		sleep(TIME_CHSWITCH);
		if ((SC_STATUS_DECODE == rtk_sc_get_value(SC_STATUS))
			|| rtk_sc_get_fix_sa()) {
			char bssid_str[64], bssid[ETH_ALEN], ssid[64], ch[5];
			int i, sec = 6 + rtk_sc_get_profile_pkt_index() * 2;
			u16 flag = 0;

			memset(ssid, 0, 64);
			memset(bssid, 0, ETH_ALEN);
			memset(bssid_str, 0, 64);
			memset(ch, 0, 5);
			rtk_sc_get_string_value(SC_BSSID, bssid);
			sprintf(bssid_str, "%02x:%02x:%02x:%02x:%02x:%02x", 
					(u8)bssid[0], (u8)bssid[1], (u8)bssid[2],
					(u8)bssid[3], (u8)bssid[4], (u8)bssid[5]);
			if ((parse_scanres(survey_info_path(), bssid_str, ch, ssid, &flag))!=TRUE) {
#if defined(USB_WIFI)
				sprintf(cmdstr,"echo %s 0 0 > /proc/net/%s/%s/monitor \n", 
						ch, PROC_USB_MODULE_PATH, ifName);
#else
				sprintf(cmdstr,"echo %s 0 0 > /proc/net/%s/%s/monitor \n", 
						ch, PROC_SDIO_MODULE_PATH, ifName);
#endif
				RTK_SYSTEM(cmdstr);
				DEBUG_PRINT("decode phase!! staying [ch:%s] for"
						" maximum [%d] seconds\n", ch, sec);
			} else 
				DEBUG_PRINT("decode phase!! staying [ch:%d] for"
						" maximum [%d] seconds\n", chPlan[chidx-1], sec);
			for (i = 0; i < sec; i++) {
				sleep(1);
				if (SC_STATUS_DONE == rtk_sc_get_value(SC_STATUS))
					break;
			}
		}

                if (SC_STATUS_DONE == rtk_sc_get_value(SC_STATUS))
                        break;
		rtk_restart_simple_config();
		if (chidx == chnum) {
			chnum = sizeof(def_chPlan)/sizeof(def_chPlan[0]); 	// scan fully channel plan if  first round doesn't success.
			memcpy(chPlan, def_chPlan, chnum*sizeof(u8));		// scan fully channel plan if  first round doesn't success.
			chidx = 0;						// start next scan round, 
		}
	}

	if (SC_STATUS_DONE == rtk_sc_get_value(SC_STATUS)) {
		wifi_monitor_mode_onoff(FALSE, ifName);		
		if (!collect_scanres()) {
			connect_ap();
#ifndef ANDROID_ENV
			request_ip();
#endif
			if (check_ip_timeout(15) && control_handler())
				exit(EXIT_FAILURE);
		}
	}
	return 0;
}
 
static void ProcessPacket(u8 *buffer, int size)
{
	struct rx_frinfo pfrinfo;
	u8 type;
//	u8 subtype;

	/*	80211 header format
		ver:	2bit
		type:	2bit
		subtype:	4bit
		tods:	1bit
		frds:	1bit
		other:	6bit		*/
	pfrinfo.pframe = buffer;
	type = *buffer & TYPE_MASK;
//	subtype = (*buffer & SUBTYPE_MASK) >> 4;	
	if ((type != TYPE_DATA) || (size < 50))
		return ;
	pfrinfo.to_fr_ds = *(buffer + 1) & FRTODS_MASK;
	if (pfrinfo.to_fr_ds == 1) {
		pfrinfo.sa = GetAddr2Ptr(pfrinfo.pframe);
		pfrinfo.da = GetAddr3Ptr(pfrinfo.pframe);
	} else if (pfrinfo.to_fr_ds == 2) {
		pfrinfo.sa = GetAddr3Ptr(pfrinfo.pframe);
		pfrinfo.da = GetAddr1Ptr(pfrinfo.pframe);
	} else {
		return;
	}
	pfrinfo.pktlen = size;
	
	if ((!pfrinfo.da) || (!pfrinfo.sa))
		return ;
	/*	SimpleConfigV1 -- Multicast packets	*/
	if (pfrinfo.da[0] == DEST_MAC0 &&
		pfrinfo.da[1] == DEST_MAC1 &&
		pfrinfo.da[2] == DEST_MAC2
	   )
	{
		if(CHECKSUM_OK(pfrinfo.da, ETH_ALEN))
		{
			rtk_sc_start_parse_packet(&pfrinfo);
		}
  	}
	else if (memcmp(pfrinfo.da, bc_mac, ETH_ALEN)==0) 	/* SimpleConfigV2 -- Broadcast packets	*/
	{
		rtk_sc_start_parse_bcpacket(&pfrinfo);
	}
	
}

