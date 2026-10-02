#include <Arduino.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>

#include "dashboard.h"
#include "rgb_config.h"

namespace {

struct Color {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

struct SegmentState {
  Color color;
  uint8_t brightness;
};

enum class EffectMode : uint8_t { SOLID, PULSE, CYCLE, ALTERNATE };

struct ControllerState {
  SegmentState segment1;
  SegmentState segment2;
  bool linked;
  EffectMode effect;
  uint8_t speed;
};

struct OutputState {
  Color segment1;
  Color segment2;
};

constexpr char AP_SSID[] = "ESP32-RGB";
constexpr char AP_PASSWORD[] = "rgb-control";
constexpr uint8_t SETTINGS_VERSION = 1;
constexpr uint32_t SAVE_DELAY_MS = 1500;
constexpr uint32_t OUTPUT_UPDATE_MS = 15;

WebServer server(80);
DNSServer dnsServer;
Preferences preferences;

ControllerState state{{{124, 92, 255}, 180},
                      {{53, 185, 255}, 180},
                      true,
                      EffectMode::SOLID,
                      128};
OutputState output{{0, 0, 0}, {0, 0, 0}};

uint32_t lastDebugAt = 0;
uint32_t lastOutputAt = 0;
uint32_t stateChangedAt = 0;
bool settingsDirty = false;
uint8_t previewSegment = 0;
uint8_t onboardSegment = 0;
bool debugEnabled = RgbConfig::DEBUG_ENABLED;

const char *effectName(EffectMode effect) {
  switch (effect) {
    case EffectMode::PULSE:
      return "pulse";
    case EffectMode::CYCLE:
      return "cycle";
    case EffectMode::ALTERNATE:
      return "alternate";
    default:
      return "solid";
  }
}

EffectMode parseEffect(const String &value) {
  if (value == "pulse") return EffectMode::PULSE;
  if (value == "cycle") return EffectMode::CYCLE;
  if (value == "alternate") return EffectMode::ALTERNATE;
  return EffectMode::SOLID;
}

uint8_t scaleChannel(uint8_t channel, uint8_t brightness) {
  return (static_cast<uint16_t>(channel) * brightness) / 255;
}

Color scaleColor(const Color &color, uint8_t brightness) {
  return {scaleChannel(color.red, brightness),
          scaleChannel(color.green, brightness),
          scaleChannel(color.blue, brightness)};
}

Color colorWheel(uint8_t position) {
  if (position < 85) {
    return {static_cast<uint8_t>(255 - position * 3),
            static_cast<uint8_t>(position * 3), 0};
  }
  if (position < 170) {
    position -= 85;
    return {0, static_cast<uint8_t>(255 - position * 3),
            static_cast<uint8_t>(position * 3)};
  }
  position -= 170;
  return {static_cast<uint8_t>(position * 3), 0,
          static_cast<uint8_t>(255 - position * 3)};
}

uint32_t effectPeriodMs() {
  return map(state.speed, 0, 255, 6000, 500);
}

void writeOutputs(const Color &segment1, const Color &segment2) {
  output.segment1 = segment1;
  output.segment2 = segment2;

  ledcWrite(RgbConfig::RED_CHANNEL, segment1.red);
  ledcWrite(RgbConfig::GREEN_CHANNEL, segment1.green);
  ledcWrite(RgbConfig::BLUE_CHANNEL, segment1.blue);
  ledcWrite(RgbConfig::SEGMENT_2_RED_CHANNEL, segment2.red);
  ledcWrite(RgbConfig::SEGMENT_2_GREEN_CHANNEL, segment2.green);
  ledcWrite(RgbConfig::SEGMENT_2_BLUE_CHANNEL, segment2.blue);

  // Follow the dashboard selection normally. During Alternate, both the UI
  // selector and NeoPixel follow the segment active in the current phase.
  onboardSegment = previewSegment;
  if (state.effect == EffectMode::ALTERNATE) {
    const uint32_t period = effectPeriodMs();
    onboardSegment = (millis() % period) < period / 2 ? 0 : 1;
  }
  const Color &preview = onboardSegment == 0 ? segment1 : segment2;
  neopixelWrite(RgbConfig::ONBOARD_NEOPIXEL_PIN, preview.red, preview.green,
                preview.blue);
}

void updateOutputs(bool force = false) {
  const uint32_t now = millis();
  if (!force && now - lastOutputAt < OUTPUT_UPDATE_MS) return;
  lastOutputAt = now;

  Color first = scaleColor(state.segment1.color, state.segment1.brightness);
  Color second = scaleColor(state.segment2.color, state.segment2.brightness);
  const uint32_t period = effectPeriodMs();
  const uint32_t phase = now % period;

  switch (state.effect) {
    case EffectMode::PULSE: {
      const uint16_t half = period / 2;
      const uint16_t ramp = phase < half ? phase : period - phase;
      const uint8_t level = 25 + (static_cast<uint32_t>(ramp) * 230) / half;
      first = scaleColor(first, level);
      second = scaleColor(second, level);
      break;
    }
    case EffectMode::CYCLE: {
      const uint8_t wheelPosition =
          (static_cast<uint32_t>(phase) * 255) / period;
      first = scaleColor(colorWheel(wheelPosition), state.segment1.brightness);
      second = scaleColor(colorWheel(state.linked ? wheelPosition
                                                   : wheelPosition + 128),
                          state.segment2.brightness);
      break;
    }
    case EffectMode::ALTERNATE:
      if (phase < period / 2) {
        second = {0, 0, 0};
      } else {
        first = {0, 0, 0};
      }
      break;
    case EffectMode::SOLID:
      break;
  }

  writeOutputs(first, second);
}

bool attachPwm(uint8_t pin, uint8_t channel) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  const double actualFrequency =
      ledcSetup(channel, RgbConfig::PWM_FREQUENCY_HZ,
                RgbConfig::PWM_RESOLUTION_BITS);
  ledcAttachPin(pin, channel);
  return actualFrequency > 0;
}

uint8_t readByteArgument(const char *name, uint8_t fallback) {
  if (!server.hasArg(name)) return fallback;
  return static_cast<uint8_t>(constrain(server.arg(name).toInt(), 0, 255));
}

void loadSettings() {
  preferences.begin("rgb-control", true);
  if (preferences.getUChar("version", 0) == SETTINGS_VERSION) {
    state.segment1.color = {preferences.getUChar("s1r", 124),
                            preferences.getUChar("s1g", 92),
                            preferences.getUChar("s1b", 255)};
    state.segment1.brightness = preferences.getUChar("s1v", 180);
    state.segment2.color = {preferences.getUChar("s2r", 53),
                            preferences.getUChar("s2g", 185),
                            preferences.getUChar("s2b", 255)};
    state.segment2.brightness = preferences.getUChar("s2v", 180);
    state.linked = preferences.getBool("linked", true);
    state.effect = static_cast<EffectMode>(
        min<uint8_t>(preferences.getUChar("effect", 0), 3));
    state.speed = preferences.getUChar("speed", 128);
    debugEnabled =
        preferences.getBool("debug", RgbConfig::DEBUG_ENABLED);
  }
  preferences.end();

  if (state.linked) state.segment2 = state.segment1;
}

void saveSettingsIfNeeded() {
  if (!settingsDirty || millis() - stateChangedAt < SAVE_DELAY_MS) return;

  preferences.begin("rgb-control", false);
  preferences.putUChar("version", SETTINGS_VERSION);
  preferences.putUChar("s1r", state.segment1.color.red);
  preferences.putUChar("s1g", state.segment1.color.green);
  preferences.putUChar("s1b", state.segment1.color.blue);
  preferences.putUChar("s1v", state.segment1.brightness);
  preferences.putUChar("s2r", state.segment2.color.red);
  preferences.putUChar("s2g", state.segment2.color.green);
  preferences.putUChar("s2b", state.segment2.color.blue);
  preferences.putUChar("s2v", state.segment2.brightness);
  preferences.putBool("linked", state.linked);
  preferences.putUChar("effect", static_cast<uint8_t>(state.effect));
  preferences.putUChar("speed", state.speed);
  preferences.putBool("debug", debugEnabled);
  preferences.end();
  settingsDirty = false;
}

void writeDebugRecord(const char *type) {
  if (!debugEnabled) return;

  Serial.printf(
      "{\"type\":\"%s\",\"uptime_ms\":%lu,\"clients\":%u,"
      "\"free_heap\":%u,\"linked\":%s,\"effect\":\"%s\","
      "\"onboard\":{\"gpio\":%u,\"segment\":%u,"
      "\"rgb\":[%u,%u,%u]},"
      "\"segment_1\":{\"rgb\":[%u,%u,%u],\"brightness\":%u,"
      "\"outputs\":{\"R(GPIO%u)\":%u,\"G(GPIO%u)\":%u,"
      "\"B(GPIO%u)\":%u}},"
      "\"segment_2\":{\"rgb\":[%u,%u,%u],\"brightness\":%u,"
      "\"outputs\":{\"R(GPIO%u)\":%u,\"G(GPIO%u)\":%u,"
      "\"B(GPIO%u)\":%u}}}\n",
      type, static_cast<unsigned long>(millis()), WiFi.softAPgetStationNum(),
      ESP.getFreeHeap(), state.linked ? "true" : "false",
      effectName(state.effect), RgbConfig::ONBOARD_NEOPIXEL_PIN,
      onboardSegment + 1,
      onboardSegment == 0 ? output.segment1.red : output.segment2.red,
      onboardSegment == 0 ? output.segment1.green : output.segment2.green,
      onboardSegment == 0 ? output.segment1.blue : output.segment2.blue,
      state.segment1.color.red,
      state.segment1.color.green, state.segment1.color.blue,
      state.segment1.brightness, RgbConfig::RED_PIN, output.segment1.red,
      RgbConfig::GREEN_PIN, output.segment1.green, RgbConfig::BLUE_PIN,
      output.segment1.blue, state.segment2.color.red,
      state.segment2.color.green, state.segment2.color.blue,
      state.segment2.brightness, RgbConfig::SEGMENT_2_RED_PIN,
      output.segment2.red, RgbConfig::SEGMENT_2_GREEN_PIN,
      output.segment2.green, RgbConfig::SEGMENT_2_BLUE_PIN,
      output.segment2.blue);
}

void streamDebugTelemetry() {
  if (!debugEnabled || millis() - lastDebugAt < RgbConfig::DEBUG_INTERVAL_MS) {
    return;
  }
  lastDebugAt = millis();
  writeDebugRecord("telemetry");
}

void sendDashboard() {
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html", DASHBOARD_HTML);
}

void sendState() {
  char json[320];
  snprintf(
      json, sizeof(json),
      "{\"linked\":%s,\"effect\":\"%s\",\"speed\":%u,"
      "\"active_segment\":%u,\"uptime_ms\":%lu,\"debug_enabled\":%s,"
      "\"segments\":[{\"r\":%u,\"g\":%u,\"b\":%u,"
      "\"brightness\":%u},{\"r\":%u,\"g\":%u,\"b\":%u,"
      "\"brightness\":%u}],\"clients\":%u,\"free_heap\":%u}",
      state.linked ? "true" : "false", effectName(state.effect), state.speed,
      previewSegment, static_cast<unsigned long>(millis()),
      debugEnabled ? "true" : "false",
      state.segment1.color.red, state.segment1.color.green,
      state.segment1.color.blue, state.segment1.brightness,
      state.segment2.color.red, state.segment2.color.green,
      state.segment2.color.blue, state.segment2.brightness,
      WiFi.softAPgetStationNum(), ESP.getFreeHeap());
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

void updateState() {
  previewSegment =
      min<uint8_t>(readByteArgument("active_segment", previewSegment), 1);
  if (server.hasArg("debug_enabled")) {
    debugEnabled = server.arg("debug_enabled") == "1";
  }
  state.linked = server.hasArg("linked") ? server.arg("linked") == "1"
                                         : state.linked;
  state.effect = server.hasArg("effect") ? parseEffect(server.arg("effect"))
                                         : state.effect;
  state.speed = readByteArgument("speed", state.speed);

  state.segment1.color.red = readByteArgument("s1r", state.segment1.color.red);
  state.segment1.color.green =
      readByteArgument("s1g", state.segment1.color.green);
  state.segment1.color.blue = readByteArgument("s1b", state.segment1.color.blue);
  state.segment1.brightness =
      readByteArgument("s1brightness", state.segment1.brightness);
  state.segment2.color.red = readByteArgument("s2r", state.segment2.color.red);
  state.segment2.color.green =
      readByteArgument("s2g", state.segment2.color.green);
  state.segment2.color.blue = readByteArgument("s2b", state.segment2.color.blue);
  state.segment2.brightness =
      readByteArgument("s2brightness", state.segment2.brightness);

  if (state.linked) state.segment2 = state.segment1;
  stateChangedAt = millis();
  settingsDirty = true;
  updateOutputs(true);
  writeDebugRecord("state_change");
  sendState();
}

void startWebServer() {
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  if (!WiFi.softAP(AP_SSID, AP_PASSWORD)) {
    Serial.println("Could not start the RGB Wi-Fi network.");
    return;
  }

  const IPAddress address = WiFi.softAPIP();
  dnsServer.start(53, "*", address);
  server.on("/", HTTP_GET, sendDashboard);
  server.on("/api/state", HTTP_GET, sendState);
  server.on("/api/state", HTTP_POST, updateState);
  server.on("/generate_204", HTTP_GET, sendDashboard);
  server.on("/hotspot-detect.html", HTTP_GET, sendDashboard);
  server.on("/connecttest.txt", HTTP_GET, sendDashboard);
  server.onNotFound(sendDashboard);
  server.begin();

  Serial.printf("Wi-Fi: %s\nPassword: %s\nDashboard: http://%s\n", AP_SSID,
                AP_PASSWORD, address.toString().c_str());
  writeDebugRecord("server_ready");
}

}  // namespace

void setup() {
  const bool outputsReady =
      attachPwm(RgbConfig::RED_PIN, RgbConfig::RED_CHANNEL) &&
      attachPwm(RgbConfig::GREEN_PIN, RgbConfig::GREEN_CHANNEL) &&
      attachPwm(RgbConfig::BLUE_PIN, RgbConfig::BLUE_CHANNEL) &&
      attachPwm(RgbConfig::SEGMENT_2_RED_PIN,
                RgbConfig::SEGMENT_2_RED_CHANNEL) &&
      attachPwm(RgbConfig::SEGMENT_2_GREEN_PIN,
                RgbConfig::SEGMENT_2_GREEN_CHANNEL) &&
      attachPwm(RgbConfig::SEGMENT_2_BLUE_PIN,
                RgbConfig::SEGMENT_2_BLUE_CHANNEL);
  writeOutputs({0, 0, 0}, {0, 0, 0});

  Serial.begin(115200);
  delay(500);
  if (!outputsReady) {
    Serial.println("RGB PWM setup failed; outputs will remain off.");
    while (true) delay(1000);
  }

  loadSettings();
  updateOutputs(true);
  Serial.println("ESP32-S3 two-zone RGB controller ready.");
  startWebServer();
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
  updateOutputs();
  saveSettingsIfNeeded();
  streamDebugTelemetry();
  delay(2);
}
