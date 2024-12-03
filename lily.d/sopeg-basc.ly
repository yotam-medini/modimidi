\version "2.22.1"
\score {
  \new ChoirStaff <<
    \new Staff \with {midiInstrument = "violin"} {
      \new Voice {
        \set Staff.instrumentName = "Sop"
        \relative c' { e2 << e2 \\ g2 >> }
      }
    }
    \new Staff \with {midiInstrument = "viola"} {
      \new Voice {
        \set Staff.instrumentName = "Alt"
        \relative c' { c1 }
      }
    }
  >>
  \layout {
    \context {
      \Score
      \override DynamicText.direction = #UP
      \override DynamicLineSpanner.direction = #UP
    }
  }
  \midi { }
}