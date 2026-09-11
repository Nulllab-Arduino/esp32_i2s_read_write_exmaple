#include <algorithm>
#include <array>

#include "cyf.h"
#include "cyf/log.h"
#include "driver/gpio.h"
#include "driver/i2s_std.h"

namespace {
#include "hello_world_pcm.h"

#if defined(ARDUINO_ESP32S3_DEV)
constexpr gpio_num_t kAmplifierPinBclk = GPIO_NUM_4;  // amplifier pin bclk / bcl / bck
constexpr gpio_num_t kAmplifierPinWs = GPIO_NUM_5;    // amplifier pin ws / lrc
constexpr gpio_num_t kAmplifierPinData = GPIO_NUM_6;  // amplifier pin dout / dat / data / din

constexpr gpio_num_t kMicrophonePinBclk = GPIO_NUM_40;  // microphone pin bclk / bcl / bck
constexpr gpio_num_t kMicrophonePinWs = GPIO_NUM_41;    // microphone pin ws / lrc
constexpr gpio_num_t kMicrophonePinData = GPIO_NUM_42;  // microphone pin dout / dat / data / din
#elif defined(ARDUINO_ESP32_DEV)
constexpr gpio_num_t kAmplifierPinBclk = GPIO_NUM_27;  // amplifier pin bclk / bcl / bck
constexpr gpio_num_t kAmplifierPinWs = GPIO_NUM_26;    // amplifier pin ws / lrc
constexpr gpio_num_t kAmplifierPinData = GPIO_NUM_25;  // amplifier pin dout / dat / data / din

constexpr gpio_num_t kMicrophonePinBclk = GPIO_NUM_23;  // microphone pin bclk / bcl / bck
constexpr gpio_num_t kMicrophonePinWs = GPIO_NUM_32;    // microphone pin ws / lrc
constexpr gpio_num_t kMicrophonePinData = GPIO_NUM_33;  // microphone pin dout / dat / data / din
#endif

constexpr size_t kSampleCount = 16000 / 1000 * 20;  // 20ms
std::array<int32_t, kSampleCount> g_i2s_raw_buffer;
std::array<int16_t, kSampleCount> g_pcm_output_buffer;

i2s_chan_handle_t g_tx_handle = 0;
i2s_chan_handle_t g_rx_handle = 0;
}  // namespace

void setup() {
  Serial.begin(115200);
  CLOGI("i2s read write example");
  i2s_chan_config_t tx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  i2s_chan_config_t rx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);

  tx_chan_cfg.auto_clear_after_cb = true;
  rx_chan_cfg.auto_clear_after_cb = true;

  i2s_new_channel(&tx_chan_cfg, &g_tx_handle, nullptr);
  i2s_new_channel(&rx_chan_cfg, nullptr, &g_rx_handle);

  i2s_std_config_t tx_std_cfg = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
      .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
      .gpio_cfg =
          {
              .mclk = I2S_GPIO_UNUSED,
              .bclk = kAmplifierPinBclk,
              .ws = kAmplifierPinWs,
              .dout = kAmplifierPinData,
              .din = I2S_GPIO_UNUSED,
              .invert_flags =
                  {
                      .mclk_inv = false,
                      .bclk_inv = false,
                      .ws_inv = false,
                  },
          },
  };
  tx_std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_BOTH;

  CLOGI("amplifier pin bclk: %d", tx_std_cfg.gpio_cfg.bclk);
  CLOGI("amplifier pin ws: %d", tx_std_cfg.gpio_cfg.ws);
  CLOGI("amplifier pin dout: %d", tx_std_cfg.gpio_cfg.dout);

  i2s_std_config_t rx_std_cfg = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
      .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
      .gpio_cfg =
          {
              .mclk = I2S_GPIO_UNUSED,
              .bclk = kMicrophonePinBclk,
              .ws = kMicrophonePinWs,
              .dout = I2S_GPIO_UNUSED,
              .din = kMicrophonePinData,
              .invert_flags =
                  {
                      .mclk_inv = false,
                      .bclk_inv = false,
                      .ws_inv = false,
                  },
          },
  };
  rx_std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;

  CLOGI("microphone pin bclk: %d", rx_std_cfg.gpio_cfg.bclk);
  CLOGI("microphone pin ws: %d", rx_std_cfg.gpio_cfg.ws);
  CLOGI("microphone pin din: %d", rx_std_cfg.gpio_cfg.din);

  ESP_ERROR_CHECK(i2s_channel_init_std_mode(g_tx_handle, &tx_std_cfg));
  ESP_ERROR_CHECK(i2s_channel_enable(g_tx_handle));

  ESP_ERROR_CHECK(i2s_channel_init_std_mode(g_rx_handle, &rx_std_cfg));
  ESP_ERROR_CHECK(i2s_channel_enable(g_rx_handle));

  size_t bytes_written = 0;
  i2s_channel_write(g_tx_handle, kHelloWorldPcm, sizeof(kHelloWorldPcm), &bytes_written, 1000 * 10);
  CLOGI("wrote %d bytes", bytes_written);
}

void loop() {
  i2s_channel_read(g_rx_handle, g_i2s_raw_buffer.data(), g_i2s_raw_buffer.size() * sizeof(int32_t), nullptr, 1000 * 10);

  for (size_t i = 0; i < g_i2s_raw_buffer.size(); i++) {
    // Right shift by 14 to convert 24-bit mic data to 16-bit with 4x gain
    int32_t scaled_sample = g_i2s_raw_buffer[i] >> 14;

    // Clamp the sample to 16-bit signed range to prevent distortion
    g_pcm_output_buffer[i] = std::clamp<int32_t>(scaled_sample, INT16_MIN, INT16_MAX);
  }

  i2s_channel_write(g_tx_handle, g_pcm_output_buffer.data(), g_pcm_output_buffer.size() * sizeof(int16_t), nullptr, 1000 * 10);
}