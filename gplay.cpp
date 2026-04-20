#include "gplay.h"
#include <cstdint>
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
  void SetProgressCallback(progress_callback_t progress_cb) {
    play_params_.progress_callback_ = progress_cb;
  }
  void GPlay() {
    DebugMessage::AddMessage("GPlay...");
    worker_ = std::make_unique<Worker>(
      *parsed_midi_, synseq_, play_params_, [](){;});
    worker_->Start();
  }
  void Stop() {
    DebugMessage::AddMessage("Stop...");
    if (worker_) {
      worker_->Stop();
    }
  }
 private:
  static constexpr auto SF2_DESKTOP = "/usr/share/sounds/sf2/FluidR3_GM.sf2";
  static constexpr auto SF2_ANDROID = "TimGM6mb.sf2";
  const bool is_android_;
  SynthSequencer synseq_;
  player::PlayerParams play_params_;
  std::unique_ptr<midi::Midi> parsed_midi_;
  std::unique_ptr<Worker> worker_;
};

GPlay::GPlay(bool is_android) :
 impl_{std::make_unique<Impl>(is_android)} {
}

GPlay::~GPlay() {
}

std::string GPlay::OpenMidi(std::vector<uint8_t> data) {
  return impl_->OpenMidi(std::move(data));
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
