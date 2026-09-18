#include "lfe_dsp.h"

#include "driver/i2s_std.h"
#include "esp_adc/adc_continuous.h"
#include "esp_log.h"
#include "esphome/core/hal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <cmath>
#include <cstdint>

namespace esphome {
namespace lfe_dsp {

static const char *const TAG = "lfe_dsp";
static adc_continuous_handle_t adc_handle = nullptr;
static i2s_chan_handle_t tx_handle = nullptr;

void LFEDsp::setup() {
  if (this->adc_pin_ == nullptr || this->bclk_pin_ == nullptr || this->lrck_pin_ == nullptr ||
      this->data_out_pin_ == nullptr) {
    ESP_LOGE(TAG, "Falta un pin requerido");
    this->mark_failed();
    return;
  }

  if (this->sample_rate_ < 8000 || this->sample_rate_ > 48000) {
    ESP_LOGE(TAG, "sample_rate invalido: %lu Hz", (unsigned long) this->sample_rate_);
    this->mark_failed();
    return;
  }

  if (!this->init_adc_() || !this->init_i2s_()) {
    this->mark_failed();
    return;
  }

  this->started_ = true;
  if (xTaskCreate(task_entry_, "lfe_audio", 8192, this, 3, nullptr) != pdPASS) {
    ESP_LOGE(TAG, "No se pudo crear lfe_audio");
    this->mark_failed();
    return;
  }

  ESP_LOGI(TAG, "LFE activo @ %lu Hz", (unsigned long) this->sample_rate_);
}

void LFEDsp::loop() {}

void LFEDsp::dump_config() {
  ESP_LOGCONFIG(TAG, "LFE DSP:");
  ESP_LOGCONFIG(TAG, "  Sample rate: %lu Hz", (unsigned long) this->sample_rate_);
  ESP_LOGCONFIG(TAG, "  Loudness: PEQ 22.4 Hz Q 0.56, 6-24 dB");
  ESP_LOGCONFIG(TAG, "  Ganancia: rampa 16 ms, bloque 256");
  ESP_LOGCONFIG(TAG, "  LPF: 1-3 biquad seleccionables");
  ESP_LOGCONFIG(TAG, "  Limitador: oleada por bloque, servo 1 s, HA 500 ms");
}

int LFEDsp::adc_channel_() const {
  switch (this->adc_pin_->get_pin()) {
    case 0:
      return ADC_CHANNEL_0;
    case 1:
      return ADC_CHANNEL_1;
    case 2:
      return ADC_CHANNEL_2;
    case 3:
      return ADC_CHANNEL_3;
    case 4:
      return ADC_CHANNEL_4;
    default:
      return ADC_CHANNEL_0;
  }
}

bool LFEDsp::init_adc_() {
  adc_continuous_handle_cfg_t handle_cfg = {};
  handle_cfg.max_store_buf_size = 8192;
  handle_cfg.conv_frame_size = 1024;
  if (adc_continuous_new_handle(&handle_cfg, &adc_handle) != ESP_OK)
    return false;
  adc_digi_pattern_config_t pattern = {};
  pattern.atten = ADC_ATTEN_DB_12;
  pattern.channel = this->adc_channel_();
  pattern.unit = ADC_UNIT_1;
  pattern.bit_width = ADC_BITWIDTH_12;
  adc_continuous_config_t cfg = {};
  cfg.pattern_num = 1;
  cfg.adc_pattern = &pattern;
  cfg.sample_freq_hz = this->sample_rate_;
  cfg.conv_mode = ADC_CONV_SINGLE_UNIT_1;
  cfg.format = ADC_DIGI_OUTPUT_FORMAT_TYPE1;
  if (adc_continuous_config(adc_handle, &cfg) != ESP_OK)
    return false;
  return adc_continuous_start(adc_handle) == ESP_OK;
}

bool LFEDsp::init_i2s_() {
  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  chan_cfg.dma_desc_num = 8;
  chan_cfg.dma_frame_num = 256;
  if (i2s_new_channel(&chan_cfg, &tx_handle, nullptr) != ESP_OK)
    return false;
  i2s_std_config_t cfg = {};
  cfg.clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(this->sample_rate_);
  cfg.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
  cfg.gpio_cfg.mclk = I2S_GPIO_UNUSED;
  cfg.gpio_cfg.bclk = (gpio_num_t) this->bclk_pin_->get_pin();
  cfg.gpio_cfg.ws = (gpio_num_t) this->lrck_pin_->get_pin();
  cfg.gpio_cfg.dout = (gpio_num_t) this->data_out_pin_->get_pin();
  cfg.gpio_cfg.din = I2S_GPIO_UNUSED;
  cfg.gpio_cfg.invert_flags.mclk_inv = false;
  cfg.gpio_cfg.invert_flags.bclk_inv = false;
  cfg.gpio_cfg.invert_flags.ws_inv = false;
  if (i2s_channel_init_std_mode(tx_handle, &cfg) != ESP_OK)
    return false;
  return i2s_channel_enable(tx_handle) == ESP_OK;
}

void LFEDsp::task_entry_(void *arg) {
  static_cast<LFEDsp *>(arg)->audio_task_();
  vTaskDelete(nullptr);
}

struct Biquad {
  float b0 = 0, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
  void set_lowpass(float fs, float fc) {
    const float k = tanf(3.14159265f * fc / fs);
    const float k2 = k * k;
    const float q = 0.70710678f;
    const float norm = 1.0f / (1.0f + k / q + k2);
    b0 = k2 * norm;
    b1 = 2.0f * b0;
    b2 = b0;
    a1 = 2.0f * (k2 - 1.0f) * norm;
    a2 = (1.0f - k / q + k2) * norm;
  }

  void set_highpass(float fs, float fc) {
    const float k = tanf(3.14159265f * fc / fs);
    const float k2 = k * k;
    const float q = 0.70710678f;
    const float norm = 1.0f / (1.0f + k / q + k2);
    b0 = norm;
    b1 = -2.0f * norm;
    b2 = norm;
    a1 = 2.0f * (k2 - 1.0f) * norm;
    a2 = (1.0f - k / q + k2) * norm;
  }

  void set_peaking(float fs, float f0, float q, float db) {
    const float A = powf(10.0f, db / 40.0f);
    const float w = 2.0f * 3.14159265f * f0 / fs;
    const float cosw = cosf(w);
    const float alpha = sinf(w) / (2.0f * q);
    const float a0 = 1.0f + alpha / A;
    b0 = (1.0f + alpha * A) / a0;
    b1 = (-2.0f * cosw) / a0;
    b2 = (1.0f - alpha * A) / a0;
    a1 = (-2.0f * cosw) / a0;
    a2 = (1.0f - alpha / A) / a0;
  }

  float process(float x) {
    const float y = b0 * x + z1;
    z1 = b1 * x - a1 * y + z2;
    z2 = b2 * x - a2 * y;
    return y;
  }

  void reset() { z1 = z2 = 0; }
};

static uint32_t isqrt64_(uint64_t value) {
  uint64_t bit = 1ULL << 62;
  uint32_t rms = 0;
  while (bit > value)
    bit >>= 2;
  while (bit != 0) {
    const uint64_t trial = (uint64_t) rms + bit;
    if (value >= trial) {
      value -= trial;
      rms = (rms >> 1) + (uint32_t) bit;
    } else {
      rms >>= 1;
    }
    bit >>= 2;
  }
  return rms;
}

void LFEDsp::audio_task_() {
  constexpr size_t FRAMES = 256;
  constexpr uint32_t ADC_BYTES_PER_SAMPLE = 4;
  constexpr uint32_t PROTECT_HOLD_MS = 6000;
  static uint8_t raw[FRAMES * ADC_BYTES_PER_SAMPLE];
  static int16_t output[FRAMES * 2];

  const float fs = (float) this->sample_rate_;
  const uint32_t offset_cal_samples = this->sample_rate_;
  const float gain_alpha = 1.0f - expf(-1.0f / (fs * 0.016f));

  Biquad hpf, lpf1, lpf2, lpf3, loud;
  float applied_hpf = this->hpf_hz_.load(std::memory_order_relaxed);
  float applied_lpf = this->lpf_hz_.load(std::memory_order_relaxed);
  float applied_loud_db = this->loudness_db_.load(std::memory_order_relaxed);
  int applied_stages = this->lpf_stages_.load(std::memory_order_relaxed);
  if (applied_stages < 1)
    applied_stages = 1;
  if (applied_stages > 3)
    applied_stages = 3;
  if (applied_loud_db < 6.0f)
    applied_loud_db = 6.0f;
  if (applied_loud_db > 24.0f)
    applied_loud_db = 24.0f;
  hpf.set_highpass(fs, applied_hpf);
  lpf1.set_lowpass(fs, applied_lpf);
  lpf2.set_lowpass(fs, applied_lpf);
  lpf3.set_lowpass(fs, applied_lpf);
  loud.set_peaking(fs, 22.4f, 0.56f, applied_loud_db);

  float gain_applied = this->gain_.load(std::memory_order_relaxed);
  if (gain_applied < 0.0f)
    gain_applied = 0.0f;
  if (gain_applied > 8.0f)
    gain_applied = 8.0f;

  float protect_scale = 1.0f;
  float peak_smooth = 0.0f;
  uint32_t protect_hold_until = 0;
  bool protect_was_on = false;
  bool bypass_was = true;
  bool hpf_was_on = false;
  bool loud_was_on = false;

  uint32_t offset_sum = 0, offset_count = 0;
  int32_t adc_offset = 2048;
  bool offset_ready = false;
  uint64_t meter_sum_squares = 0, meter_out_sum_squares = 0;
  uint32_t meter_peak = 0, meter_out_peak = 0, meter_count = 0;
  uint32_t servo_peak_out = 0;
  uint32_t last_meter_ms = millis();
  uint32_t last_servo_ms = millis();
  int16_t last_pcm = 0;
  bool hit_in = false, hit_out = false;

  while (true) {
    uint32_t bytes_read = 0;
    adc_continuous_read(adc_handle, raw, sizeof(raw), &bytes_read, pdMS_TO_TICKS(20));
    if (bytes_read < ADC_BYTES_PER_SAMPLE) {
      continue;
    }

    size_t adc_frames = bytes_read / ADC_BYTES_PER_SAMPLE;
    if (adc_frames > FRAMES)
      adc_frames = FRAMES;

    const bool bypass = this->bypass_.load(std::memory_order_relaxed);
    const bool invert = this->invert_phase_.load(std::memory_order_relaxed);
    const bool hpf_on = this->hpf_enable_.load(std::memory_order_relaxed);
    const bool loud_on = this->loudness_.load(std::memory_order_relaxed);
    const bool protect_on = this->agc_.load(std::memory_order_relaxed);

    float slider = this->gain_.load(std::memory_order_relaxed);
    if (slider < 0.0f)
      slider = 0.0f;
    if (slider > 8.0f)
      slider = 8.0f;

    int32_t clip_adc = this->input_clip_.load(std::memory_order_relaxed);
    if (clip_adc < 400)
      clip_adc = 400;
    if (clip_adc > 1800)
      clip_adc = 1800;
    const float clip_adc_f = (float) clip_adc;

    float ceiling_percent = this->compressor_ceiling_.load(std::memory_order_relaxed);
    if (ceiling_percent < 20.0f)
      ceiling_percent = 20.0f;
    if (ceiling_percent > 98.0f)
      ceiling_percent = 98.0f;

    int stages = this->lpf_stages_.load(std::memory_order_relaxed);
    if (stages < 1)
      stages = 1;
    if (stages > 3)
      stages = 3;
    if (stages != applied_stages) {
      if (stages < 2)
        lpf2.reset();
      if (stages < 3)
        lpf3.reset();
      applied_stages = stages;
    }

    if (!protect_on && protect_was_on) {
      protect_scale = 1.0f;
      peak_smooth = 0.0f;
      protect_hold_until = 0;
    }
    protect_was_on = protect_on;

    if (hpf_on && !hpf_was_on)
      hpf.reset();
    if (!bypass && bypass_was) {
      lpf1.reset();
      lpf2.reset();
      lpf3.reset();
    }
    if (loud_on && !loud_was_on)
      loud.reset();
    hpf_was_on = hpf_on;
    bypass_was = bypass;
    loud_was_on = loud_on;

    const float gain_target = slider * (protect_on ? protect_scale : 1.0f);

    float lpf = this->lpf_hz_.load(std::memory_order_relaxed);
    if (lpf < 40.0f)
      lpf = 40.0f;
    if (lpf > 250.0f)
      lpf = 250.0f;
    if (lpf != applied_lpf) {
      lpf1.set_lowpass(fs, lpf);
      lpf2.set_lowpass(fs, lpf);
      lpf3.set_lowpass(fs, lpf);
      applied_lpf = lpf;
    }

    float hp = this->hpf_hz_.load(std::memory_order_relaxed);
    if (hp < 10.0f)
      hp = 10.0f;
    if (hp > 30.0f)
      hp = 30.0f;
    if (hp != applied_hpf) {
      hpf.set_highpass(fs, hp);
      applied_hpf = hp;
    }

    float ldb = this->loudness_db_.load(std::memory_order_relaxed);
    if (ldb < 6.0f)
      ldb = 6.0f;
    if (ldb > 24.0f)
      ldb = 24.0f;
    if (ldb != applied_loud_db) {
      loud.set_peaking(fs, 22.4f, 0.56f, ldb);
      applied_loud_db = ldb;
    }

    uint32_t block_peak_out = 0;

    for (size_t i = 0; i < FRAMES; ++i) {
      int16_t pcm = last_pcm;
      if (i < adc_frames) {
        const size_t p = i * ADC_BYTES_PER_SAMPLE;
        const uint16_t packed = (uint16_t) raw[p] | ((uint16_t) raw[p + 1] << 8);
        const int32_t adc_raw = packed & 0x0FFF;

        if (!offset_ready) {
          offset_sum += (uint32_t) adc_raw;
          ++offset_count;
          pcm = 0;
          if (offset_count >= offset_cal_samples) {
            adc_offset = (int32_t) (offset_sum / offset_count);
            offset_ready = true;
            hpf.reset();
            lpf1.reset();
            lpf2.reset();
            lpf3.reset();
            loud.reset();
            gain_applied = gain_target;
            protect_scale = 1.0f;
            peak_smooth = 0.0f;
            protect_hold_until = 0;
            last_pcm = 0;
          }
        } else {
          int32_t centered = adc_raw - adc_offset;
          if (centered > clip_adc)
            centered = clip_adc;
          if (centered < -clip_adc)
            centered = -clip_adc;

          float x = (float) centered;
          if (hpf_on)
            x = hpf.process(x);
          if (!bypass) {
            x = lpf1.process(x);
            if (stages >= 2)
              x = lpf2.process(x);
            if (stages >= 3)
              x = lpf3.process(x);
          }

          if (x > clip_adc_f || x < -clip_adc_f)
            hit_in = true;

          const int32_t pre_gain = (int32_t) x;
          const uint32_t magnitude = (uint32_t) (pre_gain < 0 ? -pre_gain : pre_gain);
          meter_sum_squares += (uint64_t) magnitude * magnitude;
          if (magnitude > meter_peak)
            meter_peak = magnitude;
          ++meter_count;

          if (loud_on)
            x = loud.process(x);

          gain_applied += (gain_target - gain_applied) * gain_alpha;
          x *= gain_applied;
          if (invert)
            x = -x;

          int32_t scaled = (int32_t) x * 16;
          if (scaled > 32767 || scaled < -32768)
            hit_out = true;
          if (scaled > 32767)
            scaled = 32767;
          if (scaled < -32768)
            scaled = -32768;
          const uint32_t mag_out = (uint32_t) (scaled < 0 ? -(int64_t) scaled : (int64_t) scaled);
          meter_out_sum_squares += (uint64_t) mag_out * mag_out;
          if (mag_out > meter_out_peak)
            meter_out_peak = mag_out;
          if (mag_out > servo_peak_out)
            servo_peak_out = mag_out;
          if (mag_out > block_peak_out)
            block_peak_out = mag_out;
          pcm = (int16_t) scaled;
          last_pcm = pcm;
        }
      }
      output[2 * i] = pcm;
      output[2 * i + 1] = pcm;
    }

    if (protect_on && offset_ready && block_peak_out > 0) {
      const float block_peak_percent = ((float) block_peak_out / 32767.0f) * 100.0f;
      if (block_peak_percent >= ceiling_percent) {
        protect_scale *= ceiling_percent / block_peak_percent;
        if (protect_scale < 0.25f)
          protect_scale = 0.25f;
        if (protect_scale > 1.0f)
          protect_scale = 1.0f;
        peak_smooth = ceiling_percent;
        protect_hold_until = millis() + PROTECT_HOLD_MS;
      }
    }

    size_t written = 0;
    i2s_channel_write(tx_handle, output, sizeof(output), &written, portMAX_DELAY);

    const uint32_t now = millis();

    if (offset_ready && now - last_meter_ms >= 500 && meter_count > 0) {
      const uint32_t rms_in = isqrt64_(meter_sum_squares / meter_count);
      float rms_percent = ((float) rms_in / 2048.0f) * 100.0f;
      float peak_percent = ((float) meter_peak / 2048.0f) * 100.0f;
      if (rms_percent > 100.0f)
        rms_percent = 100.0f;
      if (peak_percent > 100.0f)
        peak_percent = 100.0f;

      const uint32_t rms_out = isqrt64_(meter_out_sum_squares / meter_count);
      float rms_out_percent = ((float) rms_out / 32767.0f) * 100.0f;
      float peak_out_percent = ((float) meter_out_peak / 32767.0f) * 100.0f;
      if (rms_out_percent > 100.0f)
        rms_out_percent = 100.0f;
      if (peak_out_percent > 100.0f)
        peak_out_percent = 100.0f;

      float gr_db = 0.0f;
      if (protect_scale < 0.99f)
        gr_db = -20.0f * log10f(protect_scale);

      this->rms_percent_.store(rms_percent);
      this->peak_percent_.store(peak_percent);
      this->rms_out_percent_.store(rms_out_percent);
      this->peak_out_percent_.store(peak_out_percent);
      this->agc_boost_db_.store(gr_db);
      this->clip_in_.store(hit_in);
      this->clip_out_.store(hit_out);
      hit_in = false;
      hit_out = false;
      meter_sum_squares = 0;
      meter_peak = 0;
      meter_count = 0;
      meter_out_sum_squares = 0;
      meter_out_peak = 0;
      last_meter_ms = now;
    }

    if (protect_on && offset_ready && now - last_servo_ms >= 1000) {
      float servo_peak_percent = ((float) servo_peak_out / 32767.0f) * 100.0f;
      if (servo_peak_percent > 100.0f)
        servo_peak_percent = 100.0f;

      if (servo_peak_percent > peak_smooth)
        peak_smooth += (servo_peak_percent - peak_smooth) * 0.65f;
      else
        peak_smooth += (servo_peak_percent - peak_smooth) * 0.20f;

      if (peak_smooth > 8.0f) {
        const float ratio = ceiling_percent / peak_smooth;
        if (ratio > 1.015f || ratio < 0.985f) {
          protect_scale *= ratio;
          if (protect_scale < 0.25f)
            protect_scale = 0.25f;
          if (protect_scale > 1.0f)
            protect_scale = 1.0f;
        }
        if (protect_scale < 0.995f || peak_smooth >= ceiling_percent)
          protect_hold_until = now + PROTECT_HOLD_MS;
      }

      if (protect_hold_until != 0 && now >= protect_hold_until && peak_smooth < (ceiling_percent * 0.70f)) {
        protect_scale = 1.0f;
        peak_smooth = 0.0f;
        protect_hold_until = 0;
      }

      servo_peak_out = 0;
      last_servo_ms = now;
    } else if (!protect_on) {
      protect_scale = 1.0f;
      peak_smooth = 0.0f;
      protect_hold_until = 0;
      servo_peak_out = 0;
    }
  }
}

}  // namespace lfe_dsp
}  // namespace esphome
