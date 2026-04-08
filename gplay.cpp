#include "gplay.h"
#include "midi.h"

class GPlay::Impl {
 public:
   std::string OpenMidi(const std::string &path) {
     parsed_midi_ = std::make_unique<midi::Midi>(path);
     auto err = parsed_midi_->GetError();
     if (!err.empty()) {
       parsed_midi_.reset();
     }
     return err;
   }
   std::unique_ptr<midi::Midi> parsed_midi_;
};

GPlay::GPlay() :
 impl_{std::make_unique<Impl>()} {
}

GPlay::~GPlay() {
}

std::string GPlay::OpenMidi(const std::string& path) {
  return impl_->OpenMidi(path);
}
