#include "gplay.h"
#include <cstdint>
#include <format>
#include <iostream>
#include <thread>
#include <utility>
#include <vector>
#include "midi.h"
#include "debug.h"
#include "play.h"
#include "qutil.h"
#include "synthseq.h"

class GPlay::Impl {
 public:
  Impl(bool is_android) : 
    is_android_{is_android},
    synseq_{
      is_android ? GetAndroidSoundFontPath(SF2_ANDROID) : SF2_DESKTOP, 
      0x0} {
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
  void GPlay() {
    DebugMessage::AddMessage("play...");
#if 1
    int rc = 0;
    std::thread([this, &rc]() {
      PlayParams play_params;
      constexpr uint32_t MINUTE_MILLIES = 60000;
      constexpr uint32_t INFINITE_MINUTES_MILLIES = MINUTE_MILLIES *
        (std::numeric_limits<uint32_t>::max() / MINUTE_MILLIES);
      // play_params.debug_ = 0x3;
      play_params.end_ms_ = INFINITE_MINUTES_MILLIES;
      // This runs in background, leaving UI responsive
      rc = ::Play(*parsed_midi_, synseq_, play_params); 
    }).detach();
#else
    PlayParams play_params;
    constexpr uint32_t MINUTE_MILLIES = 60000;
    constexpr uint32_t INFINITE_MINUTES_MILLIES = MINUTE_MILLIES *
      (std::numeric_limits<uint32_t>::max() / MINUTE_MILLIES);
    auto rc = ::Play(*parsed_midi_, synseq_, play_params);
#endif
    DebugMessage::AddMessage(std::format("played? rc={}", rc));
  }
 private:
  static constexpr auto SF2_DESKTOP = "/usr/share/sounds/sf2/FluidR3_GM.sf2";
  static constexpr auto SF2_ANDROID = "TimGM6mb.sf2";
  const bool is_android_;
  SynthSequencer synseq_;
  std::unique_ptr<midi::Midi> parsed_midi_;
};

GPlay::GPlay(bool is_android) :
 impl_{std::make_unique<Impl>(is_android)} {
}

GPlay::~GPlay() {
}

std::string GPlay::OpenMidi(std::vector<uint8_t> data) {
  return impl_->OpenMidi(std::move(data));
}

void GPlay::Play() {
  impl_->GPlay();
}
