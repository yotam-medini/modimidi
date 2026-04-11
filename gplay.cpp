#include "gplay.h"
#include <cstdint>
#include <format>
#include <iostream>
#include <utility>
#include <vector>
#include "midi.h"

class GPlay::Impl {
 public:
   Impl(bool is_android) : is_android_{is_android} {}
   std::string OpenMidi(std::vector<uint8_t> data) {
     parsed_midi_ = std::make_unique<midi::Midi>(std::move(data));
     auto err = parsed_midi_->GetError();
     if (!err.empty()) {
       parsed_midi_.reset();
     }
     return err;
   }
   std::unique_ptr<midi::Midi> parsed_midi_;
 private:
   const bool is_android_;
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
  std::cerr << std::format("{}:{} {} not yet\n", __FILE__, __LINE__, __func__);
}
