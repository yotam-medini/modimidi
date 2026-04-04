\version "2.24.3"

modi = \markup {  \translate #'(-2 . -1) \bold\italic "modi" }

\score {
  \new Staff \relative c'' {
    % \tempo "modi" % 4 = 120
    \omit Staff.TimeSignature
    <<
      \new Voice = "one" {
        \override Beam.color = #darkgreen
        \override NoteHead.color = #magenta
        \override Stem.color = #magenta
        e8[ ^\modi
        \override NoteHead.color = #darkblue
        \override Stem.color = #darkblue
        d8] % s16
      }
      \new Lyrics \lyricsto "one" {
        \override LyricText.color = #magenta
        Mi --
        \override LyricText.color = #darkblue
	Di
      }
    >>
  }
  \layout {}
}