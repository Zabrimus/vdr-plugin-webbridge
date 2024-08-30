/*
 * log.h: WebBridge plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 */

#pragma once

#include "config.h"

#define error(x...)   esyslog("[WebBridge] ERROR: " x)
#define info(x...)    isyslog("[WebBridge]: " x)
// 0x0001: Generic call stack
#define debug1(x...)  void( WebBridgeConfig.IsTraceMode(cWebBridgeConfig::eTraceModeDebug1)  ? dsyslog("[WebBridge][1]: " x)  : void() )
// 0x0002: TBD
#define debug2(x...)  void( WebBridgeConfig.IsTraceMode(cWebBridgeConfig::eTraceModeDebug2)  ? dsyslog("[WebBridge][2]: " x)  : void() )
// 0x0004: TBD
#define debug3(x...)  void( WebBridgeConfig.IsTraceMode(cWebBridgeConfig::eTraceModeDebug3)  ? dsyslog("[WebBridge][3]: " x)  : void() )
// 0x0008: TBD
#define debug4(x...)  void( WebBridgeConfig.IsTraceMode(cWebBridgeConfig::eTraceModeDebug4)  ? dsyslog("[WebBridge][4]: " x)  : void() )
// 0x0010: TBD
#define debug5(x...)  void( WebBridgeConfig.IsTraceMode(cWebBridgeConfig::eTraceModeDebug5)  ? dsyslog("[WebBridge][5]: " x)  : void() )
// 0x0020: TBD
#define debug6(x...)  void( WebBridgeConfig.IsTraceMode(cWebBridgeConfig::eTraceModeDebug6)  ? dsyslog("[WebBridge][6]: " x)  : void() )
// 0x0040: TBD
#define debug7(x...)  void( WebBridgeConfig.IsTraceMode(cWebBridgeConfig::eTraceModeDebug7)  ? dsyslog("[WebBridge][7]: " x)  : void() )
// 0x0080: TBD
#define debug8(x...)  void( WebBridgeConfig.IsTraceMode(cWebBridgeConfig::eTraceModeDebug8)  ? dsyslog("[WebBridge][8]: " x)  : void() )
// 0x0100: TBD
#define debug9(x...)  void( WebBridgeConfig.IsTraceMode(cWebBridgeConfig::eTraceModeDebug9)  ? dsyslog("[WebBridge][9]: " x)  : void() )
