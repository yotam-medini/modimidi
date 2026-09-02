#include "gplay.h"
#include <cstdint>
#include <limits>
#include <format>
#include <iostream>
#include <thread>
#include <utility>
#include <vector>
#include "midi.h"
#include "debug.h"
#include "player.h"
#include "qutil.h"
#include "synthseq.h"

class Worker {
 public:
  Worker(
    const midi::Midi &parsed_midi,
    SynthSequencer &synseq,
    const player::PlayerParams &play_params,
    std::function<void()> notify) :
      parsed_midi_{parsed_midi},
      synseq_{synseq},
      play_params_{play_params},
      notify_{std::move(notify)} {
  }
  ~Worker() {
    if (thread_.joinable()) {
      thread_.join();
    }
  }
  void Start() {
    rc_ = 0;
    finished_ = false;
    thread_ = std::thread([this] {
      try {
        Run();
      } catch (...) {
        {
          std::lock_guard<std::mutex> lock(exception_mutex_);
          exception_ = std::current_exception();
        }
        rc_ = 1;
        finished_ = true;
        notify_();
      }
    });
  }
  void Stop() {
    if (player_ && !finished_) {
      player_->PostCommand(player::Command::Quit);
    }
  }
  void Wait() {
    if (thread_.joinable()) {
      thread_.join();
    }
  }
  void PauseResume() {
    if (player_ && !finished_) {
      player_->PostCommand(player::Command::PauseResume);
    }
  }
  void Skip(player::Command command) {
    if (player_ && !finished_) {
      player_->PostCommand(command);
    }
  }
 private:
  void Run() {
    player_ = std::make_unique<player::Player>(
      parsed_midi_, synseq_, play_params_);
    rc_ = player_->run();
    finished_ = true;
    notify_();
  }
  const midi::Midi &parsed_midi_;
  SynthSequencer &synseq_;
  const player::PlayerParams &play_params_;
  std::unique_ptr<player::Player> player_;
  std::function<void()> notify_; // Used for notifying only the end.
  std::thread thread_;
  int rc_{0};
  std::atomic<bool> finished_{false};
  std::mutex exception_mutex_;
  std::exception_ptr exception_;
};

class GPlay::Impl {
 public:
  Impl(bool is_android) : 
    is_android_{is_android},
    synseq_{
      is_android ? GetAndroidSoundFontPath(SF2_ANDROID) : SF2_DESKTOP, 
      0x0} {
    constexpr uint32_t MINUTE_MILLIES = 60000;
    constexpr uint32_t INFINITE_MINUTES_MILLIES = MINUTE_MILLIES *
      (std::numeric_limits<uint32_t>::max() / MINUTE_MILLIES);
    // play_params.debug_ = 0x3;
    play_params_.end_ms_ = INFINITE_MINUTES_MILLIES;
    play_params_.interactive_ = true;
    DebugMessage::AddMessage(std::format("synseq_.err={}", synseq_.error()));
  }
  std::string OpenMidi(std::vector<uint8_t> data) {
    parsed_midi_ = std::make_unique<midi::Midi>(std::move(data));
    auto err = parsed_midi_->GetError();
    if (!err.empty()) {
      parsed_midi_.reset();
    }
    return err;
  }
  uint32_t GetMidiTotalMilliSeconds() const {
    return parsed_midi_->GetTotalMilliSeconds();
  }
  void SetProgressCallback(progress_callback_t progress_cb) {
    play_params_.progress_callback_ = progress_cb;
  }
  void GPlay() {
    DebugMessage::AddMessage("GPlay...");
    worker_ = std::make_unique<Worker>(
      *parsed_midi_, synseq_, play_params_, [this]() {
        SetState(State::None);
      });
    worker_->Start();
    SetState(State::Play);
  }
  void SetStateCallback(OnStateChange_t on_state_change) {
    on_state_change_ = on_state_change;
  }
  void Stop() {
    DebugMessage::AddMessage("Stop...");
    if (worker_) {
      worker_->Stop();
      worker_->Wait();
    }
    SetState(State::None);
  }
  void PauseResume() {
    if (state_ == State::Play) {
      SetState(State::Pause);
      worker_->PauseResume();
    } else if (state_ == State::Pause) {
      SetState(State::Play);
      worker_->PauseResume();
    }
  }
  void SkipForward() {
    if (state_ == State::Play) {
      worker_->Skip(player::Command::Forward);
    }
  }
  void SkipBackward() {
    if (state_ == State::Play) {
      worker_->Skip(player::Command::Backward);
    }
  }
  State GetState() const { return state_; }

  void SetBegin(uint32_t ms) {
    play_params_.begin_ms_ = ms;
  }

  uint32_t GetBegin() const {
    return play_params_.begin_ms_;
  }

  void SetEnd(uint32_t ms) {
    play_params_.end_ms_ = ms;
  }

  uint32_t GetEnd() const {
    return play_params_.end_ms_;
  }

  void SetTempoFactor(float x) {
    if (x > std::numeric_limits<float>::epsilon()) {
      play_params_.tempo_div_factor_ = 1./x;
    }
  };
  float GetTempoFactor() const {
    return 1./play_params_.tempo_div_factor_;
  }
  void SetKeyShift(int8_t ks) {
    play_params_.key_shift_ = ks;
  }

  void SetTMapDefault() {
    play_params_.tracks_velocity_map_.clear();
  }

  void SetTMapTrackDefault(uint8_t track) {
    play_params_.tracks_velocity_map_.erase(track);
  }

  void SetCMapDefault() {
    play_params_.channels_velocity_map_.clear();
  }

  void SetCMapChannelDefault(uint8_t channel) {
    play_params_.channels_velocity_map_.erase(channel);
  }

  void SetTMap(uint8_t track, uint8_t low, uint8_t high) {
    auto &m = play_params_.tracks_velocity_map_;
    m.insert({track, range_t{low, high}});
  }

  void SetCMap(uint8_t channel, uint8_t low, uint8_t high) {
    auto &m = play_params_.channels_velocity_map_;
    m.insert({channel, range_t{low, high}});
  }

  const midi::Midi *GetMidi() const {
    return parsed_midi_.get();
  }

  std::string GetMidiInfo() const {
    return parsed_midi_->info();
  }

 private:
  using range_t = player::PlayerParams::range_t;
  void SetState(State state) {
    state_ = state;
    on_state_change_(state);
  }
  static constexpr auto SF2_DESKTOP = "/usr/share/sounds/sf2/FluidR3_GM.sf2";
  static constexpr auto SF2_ANDROID = "TimGM6mb.sf2";
  const bool is_android_;
  SynthSequencer synseq_;
  player::PlayerParams play_params_;
  std::unique_ptr<midi::Midi> parsed_midi_;
  std::unique_ptr<Worker> worker_;
  State state_{State::None};
  OnStateChange_t on_state_change_;
};

GPlay::GPlay(bool is_android) :
 impl_{std::make_unique<Impl>(is_android)} {
}

GPlay::~GPlay() {
}

std::string GPlay::OpenMidi(std::vector<uint8_t> data) {
  return impl_->OpenMidi(std::move(data));
}

uint32_t GPlay::GetMidiTotalMilliSeconds() const {
  return impl_->GetMidiTotalMilliSeconds();
}

void GPlay::SetProgressCallback(progress_callback_t progress_cb) {
  impl_->SetProgressCallback(progress_cb);
}

void GPlay::Play() {
  impl_->GPlay();
}

void GPlay::Stop() {
  impl_->Stop();
}

void GPlay::PauseResume() {
  impl_->PauseResume();
}

void GPlay::SkipForward() {
  impl_->SkipForward();
}

void GPlay::SkipBackward() {
  impl_->SkipBackward();
}

State GPlay::GetState() const {
  return impl_->GetState();
}

void GPlay::SetStateCallback(OnStateChange_t on_state_change) {
  impl_->SetStateCallback(on_state_change);
}

void GPlay::SetBeginEnd(int i, uint32_t ms) {
  if (i == 0) {
    impl_->SetBegin(ms);
  } else {
    impl_->SetEnd(ms);
  }
}

void GPlay::SetBegin(uint32_t ms) {
  impl_->SetBegin(ms);
}

uint32_t GPlay::GetBegin() const {
  return impl_->GetBegin();
}

void GPlay::SetEnd(uint32_t ms) {
  impl_->SetEnd(ms);
}

uint32_t GPlay::GetEnd() const {
  return impl_->GetEnd();
}

void GPlay::SetTempoFactor(float x) {
  impl_->SetTempoFactor(x);
}

float GPlay::GetTempoFactor() const {
  return impl_->GetTempoFactor();
}

void GPlay::SetKeyShift(int8_t ks) {
  impl_->SetKeyShift(ks);
}

void GPlay::SetTMapDefault() {
  impl_->SetTMapDefault();
}

void GPlay::SetTMapTrackDefault(uint8_t track) {
  impl_->SetTMapTrackDefault(track);
}

void GPlay::SetCMapDefault() {
  impl_->SetCMapDefault();
}

void GPlay::SetCMapChannelDefault(uint8_t channel) {
  impl_->SetCMapChannelDefault(channel);
}

void GPlay::SetTMap(uint8_t track, uint8_t low, uint8_t high) {
  impl_->SetTMap(track, low, high);
}

void GPlay::SetCMap(uint8_t channel, uint8_t low, uint8_t high) {
  impl_->SetCMap(channel, low, high);
}

const midi::Midi *GPlay::GetMidi() const {
  return impl_->GetMidi();
}

std::string GPlay::GetMidiInfo() const {
  return impl_->GetMidiInfo();
}
