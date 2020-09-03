#pragma once
#include "resource.h"

// TODO: make these less collision prone
#define IDC_APPNAME                     5000
#define IDC_SYSLINK                     5001
#define IDC_VERSIONSTAMP                5002
#define IDS_URL_HOMEPAGE                529
#define IDS_APPNAME                     530

// Globals to keep track of compile time
const char* g_szBuildDate(__DATE__);
const char* g_szBuildTime(__TIME__);
