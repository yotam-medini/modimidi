#include "gplay.h"
#include "midi.h"

class GPlay::Impl {
 public:
   std::unique_ptr<midi::Midi> parsed_midi;
};

GPlay::GPlay() :
 impl_{std::make_unique<Impl>()} {
}

GPlay::~GPlay() {
}
