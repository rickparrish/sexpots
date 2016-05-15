/* ANSI C */
#include <stdarg.h>
#include <stdio.h>

/* xpdev lib */
#include "dirwrap.h"
#include "datewrap.h"
#include "sockwrap.h"
#include "ini_file.h"

/* comio lib */
#include "comio.h"

/* RickParrish */
#include <conio.h>
#include "menu.h"

static menu_carrier_detect carrier_detect;
static menu_lprintf lprintf;
static char menu_ansi_path[MAX_PATH + 1];
static char menu_ini_path[MAX_PATH + 1];
static BOOL *terminated;

void menu_printf(COM_HANDLE com_handle, char *fmt, ...);

void menu_display(COM_HANDLE com_handle, char *host, ushort *port) {
	// Get the sections from menu.ini
	str_list_t MenuIniContents = NULL;
	if (fexist(menu_ini_path)) {
		FILE *fp = fopen(menu_ini_path, "r");
		if (fp != NULL) {
			lprintf(LOG_INFO, "Reading %s", menu_ini_path);
			MenuIniContents = iniReadFile(fp);
			fclose(fp);

			if (iniGetSectionCount(MenuIniContents, NULL) == 0) {
				lprintf(LOG_ERR, "No hotkey entries found in %s", menu_ini_path);
				return; // No entries in menu.ini means no modifying host and port
			}
		}
		else {
			lprintf(LOG_ERR, "Error reading %s", menu_ini_path);
			return; // No menu.ini means no modifying host and port
		}
	}
	else {
		lprintf(LOG_WARNING, "File not found: %s", menu_ini_path);
		return; // No menu.ini means no modifying host and port
	}

	// Display menu.ans, if it exists
	BOOL DisplayedMenuAns = FALSE;
	if (fexist(menu_ansi_path)) {
		FILE *fp = fopen(menu_ansi_path, "rb");
		if (fp != NULL) {
			lprintf(LOG_INFO, "Displaying %s", menu_ansi_path);

			char buf[1024];
			int buflen;
			while ((buflen = fread(buf, 1, 1024 - 1, fp)) > 0)
			{
				buf[buflen] = '\0';
				menu_printf(com_handle, "%s", buf);
			}
			fclose(fp);
			DisplayedMenuAns = TRUE;
		}
		else
		{
			lprintf(LOG_ERR, "Error reading %s", menu_ansi_path);
		}
	}
	else
	{
		lprintf(LOG_WARNING, "File not found: %s", menu_ansi_path);
	}

	// Display hardcoded menu if menu.ans could not be displayed
	if (!DisplayedMenuAns) {
		str_list_t MenuIniSections = iniGetSectionList(MenuIniContents, NULL);
		int MenuIniSectionCount = iniGetSectionCount(MenuIniContents, NULL);

		menu_printf(com_handle, "\r\nSelect a BBS from this list:\r\n\r\n");
		for (int i = 0; i < MenuIniSectionCount; i++) {
			char Name[1024];
			iniGetString(MenuIniContents, MenuIniSections[i], "Name", "Unknown BBS", Name);

			menu_printf(com_handle, "  (%s) %s\r\n", MenuIniSections[i], Name);
		}
		menu_printf(com_handle, "\r\nYour choice: ");
	}

	// Wait for hotkey to be pressed
	time_t start = time(NULL);
	BYTE ch[2] = "\0\0";
	while (!(*terminated)) {
		if (time(NULL) - start >= 60) {
			menu_printf(com_handle, "\r\n");
			lprintf(LOG_WARNING, "Timeout waiting for user to pick an option, using default");
			break;
		}

		// Watch for carrier being dropped
		if (!IsDebuggerPresent()) {
			if (!carrier_detect(com_handle)) {
				lprintf(LOG_WARNING, "Loss of Carrier Detect (DCD) detected");
				break;
			}
		}

		// Try to get a key from the dial-up user
		if (IsDebuggerPresent()) {
			if (!kbhit()) {
				YIELD();
				continue;
			}
			ch[0] = _getch();
		}
		else
		{
			if (!comReadByte(com_handle, &ch[0])) {
				YIELD();
				continue;
			}
		}

		// See if the user hit one of the hotkeys
		if (iniSectionExists(MenuIniContents, ch)) {
			char Name[1024];
			iniGetString(MenuIniContents, ch, "Name", "Unknown BBS", Name);
			iniGetExistingString(MenuIniContents, ch, "Hostname", host, host);
			*port = iniGetShortInt(MenuIniContents, ch, "Port", *port);

			menu_printf(com_handle, "\r\n");
			lprintf(LOG_INFO, "User selected %s (%s:%d)", Name, host, *port);
			break;
		}
	}
}

void menu_init(char *exe_path, BOOL *sexpots_terminated, menu_carrier_detect sexpots_carrier_detect, menu_lprintf sexpots_lprintf) {
	// Hook up variable/function pointers to the sexpots originals
	terminated = sexpots_terminated;
	carrier_detect = sexpots_carrier_detect;
	lprintf = sexpots_lprintf;

	/* Generate path/menu.ans and path/menu.ini */
	SAFECOPY(menu_ansi_path, exe_path);
	char *p = getfname(menu_ansi_path);
	*p = 0;
	strcat(menu_ansi_path, "menu.ans");
	SAFECOPY(menu_ini_path, menu_ansi_path);
	p = getfext(menu_ini_path);
	*p = 0;
	strcat(menu_ini_path, ".ini");
}

void menu_printf(COM_HANDLE com_handle, char *fmt, ...) {
	va_list argptr;
	char sbuf[1024];

	va_start(argptr, fmt);
	vsnprintf(sbuf, sizeof(sbuf), fmt, argptr);
	sbuf[sizeof(sbuf) - 1] = 0;
	va_end(argptr);

	if (IsDebuggerPresent()) {
		fprintf(stdout, "%s", sbuf);
	}
	else {
		comWriteString(com_handle, sbuf);
	}
}