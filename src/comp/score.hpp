//
//  score.hpp
//  EnTT Pacman
//

#ifndef COMP_SCORE_HPP
#define COMP_SCORE_HPP

// Running score for a player. Lives on the player entity. Eating systems add to
// it; on a fatal ghost hit, half of the score is taken away.
struct Score {
  int value = 0;
};

#endif
