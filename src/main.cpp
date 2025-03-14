#include <SPI.h>
#include <BluetoothSerial.h>
#include <stdlib.h>
#include <string.h>

BluetoothSerial SerialBT;

namespace ELM327 {
  bool adaptiveTiming = true;
  bool lineFeed = true;
  bool headers = false;
  bool echo = true;
  bool spaces = true;
  int timeout = 50;
  int protocol = 0;
  const char* protocolName = "AUTO";
}

const char* ELM_VERSION = "ELM327 v1.5";
const char* ELM_PROTOCOL_NAME = "ISO 15765-4 CAN (11 bit, 500 kbaud)";
const int BUFFER_SIZE = 64;
char btBuffer[BUFFER_SIZE];
int btBufferPos = 0;

void sendPrompt() {
  SerialBT.print("> ");
}

void sendResponse(const String &response) {
  String formatted = response;

  // スペースの除去
  if (!ELM327::spaces) {
    formatted.replace(" ", "");
  }

  // 改行コードの適用
  if (ELM327::lineFeed) {
    SerialBT.print(formatted);
    SerialBT.print("\r\n>");
  } else {
    SerialBT.print(formatted);
    SerialBT.print("\r>");
  }
}

void sendOK() {
  sendResponse("OK");
}

void setProtocol(int protocol) {
  ELM327::protocol = protocol;
  switch (protocol) {
    case 1: ELM327::protocolName = "SAE J1850 PWM"; break;
    case 2: ELM327::protocolName = "SAE J1850 VPW"; break;
    case 3: ELM327::protocolName = "ISO 9141-2"; break;
    case 4: ELM327::protocolName = "ISO 14230-4 KWP (5 baud init)"; break;
    case 5: ELM327::protocolName = "ISO 14230-4 KWP (fast init)"; break;
    case 6: ELM327::protocolName = "ISO 15765-4 CAN (11 bit, 500 kbaud)"; break;
    case 7: ELM327::protocolName = "ISO 15765-4 CAN (29 bit, 500 kbaud)"; break;
    case 8: ELM327::protocolName = "ISO 15765-4 CAN (11 bit, 250 kbaud)"; break;
    case 9: ELM327::protocolName = "ISO 15765-4 CAN (29 bit, 250 kbaud)"; break;
    case 0:
    default:
      ELM327::protocolName = "AUTO";
      ELM327::protocol = 6;
      break;
  }
}

void resetToDefaults() {
  ELM327::adaptiveTiming = true;
  ELM327::lineFeed = true;
  ELM327::headers = false;
  ELM327::timeout = 50;
  ELM327::protocol = 6;
  ELM327::echo = true;
  ELM327::spaces = true;
}

void processBluetoothCommand(const char* cmd) {
  Serial.print("Received BT command: ");
  Serial.println(cmd);

  // バージョン表示
  if (strcasecmp(cmd, "ATZ") == 0) {
      sendResponse(ELM_VERSION);
      sendOK();
      return;
  }
  if (strcasecmp(cmd, "ATI") == 0) {
      sendResponse(ELM_VERSION);
      return;
  }

  // 設定リセット
  if (strcasecmp(cmd, "ATD") == 0) {
    resetToDefaults();
    sendOK();
    return;
  }
  
  // プロトコルチェック
  if (strcasecmp(cmd, "ATPC") == 0) {
    sendResponse(String(ELM327::protocol));
    sendOK();
    return;
  }
  
  // プロトコル情報表示
  if (strcasecmp(cmd, "ATDP") == 0) {
    sendResponse(ELM_PROTOCOL_NAME);
    return;
  }
  if (strcasecmp(cmd, "ATDPN") == 0) {
    char protocolNum[2];
    sprintf(protocolNum, "%X", ELM327::protocol); // 16進表記
    sendResponse(protocolNum);
    return;
  }

  // プロトコル設定
  if (strcasecmp(cmd, "ATSP0") == 0) {
    setProtocol(0);
    sendOK();
    delay(100);
    sendResponse(ELM_PROTOCOL_NAME);
    return;
  }
  if (strncasecmp(cmd, "ATSP", 4) == 0) {
    int newProtocol = atoi(cmd + 4);
    setProtocol(newProtocol);
    sendOK();
    return;
  }
  
  // ヘッダのオン/オフ
  if (strcasecmp(cmd, "ATH0") == 0) {
    ELM327::headers = false;
    sendOK();
    return;
  }
  if (strcasecmp(cmd, "ATH1") == 0) {
    ELM327::headers = true;
    sendOK();
    return;
  }

  // 自動遅延時間調整のオン/オフ
  if (strcasecmp(cmd, "ATM0") == 0) {
    ELM327::adaptiveTiming = true;
    sendOK();
    return;
  }
  if (strcasecmp(cmd, "ATM1") == 0) {
    ELM327::adaptiveTiming = false;
    sendOK();
    return;
  }

  // 自動遅延時間調整のオン/オフ
  if (strcasecmp(cmd, "ATM0") == 0) {
    ELM327::adaptiveTiming = false;
    sendOK();
    return;
  }
  if (strcasecmp(cmd, "ATM1") == 0) {
    ELM327::adaptiveTiming = true;
    sendOK();
    return;
  } 
  // スペースのオン/オフ
  if (strcasecmp(cmd, "ATS0") == 0) {
    ELM327::spaces = false;
    sendOK();
    return;
  }
  if (strcasecmp(cmd, "ATS1") == 0) {
    ELM327::spaces = true;
    sendOK();
    return;
  }

  // ATSTxx (タイムアウト設定 動的変更)
  if (strncasecmp(cmd, "ATST", 4) == 0) {
    int newTimeout = strtol(cmd + 4, NULL, 16); 
    if (newTimeout > 0 && newTimeout <= 255) {
      ELM327::timeout = newTimeout;
      sendOK();
    } else {
      sendResponse("?");
    }
    return;
  } 
  // エコーのオン/オフ
  if (strcasecmp(cmd, "ATE0") == 0) {
    ELM327::echo = false;
    sendOK();
    return;
  }
  if (strcasecmp(cmd, "ATE1") == 0) {
    ELM327::echo = true;
    sendOK();
    return;
  }
  
  // 改行コードのオン/オフ
  if (strcasecmp(cmd, "ATL0") == 0) {
    ELM327::lineFeed = false;
    sendOK();
    return;
  }
  if (strcasecmp(cmd, "ATL1") == 0) {
    ELM327::lineFeed = true;
    sendOK();
    return;
  }
  
  // --- RealDash / Torque Pro 用追加コマンド ---
  // アダプタ識別（AT@1）
  if (strcasecmp(cmd, "AT@1") == 0) {
    sendResponse("ESP32_OBD2 Emulator");
    return;
  }
  // アダプタタイプ（AT@2）
  if (strcasecmp(cmd, "AT@2") == 0) {
    sendResponse("BT Adapter");
    return;
  }
  // バッテリー電圧 (ATRV)
  // 例: 12.6V
  if (strcasecmp(cmd, "ATRV") == 0) {
      sendResponse("12.6V");
      return;
  }
  
  // --- OBD-II モード1 コマンド ---
  
  // PIDサポートビットマップ (0100)
  // サポートするPID: 0x05, 0x0A, 0x0C, 0x0D, 0x0F, 0x44, 0x5C, 0xC9
  if (strcasecmp(cmd, "0100") == 0) {
      String response = ELM327::headers ? "41 00 08 5A 00 00" : "08 5A 00 00";
      sendResponse(response);
      return;
  }
  if (strcasecmp(cmd, "0120") == 0) {
      String response = ELM327::headers ? "41 20 00 00 00 00" : "00 00 00 00";
      sendResponse(response);
      return;
  }
  if (strcasecmp(cmd, "0140") == 0) {
      String response = ELM327::headers ? "41 40 10 00 00 10" : "10 00 00 10";
      sendResponse(response);
      return;
  }
  if (strcasecmp(cmd, "0160") == 0) {
      String response = ELM327::headers ? "41 60 00 00 00 00" : "00 00 00 00";
      sendResponse(response);
      return;
  }
  if (strcasecmp(cmd, "0180") == 0) {
      String response = ELM327::headers ? "41 80 00 00 00 00" : "00 00 00 00";
      sendResponse(response);
      return;
  }
  if (strcasecmp(cmd, "01A0") == 0) {
      String response = ELM327::headers ? "41 A0 00 00 00 00" : "00 00 00 00";
      sendResponse(response);
      return;
  }
  if (strcasecmp(cmd, "01C0") == 0) {
      String response = ELM327::headers ? "41 C0 00 80 00 00" : "00 80 00 00";
      sendResponse(response);
      return;
  }
  
  // 水温 (0105)
  // 例: 80°C → 80 + 40 = 120 (0x78)
  if (strcasecmp(cmd, "0105") == 0) {
    String response = ELM327::headers ? "41 05 78" : "78";
    sendResponse(response);
    return;
  }
  // 油圧 (010A)
  // 例: 3.0bar → 3 / 3  = 1 (0x01)
  if (strcasecmp(cmd, "010A") == 0) {
    String response = ELM327::headers ? "41 0A 01" : "01";
    sendResponse(response);
    return;
  }
  // エンジン回転数 (010C)
  // 例: 3000rpm → (3000 * 4 = 12000 → 0x2E E0)
  if (strcasecmp(cmd, "010C") == 0) {
    String response = ELM327::headers ? "41 0C 2E E0" : "2E E0";
    sendResponse(response);
    return;
  }
  // 車速 (010D)
  // 例: 60km/h → 0x3C
  if (strcasecmp(cmd, "010D") == 0) {
    String response = ELM327::headers ? "41 0D 3C" : "3C";
    sendResponse(response);
    return;
  }
  // 吸気温 (010F)
  // 例: 27°C → 27 + 40 = 67 (0x43)
  if (strcasecmp(cmd, "010F") == 0) {
    String response = ELM327::headers ? "41 0F 43" : "43";
    sendResponse(response);
    return;
  }
  // 空燃比 (AFR) (0144)
  // 例: 空燃比13.2 → 13.2/14.7 * 32768 = 29491 → 0x72F3
  if (strcasecmp(cmd, "0144") == 0) {
      String response = ELM327::headers ? "41 44 72 F3" : "72 F3";
      sendResponse(response);
      return;
  }
  // 油温 (015C)
  // 例: 90°C → 90 + 40 = 130 (0x82)
  if (strcasecmp(cmd, "015C") == 0) {
      String response = ELM327::headers ? "41 5C 82" : "82";
      sendResponse(response);
      return;
  }
  // 油圧 (01C9)
  // 01C9は通常のOBD2 PIDには存在しない拡張PID
  // 例: 300kPa → 3 / 3  = 1 (0x01)
  if (strcasecmp(cmd, "01C9") == 0) {
    String response = ELM327::headers ? "41 C9 01" : "01";
    sendResponse(response);
    return;
  }

  if (strcasecmp(cmd, "0900") == 0) {
    sendResponse("49 00 FF FF FF FF");
    return;
  }
  if (strcasecmp(cmd, "0901") == 0) {
    sendResponse("49 01 01 31 48 47 43 4D 38 32");
    delay(10);
    sendResponse("49 01 02 36 33 33 41 30 30 34");
    delay(10);
    sendResponse("49 01 03 33 35 32");
    return;
  }
  // Calibration ID
  if (strcasecmp(cmd, "0902") == 0) {
    sendResponse("49 02 43 41 4C 49 44 31 32 33 34"); 
    return;
  }
  // Calibration Verification Number (CVN)
  if (strcasecmp(cmd, "0903") == 0) {
    sendResponse("49 03 12 34 56 78");
    return;
  }
  // In-use performance tracking (IPT)
  if (strcasecmp(cmd, "0904") == 0) {
    sendResponse("49 04 56 78 90 AB");
    return;
  }
  // ECU Name
  if (strcasecmp(cmd, "0906") == 0) {
    sendResponse("49 06 45 43 55 31 32 33 34 35 36");
    return;
  }

  // 未対応コマンド
  sendResponse("?");
}

void setup() {
  Serial.begin(115200);

  SerialBT.begin("ESP32_OBD2");
  Serial.println("Bluetooth ELM327 Emulator Started");

  delay(100);
  SerialBT.println(ELM_VERSION);
  sendPrompt();
}

void loop() {
  while (SerialBT.available()) {
    char c = SerialBT.read();
    if (c == '\r' || c == '\n') {
      if (btBufferPos > 0) {
        btBuffer[btBufferPos] = '\0';
        if (ELM327::echo) {
          SerialBT.println(btBuffer);
        }
        processBluetoothCommand(btBuffer);
        btBufferPos = 0;
      }
    } else {
      if (btBufferPos < BUFFER_SIZE - 1) {
        btBuffer[btBufferPos++] = c;
      }
    }
  }
}
