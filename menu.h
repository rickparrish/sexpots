#ifndef _MENU_H
#define _MENU_H

#include <Windows.h>
#include <WinDef.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef int(*menu_lprintf)(int, char *, ...);
typedef BOOL(*menu_carrier_detect)(COM_HANDLE);

void menu_display(COM_HANDLE com_handle, char *host, ushort *port);
void menu_init(char *exe_path, BOOL *sexpots_terminated, menu_carrier_detect sexpots_carrier_detect, menu_lprintf sexpots_lprintf);

#if defined(__cplusplus)
}
#endif

#endif /* Don't add anything after this #endif statement */
