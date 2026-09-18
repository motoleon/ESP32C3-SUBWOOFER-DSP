#pragma once

#include <atomic>
#include <cstdint>

#include "esphome/core/component.h"
#include "esphome/core/gpio.h"

namespace esphome {
namespace lfe_dsp {

class LFEDsp : public Component {
 public:
  void set_adc_pin(InternalGPIOPin *pin) { this->adc_pin_ = pin; }
  void set_bclk_pin(InternalGPIOPin *pin) { this->bclk_pin_ = pin; }
  void set_lrck_pin(InternalGPIOPin *pin) { this->lrck_pin_ = pin; }
  void set_data_out_pin(InternalGPIOPin *pin) { this->data_out_pin_ = pin; }
  void set_sample_rate(uint32_t sample_rate) { this->sample_rate_ = sample_rate; }

  void set_gain(float gain) {
    if (gain < 0.0f)
      gain = 0.0f;
    if (gain > 8.0f)
      gain = 8.0f;
    this->gain_.store(gain, std::memory_order_relaxed);
  }
  float get_gain() const { return this->gain_.load(std::memory_order_relaxed); }

  void set_bypass(bool bypass) { this->bypass_.store(bypass, std::memory_order_relaxed); }
  bool get_bypass() const { return this->bypass_.load(std::memory_order_relaxed); }

  void set_invert_phase(bool invert) { this->invert_phase_.store(invert, std::memory_order_relaxed); }
  bool get_invert_phase() const { return this->invert_phase_.load(std::memory_order_relaxed); }

  void set_lpf_hz(float hz) {
    if (hz < 40.0f)
      hz = 40.0f;
    if (hz > 250.0f)
      hz = 250.0f;
    this->lpf_hz_.store(hz, std::memory_order_relaxed);
  }
  float get_lpf_hz() const { return this->lpf_hz_.load(std::memory_order_relaxed); }

  void set_lpf_stages(int stages) {
    if (stages < 1)
      stages = 1;
    if (stages > 3)
      stages = 3;
    this->lpf_stages_.store(stages, std::memory_order_relaxed);
  }
  int get_lpf_stages() const { return this->lpf_stages_.load(std::memory_order_relaxed); }

  void set_hpf(bool enable) { this->hpf_enable_.store(enable, std::memory_order_relaxed); }
  bool get_hpf() const { return this->hpf_enable_.load(std::memory_order_relaxed); }

  void set_hpf_hz(float hz) {
    if (hz < 10.0f)
      hz = 10.0f;
    if (hz > 30.0f)
      hz = 30.0f;
    this->hpf_hz_.store(hz, std::memory_order_relaxed);
  }
  float get_hpf_hz() const { return this->hpf_hz_.load(std::memory_order_relaxed); }

  void set_loudness(bool enable) { this->loudness_.store(enable, std::memory_order_relaxed); }
  bool get_loudness() const { return this->loudness_.load(std::memory_order_relaxed); }

  void set_loudness_db(float db) {
    if (db < 6.0f)
      db = 6.0f;
    if (db > 24.0f)
      db = 24.0f;
    this->loudness_db_.store(db, std::memory_order_relaxed);
  }
  float get_loudness_db() const { return this->loudness_db_.load(std::memory_order_relaxed); }

  void set_input_clip(int32_t clip) {
    if (clip < 400)
      clip = 400;
    if (clip > 1800)
      clip = 1800;
    this->input_clip_.store(clip, std::memory_order_relaxed);
  }
  int32_t get_input_clip() const { return this->input_clip_.load(std::memory_order_relaxed); }

  void set_limiter(bool enable) { this->agc_.store(enable, std::memory_order_relaxed); }
  bool get_limiter() const { return this->agc_.load(std::memory_order_relaxed); }

  void set_limiter_ceiling(float percent) {
    if (percent < 20.0f)
      percent = 20.0f;
    if (percent > 98.0f)
      percent = 98.0f;
    this->compressor_ceiling_.store(percent, std::memory_order_relaxed);
  }
  float get_limiter_ceiling() const { return this->compressor_ceiling_.load(std::memory_order_relaxed); }
  float get_limiter_gr_db() const { return this->agc_boost_db_.load(std::memory_order_relaxed); }

  void set_compressor(bool enable) { this->set_limiter(enable); }
  bool get_compressor() const { return this->get_limiter(); }
  void set_compressor_ceiling(float percent) { this->set_limiter_ceiling(percent); }
  float get_compressor_ceiling() const { return this->get_limiter_ceiling(); }
  float get_compressor_gr_db() const { return this->get_limiter_gr_db(); }

  float get_rms_percent() const { return this->rms_percent_.load(std::memory_order_relaxed); }
  float get_peak_percent() const { return this->peak_percent_.load(std::memory_order_relaxed); }
  float get_rms_out_percent() const { return this->rms_out_percent_.load(std::memory_order_relaxed); }
  float get_peak_out_percent() const { return this->peak_out_percent_.load(std::memory_order_relaxed); }
  bool get_clip_in() const { return this->clip_in_.load(std::memory_order_relaxed); }
  bool get_clip_out() const { return this->clip_out_.load(std::memory_order_relaxed); }

  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }

 protected:
  static void task_entry_(void *arg);
  void audio_task_();
  bool init_adc_();
  bool init_i2s_();
  int adc_channel_() const;

  InternalGPIOPin *adc_pin_{nullptr};
  InternalGPIOPin *bclk_pin_{nullptr};
  InternalGPIOPin *lrck_pin_{nullptr};
  InternalGPIOPin *data_out_pin_{nullptr};
  uint32_t sample_rate_{24000};

  std::atomic<float> gain_{1.0f};
  std::atomic<bool> bypass_{false};
  std::atomic<bool> invert_phase_{false};
  std::atomic<float> lpf_hz_{120.0f};
  std::atomic<int> lpf_stages_{2};
  std::atomic<bool> hpf_enable_{true};
  std::atomic<float> hpf_hz_{18.0f};
  std::atomic<bool> loudness_{false};
  std::atomic<float> loudness_db_{6.0f};
  std::atomic<int32_t> input_clip_{900};

  std::atomic<bool> agc_{false};
  std::atomic<float> compressor_ceiling_{85.0f};
  std::atomic<float> agc_boost_db_{0.0f};

  std::atomic<float> rms_percent_{0.0f};
  std::atomic<float> peak_percent_{0.0f};
  std::atomic<float> rms_out_percent_{0.0f};
  std::atomic<float> peak_out_percent_{0.0f};
  std::atomic<bool> clip_in_{false};
  std::atomic<bool> clip_out_{false};
  bool started_{false};
};

}  // namespace lfe_dsp
}  // namespace esphome
