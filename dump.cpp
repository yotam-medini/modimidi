#include "midi.h"
#include <format>
#include <fstream>
#include <iostream>
#include <memory>

void midi_dump(const midi::Midi &parsed_midi, std::string &path) {
  std::unique_ptr<std::ofstream> pofs;
  std::ostream *pout = &std::cout;
  if (path != std::string("-")) {
    pofs = std::make_unique<std::ofstream>(path);
    pout = pofs.get();
  }
  std::ostream &out = *pout;
  const midi::Midi &pm = parsed_midi; // abbreviation
  out << "Midi dump\n";
  const size_t ntrks = pm.GetNumTracks();
  out << std::format("format={}, #(tracks)={} division={} ticksPer(1/4)={}\n",
    pm.GetFormat(), ntrks, pm.GetDivision(), pm.GetTicksPerQuarterNote());
  for (size_t itrack = 0; itrack < ntrks; ++itrack) {
    const midi::Track &track = pm.GetTracks()[itrack];
    const std::vector<std::unique_ptr<midi::Event>> &events = track.events_;
    const size_t ne = events.size();
    out << std::format("Track[{:2d}] #(events)={}", itrack, ne) << " {\n";
    uint32_t abs_time = 0;
    for (size_t ie = 0; ie < ne; ++ie) {
      const midi::Event *e = events[ie].get();
      abs_time += e->delta_time_;
      out << std::format(" [{:4d}] AT={}, {}\n", ie, abs_time, e->dt_str());
    } 
    out << "}\n";
  }
}

