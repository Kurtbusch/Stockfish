/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2018 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <algorithm>

#include "types.h"

Value PieceValue[PHASE_NB][PIECE_NB] = {
  { VALUE_ZERO, PawnValueMg, KnightValueMg, BishopValueMg, RookValueMg, QueenValueMg },
  { VALUE_ZERO, PawnValueEg, KnightValueEg, BishopValueEg, RookValueEg, QueenValueEg }
};

namespace PSQT {

#define S(mg, eg) make_score(mg, eg)

// Bonus[PieceType][Square / 2] contains Piece-Square scores. For each piece
// type on a given square a (middlegame, endgame) score pair is assigned. Table
// is defined for files A..D and white side: it is symmetric for black side and
// second half of the files.
constexpr Score Bonus[][RANK_NB][int(FILE_NB) / 2] = {
  { },
  { // Pawn
   { S(  0, 0), S(  0,  0), S(  0, 0), S( 0, 0) },
   { S(-11,-3), S(  7, -1), S(  7, 7), S(17, 2) },
   { S(-16,-2), S( -3,  2), S( 23, 6), S(23,-1) },
   { S(-14, 7), S( -7, -4), S( 20,-8), S(24, 2) },
   { S( -5,13), S( -2, 10), S( -1,-1), S(12,-8) },
   { S(-11,16), S(-12,  6), S( -2, 1), S( 4,16) },
   { S( -2, 1), S( 20,-12), S(-10, 6), S(-2,25) }
  },
  { // Knight
   { S(-169,-105), S(-96,-74), S(-80,-46), S(-79,-18) },
   { S( -79, -70), S(-39,-56), S(-24,-15), S( -9,  6) },
   { S( -64, -38), S(-20,-33), S(  4, -5), S( 19, 27) },
   { S( -28, -36), S(  5,  0), S( 41, 13), S( 47, 34) },
   { S( -29, -41), S( 13,-20), S( 42,  4), S( 52, 35) },
   { S( -11, -51), S( 28,-38), S( 63,-17), S( 55, 19) },
   { S( -67, -64), S(-21,-45), S(  6,-37), S( 37, 16) },
   { S(-200, -98), S(-80,-89), S(-53,-53), S(-32,-16) }
  },
  { // Bishop
   { S(-49,-58), S(- 7,-31), S(-10,-37), S(-34,-19) },
   { S(-24,-34), S(  9, -9), S( 15,-14), S(  1,  4) },
   { S( -9,-23), S( 22,  0), S( -3, -3), S( 12, 16) },
   { S(  4,-26), S(  9, -3), S( 18, -5), S( 40, 16) },
   { S( -8,-26), S( 27, -4), S( 13, -7), S( 30, 14) },
   { S(-17,-24), S( 14, -2), S( -6,  0), S(  6, 13) },
   { S(-19,-34), S(-13,-10), S(  7,-12), S(-11,  6) },
   { S(-47,-55), S( -7,-32), S(-17,-36), S(-29,-17) }
  },
  { // Rook
   { S(-25, 0), S(-16, 0), S(-10, 5), S( 6, 5) },
   { S(-21, 0), S( -8, 0), S( -3, 0), S( 0, 0) },
   { S(-21, 0), S( -9, 0), S( -4, 0), S( 2, 0) },
   { S(-22, 0), S( -6, 0), S( -1, 0), S( 2, 0) },
   { S(-22, 0), S( -7, 0), S(  0, 0), S( 1, 0) },
   { S(-21, 0), S( -7, 0), S(  0, 0), S( 2, 0) },
   { S(-12, 0), S(  4, 0), S(  8, 0), S(12, 0) },
   { S(-23, 0), S(-15, 0), S(-11, 0), S(-5, 0) }
  },
  { // Queen
   { S( 0,-71), S(-4,-56), S(-3,-42), S(-1,-29) },
   { S(-4,-56), S( 6,-30), S( 9,-21), S( 8, -5) },
   { S(-2,-39), S( 6,-17), S( 9, -8), S( 9,  5) },
   { S(-1,-29), S( 8, -5), S(10,  9), S( 7, 19) },
   { S(-3,-27), S( 9, -5), S( 8, 10), S( 7, 21) },
   { S(-2,-40), S( 6,-16), S( 8,-10), S(10,  3) },
   { S(-2,-55), S( 7,-30), S( 7,-21), S( 6, -6) },
   { S(-1,-74), S(-4,-55), S(-1,-43), S( 0,-30) }
  }
};

constexpr Score KBonus[RANK_NB][FILE_NB] =
{ // King
   { S(273, -4), S(340, 35), S(291, 74), S(207, 90), S(188, 66), S(261, 79), S(327, 37), S(273,  5) },
   { S(295, 58), S(304, 96), S(248,149), S(197,139), S(179,126), S(229,124), S(304, 99), S(278, 61) },
   { S(218, 69), S(260,145), S(160,149), S(128,175), S(113,158), S(158,162), S(251,141), S(216, 83) },
   { S(167,105), S(193,152), S(132,172), S(83, 189), S(117,172), S(141,185), S(197,149), S(161,104) },
   { S(146,101), S(169,152), S(89, 209), S(73, 190), S(51, 181), S(119,188), S(158,164), S(127, 99) },
   { S(122, 85), S(164,163), S(72, 149), S(28, 172), S(5,  184), S(96, 181), S(164,162), S(128, 81) },
   { S(84,  24), S(114,128), S(64, 126), S(29, 129), S(31, 145), S(54, 134), S(107,119), S(91,  41) },
   { S(63,  -1), S(103, 73), S(48,  73), S(6,   68), S(-1,  83), S(49,  54), S(72,  79), S(44,  19) }
};

#undef S

Score psq[PIECE_NB][SQUARE_NB];

// init() initializes piece-square tables: the white halves of the tables are
// copied from Bonus[] adding the piece value, then the black halves of the
// tables are initialized by flipping and changing the sign of the white scores.
void init() {

  for (Piece pc = W_PAWN; pc <= W_KING; ++pc)
  {
      PieceValue[MG][~pc] = PieceValue[MG][pc];
      PieceValue[EG][~pc] = PieceValue[EG][pc];

      Score score = make_score(PieceValue[MG][pc], PieceValue[EG][pc]);

      for (Square s = SQ_A1; s <= SQ_H8; ++s)
      {
          File f = std::min(file_of(s), ~file_of(s));
          psq[ pc][ s] = score + (pc == W_KING ? KBonus[rank_of(s)][file_of(s)] : Bonus[pc][rank_of(s)][f]);
          psq[~pc][~s] = -psq[pc][s];
      }
  }
}

} // namespace PSQT
