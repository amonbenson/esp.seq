#pragma once


#define SEQ_PPQN 48 // should be a multiple of 16 to support at least 16th notes

#define SEQ_TICKS_PER_BAR (SEQ_PPQN * 4)
#define SEQ_TICKS_PER_HALF_NOTE (SEQ_TICKS_PER_BAR / 2)
#define SEQ_TICKS_PER_QUARTER_NOTE (SEQ_TICKS_PER_BAR / 4)
#define SEQ_TICKS_PER_EIGHTH_NOTE (SEQ_TICKS_PER_BAR / 8)
#define SEQ_TICKS_PER_SIXTEENTH_NOTE (SEQ_TICKS_PER_BAR / 16)
