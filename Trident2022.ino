/**************************************************************************
       This file is part of Trident2022.

    I, Dick Hamill, the author of this program disclaim all copyright
    in order to make this program freely available in perpetuity to
    anyone who would like to use it. Dick Hamill, 12/1/2020

    Trident2020 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Trident2020 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    See <https://www.gnu.org/licenses/>.
*/

// Scores not ending in zero (wizard mode)
// unstructured play jackpots
// increase mode start time with new qualifier

#include "BSOS_Config.h"
#include "BallySternOS.h"
#include "Trident2020.h"
#include "SelfTestAndAudit.h"
#include <EEPROM.h>

// Wav Trigger defines have been moved to BSOS_Config.h

#include "AudioHandler.h"

#define TRIDENT2022_MAJOR_VERSION  2022
#define TRIDENT2022_MINOR_VERSION  1
#define DEBUG_MESSAGES  0


/*********************************************************************

    Game specific code

*********************************************************************/

// MachineState
//  0 - Attract Mode
//  negative - self-test modes
//  positive - game play
char MachineState = 0;
boolean MachineStateChanged = true;
#define MACHINE_STATE_ATTRACT         0
#define MACHINE_STATE_INIT_GAMEPLAY   1
#define MACHINE_STATE_INIT_NEW_BALL   2
#define MACHINE_STATE_NORMAL_GAMEPLAY 4
#define MACHINE_STATE_COUNTDOWN_BONUS 99
#define MACHINE_STATE_BALL_OVER       100
#define MACHINE_STATE_MATCH_MODE      110

#ifdef BALLY_STERN_OS_CREDITS_PER_COIN_CONTROL
#define MACHINE_STATE_ADJUST_START  -20
#else
#define MACHINE_STATE_ADJUST_START  -17
#endif

#define MACHINE_STATE_ADJUST_FREEPLAY           (MACHINE_STATE_ADJUST_START-0)
#define MACHINE_STATE_ADJUST_BALL_SAVE          (MACHINE_STATE_ADJUST_START-1)
#define MACHINE_STATE_ADJUST_SOUND_SELECTOR     (MACHINE_STATE_ADJUST_START-2)
#define MACHINE_STATE_ADJUST_TOURNAMENT_SCORING (MACHINE_STATE_ADJUST_START-3)
#define MACHINE_STATE_ADJUST_TILT_WARNING       (MACHINE_STATE_ADJUST_START-4)
#define MACHINE_STATE_ADJUST_AWARD_OVERRIDE     (MACHINE_STATE_ADJUST_START-5)
#define MACHINE_STATE_ADJUST_BALLS_OVERRIDE     (MACHINE_STATE_ADJUST_START-6)
#define MACHINE_STATE_ADJUST_SCROLLING_SCORES   (MACHINE_STATE_ADJUST_START-7)
#define MACHINE_STATE_ADJUST_EXTRA_BALL_AWARD   (MACHINE_STATE_ADJUST_START-9)
#define MACHINE_STATE_ADJUST_SPECIAL_AWARD      (MACHINE_STATE_ADJUST_START-10)
#define MACHINE_STATE_ADJUST_DIM_LEVEL          (MACHINE_STATE_ADJUST_START-11)
#define MACHINE_STATE_ADJUST_DONE               (MACHINE_STATE_ADJUST_START-12)

#define GAME_MODE_SKILL_SHOT                    0
#define GAME_MODE_UNSTRUCTURED_PLAY             1
#define GAME_MODE_MINI_GAME_QUALIFIED           2
#define GAME_MODE_MINI_GAME_ENGAGED             3
#define GAME_MODE_MINI_GAME_REWARD_COUNTDOWN    4
#define GAME_MODE_WIZARD_INTRO                  5
#define GAME_MODE_WIZARD                        6

#define MINI_GAME_FEEDING_FRENZY_FLAG           0x01
#define MINI_GAME_SHARP_SHOOTER_FLAG            0x02
#define MINI_GAME_EXPLORE_THE_DEPTHS_FLAG       0x04


#define EEPROM_BALL_SAVE_BYTE           100
#define EEPROM_FREE_PLAY_BYTE           101
#define EEPROM_SOUND_SELECTOR_BYTE      102
#define EEPROM_SKILL_SHOT_BYTE          103
#define EEPROM_TILT_WARNING_BYTE        104
#define EEPROM_AWARD_OVERRIDE_BYTE      105
#define EEPROM_BALLS_OVERRIDE_BYTE      106
#define EEPROM_TOURNAMENT_SCORING_BYTE  107
#define EEPROM_SCROLLING_SCORES_BYTE    110
#define EEPROM_DIM_LEVEL_BYTE           113
#define EEPROM_EXTRA_BALL_SCORE_BYTE    140
#define EEPROM_SPECIAL_SCORE_BYTE       144

#define SOUND_EFFECT_NONE                     0
#define SOUND_EFFECT_DT_SKILL_SHOT            1
#define SOUND_EFFECT_ROLLOVER_SKILL_SHOT      2
#define SOUND_EFFECT_SU_SKILL_SHOT            3
#define SOUND_EFFECT_LEFT_SPINNER             4
#define SOUND_EFFECT_RIGHT_SPINNER            5
#define SOUND_EFFECT_ROLLOVER                 6
#define SOUND_EFFECT_LEFT_INLANE              7
#define SOUND_EFFECT_RIGHT_INLANE             8
#define SOUND_EFFECT_RIGHT_OUTLANE            9
#define SOUND_EFFECT_TOP_BUMPER_HIT           10
#define SOUND_EFFECT_BOTTOM_BUMPER_HIT        11
#define SOUND_EFFECT_DROP_TARGET              12
#define SOUND_EFFECT_ADD_CREDIT               13
#define SOUND_EFFECT_FEEDING_FRENZY           16
#define SOUND_EFFECT_ADD_PLAYER_1             20
#define SOUND_EFFECT_ADD_PLAYER_2             SOUND_EFFECT_ADD_PLAYER_1+1
#define SOUND_EFFECT_ADD_PLAYER_3             SOUND_EFFECT_ADD_PLAYER_1+2
#define SOUND_EFFECT_ADD_PLAYER_4             SOUND_EFFECT_ADD_PLAYER_1+3
#define SOUND_EFFECT_PLAYER_1_UP              24
#define SOUND_EFFECT_PLAYER_2_UP              SOUND_EFFECT_PLAYER_1_UP+1
#define SOUND_EFFECT_PLAYER_3_UP              SOUND_EFFECT_PLAYER_1_UP+2
#define SOUND_EFFECT_PLAYER_4_UP              SOUND_EFFECT_PLAYER_1_UP+3
#define SOUND_EFFECT_BALL_OVER                30
#define SOUND_EFFECT_GAME_OVER                31
#define SOUND_EFFECT_BONUS_COUNT              32
#define SOUND_EFFECT_2X_BONUS_COUNT           33
#define SOUND_EFFECT_3X_BONUS_COUNT           34
#define SOUND_EFFECT_4X_BONUS_COUNT           35
#define SOUND_EFFECT_5X_BONUS_COUNT           36
#define SOUND_EFFECT_TILT_WARNING             39
#define SOUND_EFFECT_MATCH_SPIN               40
#define SOUND_EFFECT_LOWER_SLING              41
#define SOUND_EFFECT_UPPER_SLING              42
#define SOUND_EFFECT_10PT_SWITCH              43
#define SOUND_EFFECT_SAUCER_HIT_5K            44
#define SOUND_EFFECT_SAUCER_HIT_10K           45
#define SOUND_EFFECT_SAUCER_HIT_20K           46
#define SOUND_EFFECT_SAUCER_HIT_30K           47
#define SOUND_EFFECT_SHARP_SHOOTER_HIT        48
#define SOUND_EFFECT_DROP_TARGET_CLEAR_1      50
#define SOUND_EFFECT_DROP_TARGET_CLEAR_2      SOUND_EFFECT_DROP_TARGET_CLEAR_1+1
#define SOUND_EFFECT_DROP_TARGET_CLEAR_3      SOUND_EFFECT_DROP_TARGET_CLEAR_1+2
#define SOUND_EFFECT_DROP_TARGET_CLEAR_4      SOUND_EFFECT_DROP_TARGET_CLEAR_1+3
#define SOUND_EFFECT_DROP_TARGET_CLEAR_5      SOUND_EFFECT_DROP_TARGET_CLEAR_1+4
#define SOUND_EFFECT_FIRST_SU_SWITCH_HIT      55
#define SOUND_EFFECT_SECOND_SU_SWITCH_HIT     SOUND_EFFECT_FIRST_SU_SWITCH_HIT+1
#define SOUND_EFFECT_THIRD_SU_SWITCH_HIT      SOUND_EFFECT_FIRST_SU_SWITCH_HIT+2
#define SOUND_EFFECT_FOURTH_SU_SWITCH_HIT     SOUND_EFFECT_FIRST_SU_SWITCH_HIT+3
#define SOUND_EFFECT_FIFTH_SU_SWITCH_HIT      SOUND_EFFECT_FIRST_SU_SWITCH_HIT+4
#define SOUND_EFFECT_STANDUPS_CLEARED         61
#define SOUND_EFFECT_EXPLORE_HIT              62
#define SOUND_EFFECT_MODE_FINISHED            67

#define SOUND_EFFECT_TRIDENT_INTRO            89
#define SOUND_EFFECT_BACKGROUND_1             90
#define SOUND_EFFECT_BACKGROUND_2             91
#define SOUND_EFFECT_BACKGROUND_3             92
#define SOUND_EFFECT_BACKGROUND_4             93
#define SOUND_EFFECT_BACKGROUND_5             94
#define SOUND_EFFECT_BACKGROUND_6             95
#define SOUND_EFFECT_BACKGROUND_WIZ           96
#define SOUND_EFFECT_BACKGROUND_FOR_SINGLE_MODE     97
#define SOUND_EFFECT_BACKGROUND_FOR_DOUBLE_MODE     98
#define SOUND_EFFECT_BACKGROUND_FOR_TRIPLE_MODE     99
#define SOUND_EFFECT_COIN_DROP_1              100
#define SOUND_EFFECT_COIN_DROP_2              101
#define SOUND_EFFECT_COIN_DROP_3              102

// Game play status callouts
#define NUM_VOICE_NOTIFICATIONS                 16
byte VoiceNotificationDurations[NUM_VOICE_NOTIFICATIONS] = {
  4, 3, 3, 3, 2, 3, 4, 4, 3, 3,
  5, 5, 5, 3, 2, 3
};
#define SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START 500

#define SOUND_EFFECT_VP_RESCUE_FROM_THE_DEEP     500
#define SOUND_EFFECT_VP_SHOOT_AGAIN              501
#define SOUND_EFFECT_VP_FEEDING_FRENZY_START     502
#define SOUND_EFFECT_VP_SHARP_SHOOTER_START      503
#define SOUND_EFFECT_VP_JACKPOT                  504
#define SOUND_EFFECT_VP_EXTRA_BALL               505
#define SOUND_EFFECT_VP_EXPLORE_QUALIFIED        506
#define SOUND_EFFECT_VP_EXPLORE_THE_DEPTHS_START 507
#define SOUND_EFFECT_VP_SHARP_SHOOTER_QUALIFIED  508
#define SOUND_EFFECT_VP_FEEDING_FRENZY_QUALIFIED 509
#define SOUND_EFFECT_VP_SS_AND_FF_START          510
#define SOUND_EFFECT_VP_SS_AND_ETD_START         511
#define SOUND_EFFECT_VP_FF_AND_ETD_START         512
#define SOUND_EFFECT_VP_MEGA_STACK_START         513
#define SOUND_EFFECT_VP_SWIM_AGAIN               514
#define SOUND_EFFECT_VP_DEEP_BLUE_SEA_MODE       515

#define STANDUP_PURPLE_MASK     0x01
#define STANDUP_YELLOW_MASK     0x02
#define STANDUP_AMBER_MASK      0x04
#define STANDUP_GREEN_MASK      0x08
#define STANDUP_WHITE_MASK      0x10

#define SKILL_SHOT_DURATION 15
#define MAX_DISPLAY_BONUS     55
#define TILT_WARNING_DEBOUNCE_TIME      1000
#define BONUS_UNDERLIGHTS_OFF   0
#define BONUS_UNDERLIGHTS_DIM   1
#define BONUS_UNDERLIGHTS_FULL  2
#define SAUCER_DISPLAY_DURATION         1000
#define MODE_START_DISPLAY_DURATION     5000
#define DROP_TARGET_CLEAR_DURATION      1000
#define STANDUP_HIT_DISPLAY_DURATION    5000
#define MAX_DROP_TARGET_CLEAR_DEADLINE  5000
#define ROLLOVER_FLASH_DURATION         2000
#define RESCUE_FROM_THE_DEEP_TIME       6000
#define FEEDING_FRENZY_ALTERNATE_TIME   30000
#define MODE_QUALIFY_TIME               45000
#define SHARP_SHOOTER_TARGET_TIME       5000
#define MINI_GAME_SINGLE_DURATION       40000
#define MINI_GAME_DOUBLE_DURATION       66000
#define MINI_GAME_TRIPLE_DURATION       107000
#define WIZARD_MODE_DURATION            110000



/*********************************************************************

    Machine state and options

*********************************************************************/
byte Credits = 0;
byte SoundSelector = 3;
byte BallSaveNumSeconds = 0;
byte MaximumCredits = 40;
byte BallsPerGame = 3;
byte DimLevel = 2;
byte ScoreAwardReplay = 0;
byte SpecialLightAward = 0;
byte TotalBallsLoaded = 4;
byte NumberOfBallsInPlay = 0;
boolean HighScoreReplay = true;
boolean MatchFeature = true;
boolean TournamentScoring = false;
boolean ResetScoresToClearVersion = false;
boolean ScrollingScores = true;
boolean FreePlayMode = false;
unsigned long HighScore = 0;
unsigned long AwardScores[3];
unsigned long ExtraBallValue = 0;
unsigned long SpecialValue = 0;
unsigned long CurrentTime = 0;
unsigned long BallSaveEndTime = 0;
//byte dipBank0, dipBank1, dipBank2, dipBank3;
//int BackgroundMusicGain = -3;


/*********************************************************************

    Game State

*********************************************************************/
byte CurrentPlayer = 0;
byte CurrentBallInPlay = 1;
byte CurrentNumPlayers = 0;
byte Bonus;
byte BonusX;
byte StandupsHit[4];
byte CurrentStandupsHit = 0;

byte GameMode = GAME_MODE_SKILL_SHOT;
byte MaxTiltWarnings = 2;
byte NumTiltWarnings = 0;
byte SaucerValue = 0;
byte ShowSaucerHit = 0;
byte LastStandupTargetHit = 0;
byte CurrentDropTargetsValid = 0;
byte RolloverValue = 2;
byte LastSpinnerSide = 0; // 1=left, 2=right
byte AlternatingSpinnerCount = 0;
byte FeedingFrenzySpins[4];
byte ExploreTheDepthsHits[4];
byte SharpShooterHits[4];
byte CurrentFeedingFrenzy;
byte CurrentExploreTheDepths;
byte CurrentSharpShooter; 
byte SharpShooterTarget = 0;
byte NumberOfStandupClears = 0;
byte FeedingFrenzyAlternatingStart = 4;
byte SharpShooterStartBonus = 3;
byte TargetSpecialBonus = 4;
byte StandupSpecialLevel = 2;
byte ExploreTheDepthsStart = 1;
byte MiniGamesFlagsQualified = 0;
byte MiniGamesRunning = 0;
byte NumAlternatingSpinnersRequired[4];
byte MusicVolume = 10;
byte SoundEffectsVolume = 10;
byte CalloutsVolume = 10;

boolean SamePlayerShootsAgain = false;
boolean RescueFromTheDeepAvailable = true;
boolean JackpotLit = false;
boolean ExtraBallCollected = false;
boolean SpecialCollected = false;
boolean ShowingModeStats = false;

unsigned long CurrentScores[4];
unsigned long BallFirstSwitchHitTime = 0;
unsigned long BallTimeInTrough = 0;
unsigned long AutoPlungeTime = 0;
unsigned long LastSpinnerHitTime = 0;
unsigned long GameModeStartTime = 0;
unsigned long GameModeEndTime = 0;
unsigned long LastTiltWarningTime = 0;
unsigned long NextSaucerReduction = 0;
unsigned long SaucerHitTime = 0;
unsigned long DropTargetClearTime = 0;
unsigned long StandupDisplayEndTime = 0;
unsigned long RolloverFlashEndTime = 0;
unsigned long RescueFromTheDeepEndTime = 0;
unsigned long LastMiniGameBonusTime = 0;

AudioHandler Audio;

#define BALL_SAVE_GRACE_PERIOD  2000

/*
void GetDIPSwitches() {
  dipBank0 = BSOS_GetDipSwitches(0);
  dipBank1 = BSOS_GetDipSwitches(1);
  dipBank2 = BSOS_GetDipSwitches(2);
  dipBank3 = BSOS_GetDipSwitches(3);
}

void DecodeDIPSwitchParameters() {

}
*/
void ReadStoredParameters() {
  HighScore = BSOS_ReadULFromEEProm(BSOS_HIGHSCORE_EEPROM_START_BYTE, 10000);
  Credits = BSOS_ReadByteFromEEProm(BSOS_CREDITS_EEPROM_BYTE);
  if (Credits > MaximumCredits) Credits = MaximumCredits;

  ReadSetting(EEPROM_FREE_PLAY_BYTE, 0);
  FreePlayMode = (EEPROM.read(EEPROM_FREE_PLAY_BYTE)) ? true : false;

  BallSaveNumSeconds = ReadSetting(EEPROM_BALL_SAVE_BYTE, 15);
  if (BallSaveNumSeconds > 20) BallSaveNumSeconds = 20;

  SoundSelector = ReadSetting(EEPROM_SOUND_SELECTOR_BYTE, 3);
  if (SoundSelector > 3) SoundSelector = 3;

  Audio.SetMusicVolume(MusicVolume);
  Audio.SetSoundFXVolume(SoundEffectsVolume);
  Audio.SetNotificationsVolume(CalloutsVolume);

  TournamentScoring = (ReadSetting(EEPROM_TOURNAMENT_SCORING_BYTE, 0)) ? true : false;

  MaxTiltWarnings = ReadSetting(EEPROM_TILT_WARNING_BYTE, 2);
  if (MaxTiltWarnings > 2) MaxTiltWarnings = 2;

  byte awardOverride = ReadSetting(EEPROM_AWARD_OVERRIDE_BYTE, 99);
  if (awardOverride != 99) {
    ScoreAwardReplay = awardOverride;
  }

  byte ballsOverride = ReadSetting(EEPROM_BALLS_OVERRIDE_BYTE, 99);
  if (ballsOverride == 3 || ballsOverride == 5) {
    BallsPerGame = ballsOverride;
  } else {
    if (ballsOverride != 99) EEPROM.write(EEPROM_BALLS_OVERRIDE_BYTE, 99);
  }

  ScrollingScores = (ReadSetting(EEPROM_SCROLLING_SCORES_BYTE, 1)) ? true : false;

  ExtraBallValue = BSOS_ReadULFromEEProm(EEPROM_EXTRA_BALL_SCORE_BYTE);
  if (ExtraBallValue % 1000 || ExtraBallValue > 100000) ExtraBallValue = 20000;

  SpecialValue = BSOS_ReadULFromEEProm(EEPROM_SPECIAL_SCORE_BYTE);
  if (SpecialValue % 1000 || SpecialValue > 100000) SpecialValue = 40000;

  DimLevel = ReadSetting(EEPROM_DIM_LEVEL_BYTE, 2);
  if (DimLevel < 2 || DimLevel > 3) DimLevel = 2;
  BSOS_SetDimDivisor(1, DimLevel);

  AwardScores[0] = BSOS_ReadULFromEEProm(BSOS_AWARD_SCORE_1_EEPROM_START_BYTE);
  AwardScores[1] = BSOS_ReadULFromEEProm(BSOS_AWARD_SCORE_2_EEPROM_START_BYTE);
  AwardScores[2] = BSOS_ReadULFromEEProm(BSOS_AWARD_SCORE_3_EEPROM_START_BYTE);

}


void setup() {
  if (DEBUG_MESSAGES) {
    Serial.begin(115200);
  }

  // Tell the OS about game-specific lights and switches
  BSOS_SetupGameSwitches(NUM_SWITCHES_WITH_TRIGGERS, NUM_PRIORITY_SWITCHES_WITH_TRIGGERS, TriggeredSwitches);

  // Set up the chips and interrupts
  BSOS_InitializeMPU();
  BSOS_DisableSolenoidStack();
  BSOS_SetDisableFlippers(true);

  // Use dip switches to set up game variables
//  GetDIPSwitches();
//  DecodeDIPSwitchParameters();

  // Read parameters from EEProm
  ReadStoredParameters();

  CurrentScores[0] = TRIDENT2022_MAJOR_VERSION;
  CurrentScores[1] = TRIDENT2022_MINOR_VERSION;
  CurrentScores[2] = BALLY_STERN_OS_MAJOR_VERSION;
  CurrentScores[3] = BALLY_STERN_OS_MINOR_VERSION;
  ResetScoresToClearVersion = true;

  Audio.InitDevices(AUDIO_PLAY_TYPE_WAV_TRIGGER | AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS);
  Audio.StopAllAudio();
  delayMicroseconds(10000);
  Audio.SetMusicDuckingGain(10);
  Audio.PlaySound(SOUND_EFFECT_TRIDENT_INTRO, AUDIO_PLAY_TYPE_WAV_TRIGGER);

  CurrentTime = millis();
}

byte ReadSetting(byte setting, byte defaultValue) {
  byte value = EEPROM.read(setting);
  if (value == 0xFF) {
    EEPROM.write(setting, defaultValue);
    return defaultValue;
  }
  return value;
}

void SetGameMode(byte newGameMode) {
  GameMode = newGameMode;
  GameModeStartTime = 0;
  GameModeEndTime = 0;
  if (DEBUG_MESSAGES) {
    char buf[129];
    sprintf(buf, "Game mode set to %d\n", newGameMode);
    Serial.write(buf);
  }
}


byte CountBallsInTrough() {
  byte numBalls = BSOS_ReadSingleSwitchState(SW_OUTHOLE) + BSOS_ReadSingleSwitchState(SW_BALL_3) + 
                  BSOS_ReadSingleSwitchState(SW_BALL_2) + BSOS_ReadSingleSwitchState(SW_BALL_1) + 
                  BSOS_ReadSingleSwitchState(SW_BALL_JAM);

  return numBalls;
}


////////////////////////////////////////////////////////////////////////////
//
//  Lamp Management functions
//
////////////////////////////////////////////////////////////////////////////
void SetPlayerLamps(byte numPlayers, byte playerOffset = 0, int flashPeriod = 0) {
  for (int count = 0; count < 4; count++) {
    BSOS_SetLampState(PLAYER_1 + playerOffset + count, (numPlayers == (count + 1)) ? 1 : 0, 0, flashPeriod);
  }
}


void ShowBonusOnTree(byte bonus, byte dim=0) {
  if (bonus>MAX_DISPLAY_BONUS) bonus = MAX_DISPLAY_BONUS;
  
  byte cap = 10;

  for (byte turnOff=(bonus+1); turnOff<11; turnOff++) {
    BSOS_SetLampState(BONUS_1 + (turnOff-1), 0);
  }
  if (bonus==0) return;

  if (bonus>=cap) {
    while(bonus>=cap) {
      BSOS_SetLampState(BONUS_1 + (cap-1), 1, dim, 250);
      bonus -= cap;
      cap -= 1;
      if (cap==0) {
        bonus = 0;
        break;
      }
    }
    for (byte turnOff=(bonus+1); turnOff<(cap+1); turnOff++) {
      BSOS_SetLampState(BONUS_1 + (turnOff-1), 0);
    }
  }

  byte bottom; 
  for (bottom=1; bottom<bonus; bottom++){
    BSOS_SetLampState(BONUS_1 + (bottom-1), 0);
  }

  if (bottom<=cap) {
    BSOS_SetLampState(BONUS_1 + (bottom-1), 1, 0);
  }  
  
}


void ShowSaucerLamps() {

  if (GameMode==GAME_MODE_MINI_GAME_QUALIFIED) {
    byte lampPhase = ((CurrentTime-GameModeStartTime)/100)%4;
    for (byte count=0; count<4; count++) {
      BSOS_SetLampState(TOP_EJECT_5K-count, count==lampPhase);
    }
  } else if (SaucerHitTime!=0 && (CurrentTime-SaucerHitTime)<SAUCER_DISPLAY_DURATION) {
    byte saucerLamp = 0;
    if (ShowSaucerHit>5) saucerLamp = ShowSaucerHit/10;
    for (int count=0; count<4; count++) {
      if (count==saucerLamp) BSOS_SetLampState(TOP_EJECT_5K-count, 1, 0, 125);
      else BSOS_SetLampState(TOP_EJECT_5K-count, 0);
    }
  } else if (GameMode==GAME_MODE_SKILL_SHOT) {
    byte lampPhase = ((CurrentTime - GameModeStartTime)/250)%28;
    if (lampPhase<16) {
      BSOS_SetLampState(TOP_EJECT_5K, lampPhase%4, lampPhase%2);
      SaucerValue = 5;
      for (int count=1; count<4; count++) BSOS_SetLampState(TOP_EJECT_5K-count, 0);
    } else {
      byte saucerLamp;
      lampPhase -= 16;
      saucerLamp = lampPhase%6;
      if (saucerLamp>3) saucerLamp = 6-saucerLamp;
      BSOS_SetLampState(TOP_EJECT_5K - saucerLamp, 1);
      for (int count=0; count<4; count++) if (saucerLamp!=count) BSOS_SetLampState(TOP_EJECT_5K-count, 0);
      if (saucerLamp) SaucerValue = 10*saucerLamp;
      else SaucerValue = 5;
    }
  } else if (GameMode==GAME_MODE_WIZARD) {
    byte lampPhase = ((CurrentTime-GameModeStartTime)/175)%4;
    if (!JackpotLit) lampPhase = 0;
    for (int count=0; count<4; count++) {
      BSOS_SetLampState(TOP_EJECT_5K-count, lampPhase, (lampPhase%2));
    }
  } else {
    if (NextSaucerReduction==0) {
      BSOS_SetLampState(TOP_EJECT_5K, 1);
      for (int count=1; count<4; count++) BSOS_SetLampState(TOP_EJECT_5K-count, 0);
      SaucerValue = 5;      
    } else if (CurrentTime<NextSaucerReduction) {
      byte saucerLamp = 0;
      if (SaucerValue>5) saucerLamp = SaucerValue/10;
      for (int count=0; count<4; count++) {
        if (count!=saucerLamp) BSOS_SetLampState(TOP_EJECT_5K-count, 0);
        else BSOS_SetLampState(TOP_EJECT_5K-count, 1, 0, ((NextSaucerReduction-CurrentTime)/2000)*100 + 200);
      }
    } else {      
      if (SaucerValue>10) {
        SaucerValue -= 10;
        NextSaucerReduction = CurrentTime + 10000;    
      } else if (SaucerValue==10) {
        SaucerValue = 5;
        NextSaucerReduction = 0;
      }
    }
  }
}

byte DropTargetLampArray[] = {DROP_TARGET_1, DROP_TARGET_2, DROP_TARGET_3, DROP_TARGET_4, DROP_TARGET_5};
byte DropTargetSwitchArray[] = {SW_DROP_TARGET_1, SW_DROP_TARGET_2, SW_DROP_TARGET_3, SW_DROP_TARGET_4, SW_DROP_TARGET_5};

void ShowDropTargetLamps() {

  if (GameMode==GAME_MODE_MINI_GAME_QUALIFIED) {
    byte lampPhase = 0xFF;
    if (MiniGamesFlagsQualified&MINI_GAME_SHARP_SHOOTER_FLAG) lampPhase = ((CurrentTime-GameModeStartTime)/250)%9;
    for (byte count=0; count<4; count++) {
      BSOS_SetLampState(DROP_TARGET_1 - count, lampPhase==0, 1);
      BSOS_SetLampState(BONUS_2X_FEATURE - count, 0);
    }
    BSOS_SetLampState(DROP_TARGET_5, lampPhase==0, 1);
  } else if (MiniGamesRunning&MINI_GAME_SHARP_SHOOTER_FLAG || (DropTargetClearTime!=0 && (CurrentTime-DropTargetClearTime)<DROP_TARGET_CLEAR_DURATION)) {
    byte lampPhase = ((CurrentTime-DropTargetClearTime)/50)%5;
    for (byte count=0; count<5; count++) {
      BSOS_SetLampState(DropTargetLampArray[count], lampPhase==count);
    }
/*    
    BSOS_SetLampState(DROP_TARGET_1, lampPhase==0);
    BSOS_SetLampState(DROP_TARGET_2, lampPhase==1);
    BSOS_SetLampState(DROP_TARGET_3, lampPhase==2);
    BSOS_SetLampState(DROP_TARGET_4, lampPhase==3);
    BSOS_SetLampState(DROP_TARGET_5, lampPhase==4);
*/
    for (byte count=0; count<4; count++) {
      BSOS_SetLampState(BONUS_2X_FEATURE - count, 0);
    }
  } else if (GameMode==GAME_MODE_SKILL_SHOT) {
    byte lampPhase = ((CurrentTime-GameModeStartTime)/110)%8;
    for (byte count=0; count<5; count++) {
//      BSOS_SetLampState(DropTargetLampArray[count], (count==lampPhase)||(count==((7+lampPhase)&0x07))||(count==(7-lampPhase))||(count==((0-lampPhase)&0x07)), (count==lampPhase)||(count==((0-lampPhase)&0x07)));
      BSOS_SetLampState(DropTargetLampArray[count], (count==lampPhase)||(count==(8-lampPhase)));
    }
/*
    1 - lampPhase==7||lampPhase==0||lampPhase==7||lampPhase==0, lampPhase==0||lampPhase==0
    2 - lampPhase==0||lampPhase==1||lampPhase==6||lampPhase==7, lampPhase==1||lampPhase==7
    3 - lampPhase==1||lampPhase==2||lampPhase==5||lampPhase==6, lampPhase==2||lampPhase==6
    4 - lampPhase==2||lampPhase==3||lampPhase==4||lampPhase==5, lampPhase==3||lampPhase==5
    4 - lampPhase==3||lampPhase==4||lampPhase==3||lampPhase==4, lampPhase==4||lampPhase==4
    
    BSOS_SetLampState(DROP_TARGET_1, lampPhase==0);
    BSOS_SetLampState(DROP_TARGET_2, lampPhase%2, 1);
    BSOS_SetLampState(DROP_TARGET_3, lampPhase==2);
    BSOS_SetLampState(DROP_TARGET_4, lampPhase%2, 1);
    BSOS_SetLampState(DROP_TARGET_5, lampPhase==0);    
*/
    for (byte count=0; count<4; count++) {
      BSOS_SetLampState(BONUS_2X_FEATURE - count, 0);
    }
  } else {
    for (byte count=0; count<5; count++) {
      BSOS_SetLampState(DropTargetLampArray[count], BSOS_ReadSingleSwitchState(DropTargetSwitchArray[count])?0:1);
    }
/*    
    BSOS_SetLampState(DROP_TARGET_1, BSOS_ReadSingleSwitchState(SW_DROP_TARGET_1)?0:1);
    BSOS_SetLampState(DROP_TARGET_2, BSOS_ReadSingleSwitchState(SW_DROP_TARGET_2)?0:1);
    BSOS_SetLampState(DROP_TARGET_3, BSOS_ReadSingleSwitchState(SW_DROP_TARGET_3)?0:1);
    BSOS_SetLampState(DROP_TARGET_4, BSOS_ReadSingleSwitchState(SW_DROP_TARGET_4)?0:1);
    BSOS_SetLampState(DROP_TARGET_5, BSOS_ReadSingleSwitchState(SW_DROP_TARGET_5)?0:1);
*/    
    for (byte count=0; count<4; count++) {
      BSOS_SetLampState(BONUS_2X_FEATURE - count, BonusX==(count+1));
    }
  }
}

void ShowStandupTargetLamps() {
  if (GameMode==GAME_MODE_MINI_GAME_QUALIFIED) {
    byte lampPhase = 0xFF;
    if (MiniGamesFlagsQualified&MINI_GAME_EXPLORE_THE_DEPTHS_FLAG) lampPhase = ((CurrentTime-GameModeStartTime)/250)%9;
    BSOS_SetLampState(STAND_UP_PURPLE, (lampPhase==3), 1);
    BSOS_SetLampState(STAND_UP_YELLOW, (lampPhase==3), 1);
    BSOS_SetLampState(STAND_UP_AMBER, (lampPhase==3), 1);
    BSOS_SetLampState(STAND_UP_GREEN, (lampPhase==3), 1);
    BSOS_SetLampState(STAND_UP_WHITE, (lampPhase==3), 1);
  } else if (!(MiniGamesRunning&MINI_GAME_EXPLORE_THE_DEPTHS_FLAG) && StandupDisplayEndTime!=0 && CurrentTime<StandupDisplayEndTime) {
    BSOS_SetLampState(STAND_UP_PURPLE, CurrentStandupsHit&STANDUP_PURPLE_MASK, (LastStandupTargetHit&STANDUP_PURPLE_MASK)?0:1, (LastStandupTargetHit&STANDUP_PURPLE_MASK)?50:0);
    BSOS_SetLampState(STAND_UP_YELLOW, CurrentStandupsHit&STANDUP_YELLOW_MASK, (LastStandupTargetHit&STANDUP_YELLOW_MASK)?0:1, (LastStandupTargetHit&STANDUP_YELLOW_MASK)?50:0);
    BSOS_SetLampState(STAND_UP_AMBER, CurrentStandupsHit&STANDUP_AMBER_MASK, (LastStandupTargetHit&STANDUP_AMBER_MASK)?0:1, (LastStandupTargetHit&STANDUP_AMBER_MASK)?50:0);
    BSOS_SetLampState(STAND_UP_GREEN, CurrentStandupsHit&STANDUP_GREEN_MASK, (LastStandupTargetHit&STANDUP_GREEN_MASK)?0:1, (LastStandupTargetHit&STANDUP_GREEN_MASK)?50:0);
    BSOS_SetLampState(STAND_UP_WHITE, CurrentStandupsHit&STANDUP_WHITE_MASK, (LastStandupTargetHit&STANDUP_WHITE_MASK)?0:1, (LastStandupTargetHit&STANDUP_WHITE_MASK)?50:0);
  } else if (GameMode==GAME_MODE_SKILL_SHOT || (MiniGamesRunning&MINI_GAME_EXPLORE_THE_DEPTHS_FLAG)) {
    byte lampPhase = ((CurrentTime-GameModeStartTime)/100)%5;
    BSOS_SetLampState(STAND_UP_PURPLE, lampPhase==4||lampPhase==0, lampPhase==0);
    BSOS_SetLampState(STAND_UP_YELLOW, lampPhase==3||lampPhase==4, lampPhase==4);
    BSOS_SetLampState(STAND_UP_AMBER, lampPhase==2||lampPhase==3, lampPhase==3);
    BSOS_SetLampState(STAND_UP_GREEN, lampPhase==1||lampPhase==2, lampPhase==2);
    BSOS_SetLampState(STAND_UP_WHITE, lampPhase<2, lampPhase==1);
  } else {
    BSOS_SetLampState(STAND_UP_PURPLE, CurrentStandupsHit&STANDUP_PURPLE_MASK);
    BSOS_SetLampState(STAND_UP_YELLOW, CurrentStandupsHit&STANDUP_YELLOW_MASK);
    BSOS_SetLampState(STAND_UP_AMBER, CurrentStandupsHit&STANDUP_AMBER_MASK);
    BSOS_SetLampState(STAND_UP_GREEN, CurrentStandupsHit&STANDUP_GREEN_MASK);
    BSOS_SetLampState(STAND_UP_WHITE, CurrentStandupsHit&STANDUP_WHITE_MASK);
  }
  
}


byte LastBonusShown = 0;
void ShowBonusLamps() {
  if (GameMode==GAME_MODE_MINI_GAME_QUALIFIED) {
    byte lightPhase = ((CurrentTime-GameModeStartTime)/100)%15;
    for (byte count=0; count<10; count++) {
      BSOS_SetLampState(BONUS_1+count, (lightPhase==count)||((lightPhase-1)==count), ((lightPhase-1)==count));
    }
  } else if (Bonus!=LastBonusShown) {
    LastBonusShown = Bonus;
    ShowBonusOnTree(Bonus);
  }
}

void ShowBonusXLamps() {
  if (GameMode==GAME_MODE_MINI_GAME_QUALIFIED || (MiniGamesRunning&MINI_GAME_SHARP_SHOOTER_FLAG)) { 
    for (int count=2; count<6; count++) {
      BSOS_SetLampState(BONUS_2X-(count-2), 0);
    }
  } else {
    for (int count=2; count<6; count++) {
      BSOS_SetLampState(BONUS_2X-(count-2), (count==BonusX));
    }
  }
}

void ShowLeftSpinnerLamps() {
  if (GameMode==GAME_MODE_MINI_GAME_QUALIFIED) {
    byte lampPhase = 0xFF;
    if (MiniGamesFlagsQualified&MINI_GAME_FEEDING_FRENZY_FLAG) lampPhase = ((CurrentTime-GameModeStartTime)/250)%9;
    BSOS_SetLampState(LEFT_SPINNER_AMBER, (lampPhase==6), 1);
    BSOS_SetLampState(LEFT_SPINNER_WHITE, (lampPhase==6), 1);
    BSOS_SetLampState(LEFT_SPINNER_PURPLE, (lampPhase==6), 1);
  } else if (GameMode==GAME_MODE_SKILL_SHOT) {
    byte lampPhase = ((CurrentTime-GameModeStartTime)/600)%3;
    BSOS_SetLampState(LEFT_SPINNER_AMBER, lampPhase==0);
    BSOS_SetLampState(LEFT_SPINNER_WHITE, lampPhase==1);
    BSOS_SetLampState(LEFT_SPINNER_PURPLE, lampPhase==2);
  } else {
    if ((MiniGamesRunning&MINI_GAME_FEEDING_FRENZY_FLAG) || (LastSpinnerHitTime!=0 && LastSpinnerSide==2 && (CurrentTime-LastSpinnerHitTime)<MODE_QUALIFY_TIME)) {
      byte lampPhase = ((CurrentTime-GameModeStartTime)/100)%3;
      BSOS_SetLampState(LEFT_SPINNER_AMBER, lampPhase==2);
      BSOS_SetLampState(LEFT_SPINNER_WHITE, lampPhase==1);
      BSOS_SetLampState(LEFT_SPINNER_PURPLE, lampPhase==0);
    } else {      
      int flashFrequency = 200;
      if ((StandupDisplayEndTime-CurrentTime)<1000) flashFrequency = 100;
      BSOS_SetLampState(LEFT_SPINNER_AMBER, CurrentStandupsHit&STANDUP_AMBER_MASK, 0, (LastStandupTargetHit&STANDUP_AMBER_MASK)?flashFrequency:0);
      BSOS_SetLampState(LEFT_SPINNER_WHITE, CurrentStandupsHit&STANDUP_WHITE_MASK, 0, (LastStandupTargetHit&STANDUP_WHITE_MASK)?flashFrequency:0);
      BSOS_SetLampState(LEFT_SPINNER_PURPLE, CurrentStandupsHit&STANDUP_PURPLE_MASK, 0,(LastStandupTargetHit&STANDUP_PURPLE_MASK)?flashFrequency:0);
    }
  }
}

void ShowRightSpinnerLamps() {
  if (GameMode==GAME_MODE_MINI_GAME_QUALIFIED || GameMode==GAME_MODE_SKILL_SHOT) {
    byte lampPhase = 0xFF;
    if (MiniGamesFlagsQualified&MINI_GAME_FEEDING_FRENZY_FLAG) lampPhase = ((CurrentTime-GameModeStartTime)/250)%9;
    // No skill shot for right spinner
    BSOS_SetLampState(RIGHT_SPINNER_YELLOW, (lampPhase==6), 1);
    BSOS_SetLampState(RIGHT_SPINNER_GREEN, (lampPhase==6), 1);
    BSOS_SetLampState(RIGHT_SPINNER_PURPLE, (lampPhase==6), 1);
  } else {
    if ( (MiniGamesRunning&MINI_GAME_FEEDING_FRENZY_FLAG) || (LastSpinnerHitTime!=0 && LastSpinnerSide==1 && (CurrentTime-LastSpinnerHitTime)<MODE_QUALIFY_TIME)) {
      byte lampPhase = ((CurrentTime-GameModeStartTime)/100)%3;
      BSOS_SetLampState(RIGHT_SPINNER_YELLOW, lampPhase==2);
      BSOS_SetLampState(RIGHT_SPINNER_GREEN, lampPhase==1);
      BSOS_SetLampState(RIGHT_SPINNER_PURPLE, lampPhase==0);
    } else {      
      int flashFrequency = 200;
      if ((StandupDisplayEndTime-CurrentTime)<1000) flashFrequency = 100;
      BSOS_SetLampState(RIGHT_SPINNER_YELLOW, CurrentStandupsHit&STANDUP_YELLOW_MASK, 0, (LastStandupTargetHit&STANDUP_YELLOW_MASK)?flashFrequency:0);
      BSOS_SetLampState(RIGHT_SPINNER_GREEN, CurrentStandupsHit&STANDUP_GREEN_MASK, 0, (LastStandupTargetHit&STANDUP_GREEN_MASK)?flashFrequency:0);
      BSOS_SetLampState(RIGHT_SPINNER_PURPLE, CurrentStandupsHit&STANDUP_PURPLE_MASK, 0, (LastStandupTargetHit&STANDUP_PURPLE_MASK)?flashFrequency:0);
    }
  }
}

void ShowLeftLaneLamps() {
  byte valueToShow = RolloverValue;
  int valueFlash = 0;
  if (GameMode==GAME_MODE_MINI_GAME_QUALIFIED) {
    valueToShow = 0;
  } else if (GameMode==GAME_MODE_SKILL_SHOT) {
    valueToShow = 8;
    valueFlash = 500;
  } else {
    if (RolloverFlashEndTime!=0 && CurrentTime<RolloverFlashEndTime) valueFlash = 100;
  }
  
  BSOS_SetLampState(LEFT_LANE_2K, (valueToShow==2||valueToShow==10), 0, valueFlash);  
  BSOS_SetLampState(LEFT_LANE_4K, (valueToShow==4||valueToShow==12), 0, valueFlash);  
  BSOS_SetLampState(LEFT_LANE_6K, (valueToShow==6||valueToShow==14), 0, valueFlash);  
  BSOS_SetLampState(LEFT_LANE_8K, (valueToShow>6), 0, valueFlash);  
}

void ShowAwardLamps() {
  BSOS_SetLampState(EXTRA_BALL, ((NumberOfStandupClears==1&&!ExtraBallCollected)||RescueFromTheDeepEndTime!=0), 0, (RescueFromTheDeepEndTime)?100:0);    
  BSOS_SetLampState(DROP_TARGET_SPECIAL, (BonusX==(TargetSpecialBonus-1)) && ! (MiniGamesRunning&MINI_GAME_SHARP_SHOOTER_FLAG));
  BSOS_SetLampState(STAND_UP_SPECIAL, (NumberOfStandupClears==(StandupSpecialLevel-1)) && !(MiniGamesRunning&MINI_GAME_EXPLORE_THE_DEPTHS_FLAG));
  BSOS_SetLampState(RIGHT_OUTLANE_SPECIAL, (NumberOfStandupClears==StandupSpecialLevel && !SpecialCollected));  
}


void ShowShootAgainLamp() {

  if ( (BallFirstSwitchHitTime==0 && BallSaveNumSeconds) || (BallSaveEndTime && CurrentTime<BallSaveEndTime) ) {
    unsigned long msRemaining = 5000;
    if (BallSaveEndTime!=0) msRemaining = BallSaveEndTime - CurrentTime;
    BSOS_SetLampState(SHOOT_AGAIN, 1, 0, (msRemaining<3000)?100:500);
  } else {
    BSOS_SetLampState(SHOOT_AGAIN, SamePlayerShootsAgain);
  }
}



////////////////////////////////////////////////////////////////////////////
//
//  Display Management functions
//
////////////////////////////////////////////////////////////////////////////
unsigned long LastTimeScoreChanged = 0;
unsigned long LastTimeOverrideAnimated = 0;
unsigned long LastFlashOrDash = 0;
unsigned long ScoreOverrideValue[4]= {0, 0, 0, 0};
byte ScoreOverrideStatus = 0;
byte LastScrollPhase = 0;

byte MagnitudeOfScore(unsigned long score) {
  if (score == 0) return 0;

  byte retval = 0;
  while (score > 0) {
    score = score / 10;
    retval += 1;
  }
  return retval;
}

void OverrideScoreDisplay(byte displayNum, unsigned long value, boolean animate) {
  if (displayNum>3) return;
  ScoreOverrideStatus |= (0x10<<displayNum);
  if (animate) ScoreOverrideStatus |= (0x01<<displayNum);
  else ScoreOverrideStatus &= ~(0x01<<displayNum);
  ScoreOverrideValue[displayNum] = value;
}

byte GetDisplayMask(byte numDigits) {
  byte displayMask = 0;
  for (byte digitCount=0; digitCount<numDigits; digitCount++) {
    displayMask |= (0x20>>digitCount);
  }  
  return displayMask;
}


void ShowPlayerScores(byte displayToUpdate, boolean flashCurrent, boolean dashCurrent, unsigned long allScoresShowValue=0) {

  if (displayToUpdate==0xFF) ScoreOverrideStatus = 0;

  byte displayMask = 0x3F;
  unsigned long displayScore = 0;
  unsigned long overrideAnimationSeed = CurrentTime/250;
  byte scrollPhaseChanged = false;

  byte scrollPhase = ((CurrentTime-LastTimeScoreChanged)/250)%16;
  if (scrollPhase!=LastScrollPhase) {
    LastScrollPhase = scrollPhase;
    scrollPhaseChanged = true;
  }

  boolean updateLastTimeAnimated = false;

  for (byte scoreCount=0; scoreCount<4; scoreCount++) {
    // If this display is currently being overriden, then we should update it
    if (allScoresShowValue==0 && (ScoreOverrideStatus & (0x10<<scoreCount))) {
      displayScore = ScoreOverrideValue[scoreCount];
      byte numDigits = MagnitudeOfScore(displayScore);
      if (numDigits==0) numDigits = 1;
      if (numDigits<(BALLY_STERN_OS_NUM_DIGITS-1) && (ScoreOverrideStatus & (0x01<<scoreCount))) {
        if (overrideAnimationSeed!=LastTimeOverrideAnimated) {
          updateLastTimeAnimated = true;
          byte shiftDigits = (overrideAnimationSeed)%(((BALLY_STERN_OS_NUM_DIGITS+1)-numDigits)+((BALLY_STERN_OS_NUM_DIGITS-1)-numDigits));
          if (shiftDigits>=((BALLY_STERN_OS_NUM_DIGITS+1)-numDigits)) shiftDigits = (BALLY_STERN_OS_NUM_DIGITS-numDigits)*2-shiftDigits;
          byte digitCount;
          displayMask = GetDisplayMask(numDigits);
          for (digitCount=0; digitCount<shiftDigits; digitCount++) {
            displayScore *= 10;
            displayMask = displayMask>>1;
          }
          BSOS_SetDisplayBlank(scoreCount, 0x00);
          BSOS_SetDisplay(scoreCount, displayScore, false);
          BSOS_SetDisplayBlank(scoreCount, displayMask);
        }
      } else {
        BSOS_SetDisplay(scoreCount, displayScore, true);
      }
      
    } else {
      // No override, update scores designated by displayToUpdate
//      if (allScoresShowValue==0) displayScore = (scoreCount==CurrentPlayer)?CurrentPlayerCurrentScore:CurrentScores[scoreCount];
      if (allScoresShowValue==0) displayScore = CurrentScores[scoreCount];
      else displayScore = allScoresShowValue;

      // If we're updating all displays, or the one currently matching the loop, or if we have to scroll
      if (displayToUpdate==0xFF || displayToUpdate==scoreCount || displayScore>BALLY_STERN_OS_MAX_DISPLAY_SCORE) {

        // Don't show this score if it's not a current player score (even if it's scrollable)
        if (displayToUpdate==0xFF && (scoreCount>=CurrentNumPlayers&&CurrentNumPlayers!=0) && allScoresShowValue==0) {
          BSOS_SetDisplayBlank(scoreCount, 0x00);
          continue;
        }

        if (displayScore>BALLY_STERN_OS_MAX_DISPLAY_SCORE) {
          // Score needs to be scrolled
          if ((CurrentTime-LastTimeScoreChanged)<4000) {
            BSOS_SetDisplay(scoreCount, displayScore%(BALLY_STERN_OS_MAX_DISPLAY_SCORE+1), false);  
            BSOS_SetDisplayBlank(scoreCount, BALLY_STERN_OS_ALL_DIGITS_MASK);
          } else {

            // Scores are scrolled 10 digits and then we wait for 6
            if (scrollPhase<11 && scrollPhaseChanged) {
              byte numDigits = MagnitudeOfScore(displayScore);
              
              // Figure out top part of score
              unsigned long tempScore = displayScore;
              if (scrollPhase<BALLY_STERN_OS_NUM_DIGITS) {
                displayMask = BALLY_STERN_OS_ALL_DIGITS_MASK;
                for (byte scrollCount=0; scrollCount<scrollPhase; scrollCount++) {
                  displayScore = (displayScore % (BALLY_STERN_OS_MAX_DISPLAY_SCORE+1)) * 10;
                  displayMask = displayMask >> 1;
                }
              } else {
                displayScore = 0; 
                displayMask = 0x00;
              }

              // Add in lower part of score
              if ((numDigits+scrollPhase)>10) {
                byte numDigitsNeeded = (numDigits+scrollPhase)-10;
                for (byte scrollCount=0; scrollCount<(numDigits-numDigitsNeeded); scrollCount++) {
                  tempScore /= 10;
                }
                displayMask |= GetDisplayMask(MagnitudeOfScore(tempScore));
                displayScore += tempScore;
              }
              BSOS_SetDisplayBlank(scoreCount, displayMask);
              BSOS_SetDisplay(scoreCount, displayScore);
            }
          }          
        } else {
          if (flashCurrent) {
            unsigned long flashSeed = CurrentTime/250;
            if (flashSeed != LastFlashOrDash) {
              LastFlashOrDash = flashSeed;
              if (((CurrentTime/250)%2)==0) BSOS_SetDisplayBlank(scoreCount, 0x00);
              else BSOS_SetDisplay(scoreCount, displayScore, true, 2);
            }
          } else if (dashCurrent) {
            unsigned long dashSeed = CurrentTime/50;
            if (dashSeed != LastFlashOrDash) {
              LastFlashOrDash = dashSeed;
              byte dashPhase = (CurrentTime/60)%36;
              byte numDigits = MagnitudeOfScore(displayScore);
              if (dashPhase<12) { 
                displayMask = GetDisplayMask((numDigits==0)?2:numDigits);
                if (dashPhase<7) {
                  for (byte maskCount=0; maskCount<dashPhase; maskCount++) {
                    displayMask &= ~(0x01<<maskCount);
                  }
                } else {
                  for (byte maskCount=12; maskCount>dashPhase; maskCount--) {
                    displayMask &= ~(0x20>>(maskCount-dashPhase-1));
                  }
                }
                BSOS_SetDisplay(scoreCount, displayScore);
                BSOS_SetDisplayBlank(scoreCount, displayMask);
              } else {
                BSOS_SetDisplay(scoreCount, displayScore, true, 2);
              }
            }
          } else {
            BSOS_SetDisplay(scoreCount, displayScore, true, 2);          
          }
        }
      } // End if this display should be updated
    } // End on non-overridden
  } // End loop on scores

  if (updateLastTimeAnimated) {
    LastTimeOverrideAnimated = overrideAnimationSeed;
  }

}




////////////////////////////////////////////////////////////////////////////
//
//  Machine State Helper functions
//
////////////////////////////////////////////////////////////////////////////
boolean AddPlayer(boolean resetNumPlayers = false) {

  if (Credits < 1 && !FreePlayMode) return false;
  if (resetNumPlayers) CurrentNumPlayers = 0;
  if (CurrentNumPlayers >= 4) return false;

  CurrentNumPlayers += 1;
  BSOS_SetDisplay(CurrentNumPlayers - 1, 0);
  BSOS_SetDisplayBlank(CurrentNumPlayers - 1, 0x30);

  if (!FreePlayMode) {
    Credits -= 1;
    BSOS_WriteByteToEEProm(BSOS_CREDITS_EEPROM_BYTE, Credits);
    BSOS_SetDisplayCredits(Credits);
    BSOS_SetCoinLockout(false);
  }
  PlaySoundEffect(SOUND_EFFECT_ADD_PLAYER_1 + (CurrentNumPlayers - 1));
  SetPlayerLamps(CurrentNumPlayers);

  BSOS_WriteULToEEProm(BSOS_TOTAL_PLAYS_EEPROM_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_PLAYS_EEPROM_START_BYTE) + 1);

  return true;
}

void AddCoinToAudit(byte switchHit) {

  unsigned short coinAuditStartByte = 0;

  switch (switchHit) {
    case SW_COIN_3: coinAuditStartByte = BSOS_CHUTE_3_COINS_START_BYTE; break;
    case SW_COIN_2: coinAuditStartByte = BSOS_CHUTE_2_COINS_START_BYTE; break;
    case SW_COIN_1: coinAuditStartByte = BSOS_CHUTE_1_COINS_START_BYTE; break;
  }

  if (coinAuditStartByte) {
    BSOS_WriteULToEEProm(coinAuditStartByte, BSOS_ReadULFromEEProm(coinAuditStartByte) + 1);
  }

}


void AddCredit(boolean playSound = false, byte numToAdd = 1) {
  if (Credits < MaximumCredits) {
    Credits += numToAdd;
    if (Credits > MaximumCredits) Credits = MaximumCredits;
    BSOS_WriteByteToEEProm(BSOS_CREDITS_EEPROM_BYTE, Credits);
    if (playSound) PlaySoundEffect(SOUND_EFFECT_ADD_CREDIT);
    BSOS_SetDisplayCredits(Credits);
    BSOS_SetCoinLockout(false);
  } else {
    BSOS_SetDisplayCredits(Credits);
    BSOS_SetCoinLockout(true);
  }

}

void AddSpecialCredit() {
  AddCredit(false, 1);
  BSOS_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime, true);
  BSOS_WriteULToEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE) + 1);  
}


#define ADJ_TYPE_LIST                 1
#define ADJ_TYPE_MIN_MAX              2
#define ADJ_TYPE_MIN_MAX_DEFAULT      3
#define ADJ_TYPE_SCORE                4
#define ADJ_TYPE_SCORE_WITH_DEFAULT   5
#define ADJ_TYPE_SCORE_NO_DEFAULT     6
byte AdjustmentType = 0;
byte NumAdjustmentValues = 0;
byte AdjustmentValues[8];
unsigned long AdjustmentScore;
byte *CurrentAdjustmentByte = NULL;
unsigned long *CurrentAdjustmentUL = NULL;
byte CurrentAdjustmentStorageByte = 0;
byte TempValue = 0;


int RunSelfTest(int curState, boolean curStateChanged) {
  int returnState = curState;
  CurrentNumPlayers = 0;

  if (curStateChanged) {
    // Send a stop-all command and reset the sample-rate offset, in case we have
    //  reset while the WAV Trigger was already playing.
    Audio.StopAllAudio();
  }

  // Any state that's greater than CHUTE_3 is handled by the Base Self-test code
  // Any that's less, is machine specific, so we handle it here.
  if (curState >= MACHINE_STATE_TEST_CHUTE_3_COINS) {
    returnState = RunBaseSelfTest(returnState, curStateChanged, CurrentTime, SW_CREDIT_RESET, SW_SLAM);
  } else {
    byte curSwitch = BSOS_PullFirstFromSwitchStack();

    if (curSwitch == SW_SELF_TEST_SWITCH && (CurrentTime - GetLastSelfTestChangedTime()) > 250) {
      SetLastSelfTestChangedTime(CurrentTime);
      returnState -= 1;
    }

    if (curSwitch == SW_SLAM) {
      returnState = MACHINE_STATE_ATTRACT;
    }

    if (curStateChanged) {
      for (int count = 0; count < 4; count++) {
        BSOS_SetDisplay(count, 0);
        BSOS_SetDisplayBlank(count, 0x00);
      }
      BSOS_SetDisplayCredits(MACHINE_STATE_TEST_SOUNDS - curState);
      BSOS_SetDisplayBallInPlay(0, false);
      CurrentAdjustmentByte = NULL;
      CurrentAdjustmentUL = NULL;
      CurrentAdjustmentStorageByte = 0;

      AdjustmentType = ADJ_TYPE_MIN_MAX;
      AdjustmentValues[0] = 0;
      AdjustmentValues[1] = 1;
      TempValue = 0;

      switch (curState) {
        case MACHINE_STATE_ADJUST_FREEPLAY:
          CurrentAdjustmentByte = (byte *)&FreePlayMode;
          CurrentAdjustmentStorageByte = EEPROM_FREE_PLAY_BYTE;
          break;
        case MACHINE_STATE_ADJUST_BALL_SAVE:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 5;
          AdjustmentValues[1] = 5;
          AdjustmentValues[2] = 10;
          AdjustmentValues[3] = 15;
          AdjustmentValues[4] = 20;
          CurrentAdjustmentByte = &BallSaveNumSeconds;
          CurrentAdjustmentStorageByte = EEPROM_BALL_SAVE_BYTE;
          break;
        case MACHINE_STATE_ADJUST_SOUND_SELECTOR:
          AdjustmentType = ADJ_TYPE_MIN_MAX_DEFAULT;
          AdjustmentValues[1] = 3;
          CurrentAdjustmentByte = &SoundSelector;
          CurrentAdjustmentStorageByte = EEPROM_SOUND_SELECTOR_BYTE;
          break;
        case MACHINE_STATE_ADJUST_TOURNAMENT_SCORING:
          CurrentAdjustmentByte = (byte *)&TournamentScoring;
          CurrentAdjustmentStorageByte = EEPROM_TOURNAMENT_SCORING_BYTE;
          break;
        case MACHINE_STATE_ADJUST_TILT_WARNING:
          AdjustmentValues[1] = 2;
          CurrentAdjustmentByte = &MaxTiltWarnings;
          CurrentAdjustmentStorageByte = EEPROM_TILT_WARNING_BYTE;
          break;
        case MACHINE_STATE_ADJUST_AWARD_OVERRIDE:
          AdjustmentType = ADJ_TYPE_MIN_MAX_DEFAULT;
          AdjustmentValues[1] = 7;
          CurrentAdjustmentByte = &ScoreAwardReplay;
          CurrentAdjustmentStorageByte = EEPROM_AWARD_OVERRIDE_BYTE;
          break;
        case MACHINE_STATE_ADJUST_BALLS_OVERRIDE:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 3;
          AdjustmentValues[0] = 3;
          AdjustmentValues[1] = 5;
          AdjustmentValues[2] = 99;
          CurrentAdjustmentByte = &BallsPerGame;
          CurrentAdjustmentStorageByte = EEPROM_BALLS_OVERRIDE_BYTE;
          break;
        case MACHINE_STATE_ADJUST_SCROLLING_SCORES:
          CurrentAdjustmentByte = (byte *)&ScrollingScores;
          CurrentAdjustmentStorageByte = EEPROM_SCROLLING_SCORES_BYTE;
          break;

        case MACHINE_STATE_ADJUST_EXTRA_BALL_AWARD:
          AdjustmentType = ADJ_TYPE_SCORE_WITH_DEFAULT;
          CurrentAdjustmentUL = &ExtraBallValue;
          CurrentAdjustmentStorageByte = EEPROM_EXTRA_BALL_SCORE_BYTE;
          break;

        case MACHINE_STATE_ADJUST_SPECIAL_AWARD:
          AdjustmentType = ADJ_TYPE_SCORE_WITH_DEFAULT;
          CurrentAdjustmentUL = &SpecialValue;
          CurrentAdjustmentStorageByte = EEPROM_SPECIAL_SCORE_BYTE;
          break;

        case MACHINE_STATE_ADJUST_DIM_LEVEL:
          AdjustmentType = ADJ_TYPE_LIST;
          NumAdjustmentValues = 2;
          AdjustmentValues[0] = 2;
          AdjustmentValues[1] = 3;
          CurrentAdjustmentByte = &DimLevel;
          CurrentAdjustmentStorageByte = EEPROM_DIM_LEVEL_BYTE;
          for (int count = 0; count < 10; count++) BSOS_SetLampState(BONUS_1 + count, 1, 1);
          break;

        case MACHINE_STATE_ADJUST_DONE:
          returnState = MACHINE_STATE_ATTRACT;
          break;
      }

    }

    // Change value, if the switch is hit
    if (curSwitch == SW_CREDIT_RESET) {

      if (CurrentAdjustmentByte && (AdjustmentType == ADJ_TYPE_MIN_MAX || AdjustmentType == ADJ_TYPE_MIN_MAX_DEFAULT)) {
        byte curVal = *CurrentAdjustmentByte;
        curVal += 1;
        if (curVal > AdjustmentValues[1]) {
          if (AdjustmentType == ADJ_TYPE_MIN_MAX) curVal = AdjustmentValues[0];
          else {
            if (curVal > 99) curVal = AdjustmentValues[0];
            else curVal = 99;
          }
        }
        *CurrentAdjustmentByte = curVal;
        if (CurrentAdjustmentStorageByte) EEPROM.write(CurrentAdjustmentStorageByte, curVal);
      } else if (CurrentAdjustmentByte && AdjustmentType == ADJ_TYPE_LIST) {
        byte valCount = 0;
        byte curVal = *CurrentAdjustmentByte;
        byte newIndex = 0;
        for (valCount = 0; valCount < (NumAdjustmentValues - 1); valCount++) {
          if (curVal == AdjustmentValues[valCount]) newIndex = valCount + 1;
        }
        *CurrentAdjustmentByte = AdjustmentValues[newIndex];
        if (CurrentAdjustmentStorageByte) EEPROM.write(CurrentAdjustmentStorageByte, AdjustmentValues[newIndex]);
      } else if (CurrentAdjustmentUL && (AdjustmentType == ADJ_TYPE_SCORE_WITH_DEFAULT || AdjustmentType == ADJ_TYPE_SCORE_NO_DEFAULT)) {
        unsigned long curVal = *CurrentAdjustmentUL;
        curVal += 5000;
        if (curVal > 100000) curVal = 0;
        if (AdjustmentType == ADJ_TYPE_SCORE_NO_DEFAULT && curVal == 0) curVal = 5000;
        *CurrentAdjustmentUL = curVal;
        if (CurrentAdjustmentStorageByte) BSOS_WriteULToEEProm(CurrentAdjustmentStorageByte, curVal);
      }

      if (curState == MACHINE_STATE_ADJUST_DIM_LEVEL) {
        BSOS_SetDimDivisor(1, DimLevel);
      }
    }

    // Show current value
    if (CurrentAdjustmentByte != NULL) {
      BSOS_SetDisplay(0, (unsigned long)(*CurrentAdjustmentByte), true);
    } else if (CurrentAdjustmentUL != NULL) {
      BSOS_SetDisplay(0, (*CurrentAdjustmentUL), true);
    }

  }

  if (curState == MACHINE_STATE_ADJUST_DIM_LEVEL) {
    for (int count = 0; count < 10; count++) BSOS_SetLampState(BONUS_1 + count, 1, (CurrentTime / 1000) % 2);
  }

  if (returnState == MACHINE_STATE_ATTRACT) {
    // If any variables have been set to non-override (99), return
    // them to dip switch settings
    // Balls Per Game, Player Loses On Ties, Novelty Scoring, Award Score
//    DecodeDIPSwitchParameters();
    ReadStoredParameters();
  }

  return returnState;
}




////////////////////////////////////////////////////////////////////////////
//
//  Audio Output functions
//
////////////////////////////////////////////////////////////////////////////

void QueueNotification(unsigned int soundEffectNum, byte priority) {
  if (CalloutsVolume==0) return;
  if (SoundSelector<3 || SoundSelector==4 || SoundSelector==7 || SoundSelector==9) return;   
  if (soundEffectNum < SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START || soundEffectNum >= (SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START + NUM_VOICE_NOTIFICATIONS)) return;

  Audio.QueueNotification(soundEffectNum, VoiceNotificationDurations[soundEffectNum-SOUND_EFFECT_VP_VOICE_NOTIFICATIONS_START], priority, CurrentTime);
}


void PlayBackgroundSong(byte songNum) {
  Audio.PlayBackgroundSong(songNum, true);
}


unsigned long NextSoundEffectTime = 0;

void PlaySoundEffect(unsigned short soundEffectNum) {

  if (SoundSelector == 0) return;

  if (SoundSelector>2) Audio.PlaySound(soundEffectNum, AUDIO_PLAY_TYPE_WAV_TRIGGER);

  if (SoundSelector==1) {
  
    switch(soundEffectNum) {
      case SOUND_EFFECT_ROLLOVER:
      case SOUND_EFFECT_DT_SKILL_SHOT:
      case SOUND_EFFECT_ROLLOVER_SKILL_SHOT:
      case SOUND_EFFECT_SU_SKILL_SHOT:
      case SOUND_EFFECT_LEFT_SPINNER: 
      case SOUND_EFFECT_RIGHT_SPINNER:
      case SOUND_EFFECT_DROP_TARGET:  
      case SOUND_EFFECT_BALL_OVER:    
        Audio.QueueSound(0x02, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime);      
        Audio.QueueSound(0x00, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime + 75);
      break;
      case SOUND_EFFECT_LEFT_INLANE:  
        for (int count=0; count<RolloverValue; count++) {
          Audio.QueueSound(0x04, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime + 200*count);      
          Audio.QueueSound(0x00, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime + 75 + (200*count));
        }
      break;
      case SOUND_EFFECT_RIGHT_INLANE: 
        for (int count=0; count<6; count++) {
          Audio.QueueSound((count<3)?0x04:0x10, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime + 200*count);      
          Audio.QueueSound(0x00, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime + 75 + (200*count));
        }
      break;
      case SOUND_EFFECT_SAUCER_HIT_5K:
        for (int count=0; count<5; count++) {
          Audio.QueueSound(0x04, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime + 200*count);      
          Audio.QueueSound(0x00, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime + 75 + (200*count));
        }
      break;
      case SOUND_EFFECT_SAUCER_HIT_30K: 
        for (int count=0; count<3; count++) {
          Audio.QueueSound(0x08, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime + 200*count);      
          Audio.QueueSound(0x00, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime + 75 + (200*count));
        }
      break;
      case SOUND_EFFECT_SAUCER_HIT_20K:     
        for (int count=0; count<2; count++) {
          Audio.QueueSound(0x08, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime + 200*count);      
          Audio.QueueSound(0x00, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime + 75 + (200*count));
        }
      break;
      case SOUND_EFFECT_SAUCER_HIT_10K:    
        for (int count=0; count<1; count++) {
          Audio.QueueSound(0x08, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime + 200*count);      
          Audio.QueueSound(0x00, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime + 75 + (200*count));
        }
      break;
      case SOUND_EFFECT_RIGHT_OUTLANE:
        for (int count=0; count<5; count++) {
          Audio.QueueSound(0x04, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime + 200*count);      
          Audio.QueueSound(0x00, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime + 75 + (200*count));
        }
      break;
      case SOUND_EFFECT_TOP_BUMPER_HIT:     
      case SOUND_EFFECT_BOTTOM_BUMPER_HIT: 
        Audio.QueueSound(0x20, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime);      
        Audio.QueueSound(0x00, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime+75);      
      break;     
      case SOUND_EFFECT_VP_SHOOT_AGAIN:  
      case SOUND_EFFECT_PLAYER_1_UP:  
      case SOUND_EFFECT_PLAYER_2_UP:  
      case SOUND_EFFECT_PLAYER_3_UP:  
      case SOUND_EFFECT_PLAYER_4_UP:  
        Audio.QueueSound(0x08, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime);      
        Audio.QueueSound(0x04, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime+75);      
        Audio.QueueSound(0x00, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime+175);      
      break;
      case SOUND_EFFECT_BONUS_COUNT:  
      case SOUND_EFFECT_2X_BONUS_COUNT:     
      case SOUND_EFFECT_3X_BONUS_COUNT:     
      case SOUND_EFFECT_4X_BONUS_COUNT:     
      case SOUND_EFFECT_5X_BONUS_COUNT:
        Audio.QueueSound(0x04, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime);      
        Audio.QueueSound(0x00, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime+75);      
      break;     
      case SOUND_EFFECT_UPPER_SLING:  
      case SOUND_EFFECT_VP_EXTRA_BALL:   
      case SOUND_EFFECT_TILT_WARNING: 
        Audio.QueueSound(0x10, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime);      
        Audio.QueueSound(0x00, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime+75);      
      break;
      case SOUND_EFFECT_10PT_SWITCH:  
      case SOUND_EFFECT_MATCH_SPIN:   
      case SOUND_EFFECT_LOWER_SLING:  
        Audio.QueueSound(0x01, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime);      
        Audio.QueueSound(0x00, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime+75);      
      break;
      case SOUND_EFFECT_DROP_TARGET_CLEAR_1:
      case SOUND_EFFECT_DROP_TARGET_CLEAR_2:
      case SOUND_EFFECT_DROP_TARGET_CLEAR_3:
      case SOUND_EFFECT_DROP_TARGET_CLEAR_4:
      case SOUND_EFFECT_DROP_TARGET_CLEAR_5:
        Audio.QueueSound(0x08, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime);      
        Audio.QueueSound(0x00, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime+75);      
      break;
      case SOUND_EFFECT_FIRST_SU_SWITCH_HIT:
      case SOUND_EFFECT_SECOND_SU_SWITCH_HIT:
      case SOUND_EFFECT_THIRD_SU_SWITCH_HIT:
      case SOUND_EFFECT_FOURTH_SU_SWITCH_HIT: 
      case SOUND_EFFECT_FIFTH_SU_SWITCH_HIT:
        Audio.QueueSound(0x04, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime);      
        Audio.QueueSound(0x00, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime+75);      
      break;
      case SOUND_EFFECT_ADD_CREDIT:
      case SOUND_EFFECT_GAME_OVER:
        Audio.QueueSound(0x08, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime);
        Audio.QueueSound(0x04, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime+75);
        Audio.QueueSound(0x02, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime+150);
        Audio.QueueSound(0x01, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime+225);
        Audio.QueueSound(0x08, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime+325);
        Audio.QueueSound(0x04, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime+400);
        Audio.QueueSound(0x02, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime+475);
        Audio.QueueSound(0x01, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime+550);
        Audio.QueueSound(0x00, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime+650);
      break;
      case SOUND_EFFECT_ADD_PLAYER_1:
      case SOUND_EFFECT_ADD_PLAYER_2: 
      case SOUND_EFFECT_ADD_PLAYER_3: 
      case SOUND_EFFECT_ADD_PLAYER_4: 
      case SOUND_EFFECT_VP_RESCUE_FROM_THE_DEEP:
      case SOUND_EFFECT_TRIDENT_INTRO:
        Audio.QueueSound(0x01, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime);
        Audio.QueueSound(0x02, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime+75);
        Audio.QueueSound(0x04, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime+150);
        Audio.QueueSound(0x08, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime+225);
        Audio.QueueSound(0x01, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime+325);
        Audio.QueueSound(0x02, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime+400);
        Audio.QueueSound(0x04, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime+475);
        Audio.QueueSound(0x08, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime+550);
        Audio.QueueSound(0x00, AUDIO_PLAY_TYPE_ORIGINAL_SOUNDS, CurrentTime+650);
      break;
    }
  }


}


////////////////////////////////////////////////////////////////////////////
//
//  Attract Mode
//
////////////////////////////////////////////////////////////////////////////

unsigned long AttractLastLadderTime = 0;
byte AttractLastLadderBonus = 0;
unsigned long AttractLastStarTime = 0;
byte AttractLastHeadMode = 255;
byte AttractLastPlayfieldMode = 255;
byte InAttractMode = false;

int RunAttractMode(int curState, boolean curStateChanged) {

  int returnState = curState;

  if (curStateChanged) {
#ifdef BALLY_STERN_OS_USE_SB100    
    BSOS_PlaySB100(0);
#endif
    BSOS_DisableSolenoidStack();
    BSOS_TurnOffAllLamps();
    BSOS_SetDisableFlippers(true);
    if (DEBUG_MESSAGES) {
      Serial.write("Entering Attract Mode\n\r");
    }

    AttractLastHeadMode = 0;
    AttractLastPlayfieldMode = 0;
  }

  // Alternate displays between high score and blank
  if (CurrentTime<16000) {
    if (AttractLastHeadMode!=1) {
      ShowPlayerScores(0xFF, false, false);
      SetPlayerLamps(0);
      BSOS_SetDisplayCredits(Credits, true);
      BSOS_SetDisplayBallInPlay(0, true);
    }    
  } else if ((CurrentTime / 8000) % 2 == 0) {

    if (AttractLastHeadMode != 2) {
      BSOS_SetLampState(HIGH_SCORE_TO_DATE, 1, 0, 250);
      BSOS_SetLampState(GAME_OVER, 0);
      SetPlayerLamps(0);
      LastTimeScoreChanged = CurrentTime;
    }
    AttractLastHeadMode = 2;
    ShowPlayerScores(0xFF, false, false, HighScore);
  } else {
    if (AttractLastHeadMode != 3) {
      if (CurrentTime<32000) {
        for (int count = 0; count < 4; count++) {
          CurrentScores[count] = 0;
        }
        CurrentNumPlayers = 0;
      }
      BSOS_SetLampState(HIGH_SCORE_TO_DATE, 0);
      BSOS_SetLampState(GAME_OVER, 1);
      BSOS_SetDisplayCredits(Credits, true);
      BSOS_SetDisplayBallInPlay(0, true);
      LastTimeScoreChanged = CurrentTime;
    }
    ShowPlayerScores(0xFF, false, false);
    
    SetPlayerLamps(((CurrentTime / 250) % 4) + 1);
    AttractLastHeadMode = 3;
  }

  if ((CurrentTime / 10000) % 3 < 2) {
    if (AttractLastPlayfieldMode != 1) {
      BSOS_TurnOffAllLamps();
      SetGameMode(GAME_MODE_SKILL_SHOT);
    }
    ShowSaucerLamps();
    ShowDropTargetLamps();
    ShowStandupTargetLamps();
    ShowLeftSpinnerLamps();
    ShowLeftLaneLamps();

    AttractLastPlayfieldMode = 1;
  } else {
    if (AttractLastPlayfieldMode != 2) {
      BSOS_TurnOffAllLamps();
      AttractLastLadderBonus = 1;
      AttractLastLadderTime = CurrentTime;
    }
    if ((CurrentTime-AttractLastLadderTime)>200) {
      AttractLastLadderBonus += 1;
      AttractLastLadderTime = CurrentTime;
      ShowBonusOnTree(AttractLastLadderBonus%MAX_DISPLAY_BONUS);
    }

    AttractLastPlayfieldMode = 2;
  }

  byte switchHit;
  while ( (switchHit = BSOS_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY ) {
    if (switchHit == SW_CREDIT_RESET) {
      if (AddPlayer(true)) returnState = MACHINE_STATE_INIT_GAMEPLAY;
    }
    if (switchHit == SW_COIN_1 || switchHit == SW_COIN_2 || switchHit == SW_COIN_3) {
      AddCoinToAudit(switchHit);
      AddCredit(true, 1);
    }
    if (switchHit == SW_SELF_TEST_SWITCH && (CurrentTime - GetLastSelfTestChangedTime()) > 250) {
      returnState = MACHINE_STATE_TEST_LIGHTS;
      SetLastSelfTestChangedTime(CurrentTime);
    }
  }

  return returnState;
}





////////////////////////////////////////////////////////////////////////////
//
//  Game Play functions
//
////////////////////////////////////////////////////////////////////////////
byte CountBits(byte byteToBeCounted) {
  byte numBits = 0;

  for (byte count=0; count<8; count++) {
    numBits += (byteToBeCounted&0x01);
    byteToBeCounted = byteToBeCounted>>1;
  }

  return numBits;
}



void ResetDropTargets() {
  BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_RESET, 12, CurrentTime + 400);  
  DropTargetClearTime = CurrentTime;

  if (MiniGamesRunning&MINI_GAME_SHARP_SHOOTER_FLAG) {
    if (SharpShooterTarget!=1) BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_1, 7, CurrentTime + 600);
    if (SharpShooterTarget!=2) BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_2, 7, CurrentTime + 625);
    if (SharpShooterTarget!=3) BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_3, 7, CurrentTime + 650);
    if (SharpShooterTarget!=4) BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_4, 7, CurrentTime + 675);
    if (SharpShooterTarget!=5) BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_5, 7, CurrentTime + 700);
    CurrentDropTargetsValid = 0x01 << (SharpShooterTarget-1);
  } else {  
    // For Bonus 2 & 4, we want the 2nd and 4th
    if (BonusX==1) {
      BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_1, 4, CurrentTime + 700);
      BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_3, 4, CurrentTime + 750);
      BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_5, 4, CurrentTime + 800);
      CurrentDropTargetsValid = 0x0A;
    } else if (BonusX==2) {
      BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_2, 4, CurrentTime + 600);
      BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_4, 4, CurrentTime + 650);
      CurrentDropTargetsValid = 0x15;
    } else if (BonusX==3) {
      BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_3, 4, CurrentTime + 600);
      CurrentDropTargetsValid = 0x1B;
    } else {
      CurrentDropTargetsValid = 0x1F;
    }
  }

}


void HandleDropTargetHit(byte switchHit, unsigned long scoreMultiplier) {
  if (GameMode==GAME_MODE_SKILL_SHOT) {
    BonusX = 2;
    PlaySoundEffect(SOUND_EFFECT_DT_SKILL_SHOT);
    ResetDropTargets();
    CurrentScores[CurrentPlayer] += 10000;
  } else {
    byte switchMask = 1<<(SW_DROP_TARGET_1-switchHit);
    
    if (switchMask & CurrentDropTargetsValid) {
      if (  BSOS_ReadSingleSwitchState(SW_DROP_TARGET_1) &&
            BSOS_ReadSingleSwitchState(SW_DROP_TARGET_2) &&
            BSOS_ReadSingleSwitchState(SW_DROP_TARGET_3) &&
            BSOS_ReadSingleSwitchState(SW_DROP_TARGET_4) &&
            BSOS_ReadSingleSwitchState(SW_DROP_TARGET_5) ) {
        // all drop targets are down
        if (!(MiniGamesRunning & MINI_GAME_SHARP_SHOOTER_FLAG)) {
          BonusX += 1;
          PlaySoundEffect(SOUND_EFFECT_DROP_TARGET_CLEAR_1 + (BonusX-1)); 
          if (BonusX==TargetSpecialBonus) {
            if (TournamentScoring) {
              CurrentScores[CurrentPlayer] += SpecialValue;
            } else {
              AddSpecialCredit();
            }
          }        

          if (BonusX==SharpShooterStartBonus && !(MiniGamesFlagsQualified&MINI_GAME_SHARP_SHOOTER_FLAG)) {
            if (GameMode==GAME_MODE_UNSTRUCTURED_PLAY) {
              MiniGamesFlagsQualified |= MINI_GAME_SHARP_SHOOTER_FLAG;
              // Play sound to indicate that sharp shooter is qualified
              QueueNotification(SOUND_EFFECT_VP_SHARP_SHOOTER_QUALIFIED, 7);
            } else {
              SharpShooterStartBonus = BonusX + 1;
            }
            SharpShooterTarget = 1;
            // If a mini game is already qualified, give the player more time
            if (GameMode==GAME_MODE_MINI_GAME_QUALIFIED) {
              GameModeEndTime = CurrentTime + MODE_QUALIFY_TIME;
            }
          }
        
        } else {
          if (CurrentSharpShooter<255) CurrentSharpShooter += 1;
          CurrentScores[CurrentPlayer] += (unsigned long)2000*(unsigned long)scoreMultiplier;
          SharpShooterTarget += 1;
          if (SharpShooterTarget>5) SharpShooterTarget = 1;
          PlaySoundEffect(SOUND_EFFECT_SHARP_SHOOTER_HIT);
        }
        
        if (BonusX>5) BonusX = 5;
        CurrentScores[CurrentPlayer] += ((unsigned long)1000*(unsigned long)BonusX);
        ResetDropTargets();
      } else {
        CurrentScores[CurrentPlayer] += 500;
        PlaySoundEffect(SOUND_EFFECT_DROP_TARGET); 
      }
    }
  }
}


void HandleStandupHit(byte switchHit, unsigned long scoreMultiplier) {
  byte switchMask = (1<<(switchHit-19));

  if (!(MiniGamesRunning & MINI_GAME_EXPLORE_THE_DEPTHS_FLAG)) {
    if (CurrentTime>StandupDisplayEndTime || StandupDisplayEndTime==0) {
      StandupDisplayEndTime = CurrentTime + STANDUP_HIT_DISPLAY_DURATION;
      LastStandupTargetHit = 0;
    } else {
      byte numSwitchesOn = CountBits(LastStandupTargetHit);
      if (numSwitchesOn>3) numSwitchesOn = 3;
      StandupDisplayEndTime = CurrentTime + STANDUP_HIT_DISPLAY_DURATION*numSwitchesOn;
    }
  
    if (GameMode==GAME_MODE_SKILL_SHOT) {
      CurrentScores[CurrentPlayer] += 15000;    
      PlaySoundEffect(SOUND_EFFECT_SU_SKILL_SHOT);
    } else {
      byte numSwitchesOn = CountBits(switchMask | CurrentStandupsHit);  
      PlaySoundEffect(SOUND_EFFECT_FIRST_SU_SWITCH_HIT + (numSwitchesOn-1));
      
      // Hitting an already lit switch is worth half as much as a new switch
      if (CurrentStandupsHit & switchMask) {
        CurrentScores[CurrentPlayer]+=500;  
      } else {
        CurrentScores[CurrentPlayer]+=1000;  
      }
    }
    CurrentStandupsHit |= switchMask;
    LastStandupTargetHit |= switchMask;
  } else {
    PlaySoundEffect(SOUND_EFFECT_EXPLORE_HIT);
    CurrentScores[CurrentPlayer]+=(unsigned long)10000*(unsigned long)scoreMultiplier;  
    if (CurrentExploreTheDepths<255) CurrentExploreTheDepths += 1;
  }

  // If the last target has been hit
  if (CurrentStandupsHit==0x1F) {
    CurrentStandupsHit = 0;
    LastStandupTargetHit = 0;
    NumberOfStandupClears += 1;
    if (NumberOfStandupClears==StandupSpecialLevel) {
      if (TournamentScoring) CurrentScores[CurrentPlayer] += (unsigned long)SpecialValue;
      else AddSpecialCredit();
    }
    if ((NumberOfStandupClears%ExploreTheDepthsStart)==0 && !(MiniGamesFlagsQualified&MINI_GAME_EXPLORE_THE_DEPTHS_FLAG)) {
      if (GameMode==GAME_MODE_UNSTRUCTURED_PLAY) {
        MiniGamesFlagsQualified |= MINI_GAME_EXPLORE_THE_DEPTHS_FLAG;
        QueueNotification(SOUND_EFFECT_VP_EXPLORE_QUALIFIED, 7);
      }
      // If a mini game is already qualified, give the player more time      
      if (GameMode==GAME_MODE_MINI_GAME_QUALIFIED) {
        GameModeEndTime = CurrentTime + MODE_QUALIFY_TIME;
      }
    } else {
      PlaySoundEffect(SOUND_EFFECT_STANDUPS_CLEARED);
      CurrentScores[CurrentPlayer]+=10000;  
    }
  }
}


int InitGamePlay() {

  if (DEBUG_MESSAGES) {
    Serial.write("Starting game\n\r");
  }

  // The start button has been hit only once to get
  // us into this mode, so we assume a 1-player game
  // at the moment
  BSOS_EnableSolenoidStack();
  BSOS_SetCoinLockout((Credits >= MaximumCredits) ? true : false);
  BSOS_TurnOffAllLamps();
  Audio.StopAllAudio();

  // Turn back on all lamps that are needed
  SetPlayerLamps(1);

  // When we go back to attract mode, there will be no need to reset scores
  ResetScoresToClearVersion = false;

  // Reset displays & game state variables
  for (int count = 0; count < 4; count++) {
    BSOS_SetDisplay(count, 0);
    if (count == 0) BSOS_SetDisplayBlank(count, 0x30);
    else BSOS_SetDisplayBlank(count, 0x00);

    CurrentScores[count] = 0;
    SamePlayerShootsAgain = false;

    // Initialize game-specific variables
    StandupsHit[count] = 0;
    FeedingFrenzySpins[count] = 0;
    ExploreTheDepthsHits[count] = 0;
    SharpShooterHits[count] = 0;
    NumAlternatingSpinnersRequired[count] = 3;
  }

  CurrentBallInPlay = 1;
  CurrentNumPlayers = 1;
  CurrentPlayer = 0;
  NumberOfBallsInPlay = 0;
  ShowPlayerScores(0xFF, false, false);

  if (BSOS_ReadSingleSwitchState(SW_SAUCER)) BSOS_PushToSolenoidStack(SOL_SAUCER, 5);

  return MACHINE_STATE_INIT_NEW_BALL;
}


int InitNewBall(bool curStateChanged, byte playerNum, int ballNum) {

  // If we're coming into this mode for the first time
  // then we have to do everything to set up the new ball
  if (curStateChanged) {
    SamePlayerShootsAgain = false;
    BallFirstSwitchHitTime = 0;

    BSOS_EnableSolenoidStack();
    BSOS_SetDisplayCredits(Credits, true);
    SetPlayerLamps(playerNum + 1, 4);
    if (CurrentNumPlayers>1 && (ballNum!=1 || playerNum!=0)) PlaySoundEffect(SOUND_EFFECT_PLAYER_1_UP+playerNum);
    // Start appropriate mode music
    PlayBackgroundSongBasedOnBall(ballNum);

    BSOS_SetDisplayBallInPlay(ballNum);
    BSOS_SetLampState(BALL_IN_PLAY, 1);
    BSOS_SetLampState(TILT, 0);

    if (BallSaveNumSeconds > 0) {
      BSOS_SetLampState(SHOOT_AGAIN, 1, 0, 500);
    }

    Bonus = 1;
    ShowBonusOnTree(1);
    BonusX = 1;
    BallTimeInTrough = 0;
    NumTiltWarnings = 0;
    LastTiltWarningTime = 0;
    SharpShooterStartBonus = 3;

    // Initialize game-specific start-of-ball lights & variables
    GameModeStartTime = CurrentTime;
    SetGameMode(GAME_MODE_SKILL_SHOT);
    MiniGamesFlagsQualified = 0;
    MiniGamesRunning = 0;
    SaucerValue = 5;
    NextSaucerReduction = 0;
    ShowSaucerHit = 0;
    SaucerHitTime = 0;
    DropTargetClearTime = 0;
    StandupDisplayEndTime = 0;
    CurrentDropTargetsValid = 0x1F;
    RolloverValue = 2;
    RolloverFlashEndTime = 0;
    RescueFromTheDeepEndTime = 0;
    RescueFromTheDeepAvailable = true;
    LastSpinnerSide = 0; // 1=left, 2=right
    AlternatingSpinnerCount = 0;
    LastSpinnerHitTime = 0;
    NumberOfStandupClears = 0;
    CurrentFeedingFrenzy = 0;
    CurrentExploreTheDepths = 0;
    CurrentSharpShooter = 0; 
    ExtraBallCollected = false;
    ShowingModeStats = false;
    JackpotLit = false;
    BallSaveEndTime = 0;

    CurrentStandupsHit = StandupsHit[ CurrentPlayer];

    if (CountBallsInTrough()==TotalBallsLoaded) {
      BSOS_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 100);
      NumberOfBallsInPlay = 1;
    }

    // Reset drop targets
    BSOS_PushToTimedSolenoidStack(SOL_DROP_TARGET_RESET, 12, CurrentTime);

    Audio.StopAllAudio();
  }

  // We should only consider the ball initialized when
  // the ball is in the shooter lane
  if (!BSOS_ReadSingleSwitchState(SW_SHOOTER_LANE)) {
    return MACHINE_STATE_INIT_NEW_BALL;
  } else {
    BSOS_SetDisableFlippers(false);
    return MACHINE_STATE_NORMAL_GAMEPLAY;
  }

}


void AddToBonus(byte bonusAddition) {
  Bonus += bonusAddition;
  if (Bonus > MAX_DISPLAY_BONUS) Bonus = MAX_DISPLAY_BONUS;
}


void PlayBackgroundSongBasedOnBall(byte ballNum) {
  if (ballNum==1) {
    PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_1);
  } else if (ballNum==BallsPerGame) {
    PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_6);
  } else {
    PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_2 + CurrentTime%4);
  }
}


void CheckForFeedingFrenzyQualify() {
  if (AlternatingSpinnerCount==0) return;
  
  if (LastSpinnerHitTime!=0 && (CurrentTime-LastSpinnerHitTime)>FEEDING_FRENZY_ALTERNATE_TIME) {
    LastSpinnerHitTime = 0;
    AlternatingSpinnerCount = 0;
    LastSpinnerSide = 0;
  }
  if (AlternatingSpinnerCount==NumAlternatingSpinnersRequired[CurrentPlayer] && !(MiniGamesFlagsQualified&MINI_GAME_FEEDING_FRENZY_FLAG)) {
    if (GameMode==GAME_MODE_UNSTRUCTURED_PLAY) {
      MiniGamesFlagsQualified |= MINI_GAME_FEEDING_FRENZY_FLAG;
      QueueNotification(SOUND_EFFECT_VP_FEEDING_FRENZY_QUALIFIED, 7);
    }

    // If a mini game is already qualified, give the player more time
    if (GameMode==GAME_MODE_MINI_GAME_QUALIFIED) {
      GameModeEndTime = CurrentTime + MODE_QUALIFY_TIME;
    }
  }
}



int LastReportedValue = 0;
boolean PlayerUpLightBlinking = false;
byte LastModeStep;
unsigned long LastModeStepTime;
unsigned long LastTroughSwitchCheck;


boolean AddABall() {
  if (NumberOfBallsInPlay>=4) return false;

  NumberOfBallsInPlay += 1;
  BSOS_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 100);
  AutoPlungeTime = CurrentTime + 100;
  LastTroughSwitchCheck = AutoPlungeTime;
  PlaySoundEffect(SOUND_EFFECT_ROLLOVER);

  if (BallSaveEndTime) BallSaveEndTime += 10000;
  else BallSaveEndTime = CurrentTime + 20000;

  return true;
}


// This function manages all timers, flags, and lights
int ManageGameMode() {
  int returnState = MACHINE_STATE_NORMAL_GAMEPLAY;

  // If the playfield hasn't been validated yet, flash score and player up num
  if (BallFirstSwitchHitTime == 0) {
    if (!PlayerUpLightBlinking) {
      SetPlayerLamps((CurrentPlayer + 1), 4, 250);
      PlayerUpLightBlinking = true;
    }
  } else {
    if (PlayerUpLightBlinking) {
      SetPlayerLamps((CurrentPlayer + 1), 4);
      PlayerUpLightBlinking = false;
    }
  }

  if (CurrentTime>RescueFromTheDeepEndTime) RescueFromTheDeepEndTime = 0;
  if (LastTroughSwitchCheck==0) LastTroughSwitchCheck = CurrentTime;

  if (CurrentTime>(LastTroughSwitchCheck+3000)) {
    LastTroughSwitchCheck = CurrentTime;
    if (BSOS_ReadSingleSwitchState(SW_SHOOTER_LANE)) {
      // Ball stuck in shooter lane, so kick it
      BSOS_FireContinuousSolenoid(0x10, 15);
    }
  }

  switch ( GameMode ) {
    case GAME_MODE_SKILL_SHOT:
      LastTroughSwitchCheck = CurrentTime;
      if (BallFirstSwitchHitTime!=0) {
        // Something has been hit, so we shouldn't be in skill shot anymore
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
        GameModeStartTime = 0;
        ResetDropTargets();
        
        if (DEBUG_MESSAGES) {
          Serial.write("Exit skill shot - Changing to Qualify Select\n\r");
        }
      }
    break;    
    case GAME_MODE_UNSTRUCTURED_PLAY:
      // If this is the first time in this mode
      if (GameModeStartTime==0) {
        GameModeStartTime = CurrentTime;
      }

      if (CurrentTime>StandupDisplayEndTime) {
        LastStandupTargetHit = 0;
      }

      CheckForFeedingFrenzyQualify();
      
      if ( ( CurrentTime-LastTimeScoreChanged)>2000 && (((CurrentTime-LastTimeScoreChanged)/4000)%2)==0 ) {
        if (!ShowingModeStats && (FeedingFrenzySpins[CurrentPlayer] || SharpShooterHits[CurrentPlayer] || ExploreTheDepthsHits[CurrentPlayer])) {
          int modeStatShown = 0;
          for (int displayCount=0; displayCount<4; displayCount++) {
            if (displayCount!=CurrentPlayer) {
              if (modeStatShown==0) OverrideScoreDisplay(displayCount, FeedingFrenzySpins[CurrentPlayer], false);
              if (modeStatShown==1) OverrideScoreDisplay(displayCount, SharpShooterHits[CurrentPlayer], false);
              if (modeStatShown==2) OverrideScoreDisplay(displayCount, ExploreTheDepthsHits[CurrentPlayer], false);
              modeStatShown += 1;
            }
          }
          ShowingModeStats = true;
        }        
      } else {
        if (ShowingModeStats) {
          ShowingModeStats = false;
          ShowPlayerScores(0xFF, false, false);
        }
      }

      // If any mode is qualified, enable the saucer to begin mini game
      if (MiniGamesFlagsQualified) {
        SetGameMode(GAME_MODE_MINI_GAME_QUALIFIED);
//        GameMode = GAME_MODE_MINI_GAME_QUALIFIED;
//        GameModeStartTime = 0;
      }
    break;
    case GAME_MODE_MINI_GAME_QUALIFIED:
      if (GameModeStartTime==0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + MODE_QUALIFY_TIME;
        // Play sound to direct player to saucer        
      }
      CheckForFeedingFrenzyQualify();
      
      if (CurrentTime>GameModeEndTime) {
//        GameModeStartTime = 0;
//        GameModeFlagsQualified = 0;
        MiniGamesFlagsQualified = 0;
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
//        GameMode = GAME_MODE_UNSTRUCTURED_PLAY;        
      }
    break;
    case GAME_MODE_MINI_GAME_ENGAGED:
      if (GameModeStartTime==0) {
        GameModeStartTime = CurrentTime;
        MiniGamesRunning = MiniGamesFlagsQualified;
        MiniGamesFlagsQualified = 0;
        unsigned short modeStartSound = SOUND_EFFECT_VP_FEEDING_FRENZY_START;
        switch (MiniGamesRunning) {
          case MINI_GAME_SHARP_SHOOTER_FLAG: modeStartSound = SOUND_EFFECT_VP_SHARP_SHOOTER_START; break;
          case MINI_GAME_EXPLORE_THE_DEPTHS_FLAG: modeStartSound = SOUND_EFFECT_VP_EXPLORE_THE_DEPTHS_START; break;
          case MINI_GAME_FEEDING_FRENZY_FLAG: modeStartSound = SOUND_EFFECT_VP_FEEDING_FRENZY_START; break;
          case (MINI_GAME_SHARP_SHOOTER_FLAG|MINI_GAME_FEEDING_FRENZY_FLAG): modeStartSound = SOUND_EFFECT_VP_SS_AND_FF_START; break;
          case (MINI_GAME_SHARP_SHOOTER_FLAG|MINI_GAME_EXPLORE_THE_DEPTHS_FLAG): modeStartSound = SOUND_EFFECT_VP_SS_AND_ETD_START; break;
          case (MINI_GAME_FEEDING_FRENZY_FLAG|MINI_GAME_EXPLORE_THE_DEPTHS_FLAG): modeStartSound = SOUND_EFFECT_VP_FF_AND_ETD_START; break;
          case (MINI_GAME_SHARP_SHOOTER_FLAG|MINI_GAME_FEEDING_FRENZY_FLAG|MINI_GAME_EXPLORE_THE_DEPTHS_FLAG): modeStartSound = SOUND_EFFECT_VP_MEGA_STACK_START; break;
        }
        QueueNotification(modeStartSound, 9);

        byte numMiniGames = CountBits(MiniGamesRunning);
        if (numMiniGames==1) {
          PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_FOR_SINGLE_MODE);
          GameModeEndTime = CurrentTime + (MINI_GAME_SINGLE_DURATION);
        } else if (numMiniGames==2) {
          PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_FOR_DOUBLE_MODE);
          GameModeEndTime = CurrentTime + (MINI_GAME_DOUBLE_DURATION);
        } else {
          PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_FOR_TRIPLE_MODE);
          GameModeEndTime = CurrentTime + (MINI_GAME_TRIPLE_DURATION);
        }

        LastModeStep = 0; 
        LastModeStepTime = CurrentTime + 1000;
      }

      if (CurrentTime>LastModeStepTime && LastModeStep<CountBits(MiniGamesRunning)) {
        LastModeStep += 1;
        LastModeStepTime = CurrentTime + 1000;
        AddABall();
      }

      if ((CurrentTime-GameModeStartTime)>MODE_START_DISPLAY_DURATION) {
        for (byte count=0; count<4; count++) {
          if (count!=CurrentPlayer) OverrideScoreDisplay(count, (GameModeEndTime-CurrentTime)/1000, true);
        }
      }

      if (MiniGamesRunning & MINI_GAME_SHARP_SHOOTER_FLAG) {
        if ((CurrentTime-DropTargetClearTime)>SHARP_SHOOTER_TARGET_TIME) {
          SharpShooterTarget += 1;
          if (SharpShooterTarget>5) SharpShooterTarget = 1;
          ResetDropTargets();
        }
      }

      if (CurrentTime>GameModeEndTime) {
        LastMiniGameBonusTime = 0;
        ShowPlayerScores(0xFF, false, false);
        PlayBackgroundSong(SOUND_EFFECT_NONE);
        SetGameMode(GAME_MODE_MINI_GAME_REWARD_COUNTDOWN);
        MiniGamesRunning = 0;
      }
    break;
    case GAME_MODE_MINI_GAME_REWARD_COUNTDOWN:
      if (GameModeStartTime==0) GameModeStartTime = CurrentTime;
      if (LastMiniGameBonusTime==0 || (CurrentTime-LastMiniGameBonusTime)>250) {
        if (CurrentFeedingFrenzy>0) {
          CurrentFeedingFrenzy -= 1;
          FeedingFrenzySpins[CurrentPlayer] += 1;
          CurrentScores[CurrentPlayer] += 1000;
          PlaySoundEffect(SOUND_EFFECT_FEEDING_FRENZY);   
        } else if (CurrentSharpShooter>0) {
          CurrentSharpShooter -= 1;
          SharpShooterHits[CurrentPlayer] += 1;
          CurrentScores[CurrentPlayer] += 2500;
          PlaySoundEffect(SOUND_EFFECT_SHARP_SHOOTER_HIT);
        } else if (CurrentExploreTheDepths>0) {
          CurrentExploreTheDepths -= 1;
          ExploreTheDepthsHits[CurrentPlayer] += 1;
          CurrentScores[CurrentPlayer] += 2500;
          PlaySoundEffect(SOUND_EFFECT_EXPLORE_HIT);
        } else {
          GameModeEndTime = 0;
          GameModeStartTime = 0;
          if (FeedingFrenzySpins[CurrentPlayer] && SharpShooterHits[CurrentPlayer] && ExploreTheDepthsHits[CurrentPlayer]) {
            SetGameMode(GAME_MODE_WIZARD);
          } else {
            SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
            PlayBackgroundSongBasedOnBall(CurrentBallInPlay);
            PlaySoundEffect(SOUND_EFFECT_MODE_FINISHED);
          }
        }
        LastMiniGameBonusTime = CurrentTime;
      }
    break;
    case GAME_MODE_WIZARD:
      if (GameModeStartTime==0) {
        GameModeStartTime = CurrentTime;
        GameModeEndTime = CurrentTime + WIZARD_MODE_DURATION;
        PlayBackgroundSong(SOUND_EFFECT_BACKGROUND_WIZ);
        QueueNotification(SOUND_EFFECT_VP_DEEP_BLUE_SEA_MODE, 9);
        JackpotLit = true;
        LastModeStepTime = CurrentTime + 1000;
        LastModeStep = 0;
      }

      if (CurrentTime>LastModeStepTime && LastModeStep<3) {
        LastModeStep += 1;
        LastModeStepTime = CurrentTime + 1000;
        AddABall();
      }

      if (!JackpotLit) {
        if (CurrentFeedingFrenzy && CurrentSharpShooter && CurrentExploreTheDepths) {
          JackpotLit = true;
        }
      }

      for (byte count=0; count<4; count++) {
        if (count!=CurrentPlayer) OverrideScoreDisplay(count, (GameModeEndTime-CurrentTime)/1000, true);
      }

      if (CurrentTime>GameModeEndTime) {
        FeedingFrenzySpins[CurrentPlayer] = 0;
        SharpShooterHits[CurrentPlayer] = 0;
        ExploreTheDepthsHits[CurrentPlayer] = 0;
        JackpotLit = false;
        LastMiniGameBonusTime = 0;
        ShowPlayerScores(0xFF, false, false);
        PlayBackgroundSongBasedOnBall(CurrentBallInPlay);
        PlaySoundEffect(SOUND_EFFECT_MODE_FINISHED);
        SetGameMode(GAME_MODE_UNSTRUCTURED_PLAY);
      }
      
    break;
  }

  if (GameModeStartTime!=0) {
    ShowSaucerLamps();
    ShowDropTargetLamps();
    ShowStandupTargetLamps();
    ShowBonusLamps();
    ShowBonusXLamps();
    ShowLeftSpinnerLamps();
    ShowRightSpinnerLamps();
    ShowLeftLaneLamps();
    ShowAwardLamps();
    ShowShootAgainLamp();
    ShowPlayerScores(CurrentPlayer, (BallFirstSwitchHitTime==0)?true:false, (BallFirstSwitchHitTime>0 && ((CurrentTime-LastTimeScoreChanged)>2000))?true:false);
  }

  // Check to see if ball is in the outhole
  if (CountBallsInTrough()>(TotalBallsLoaded-NumberOfBallsInPlay)) {    
    if (BallTimeInTrough == 0) {
      // If this is the first time we're seeing too many balls in the trough, we'll wait to make sure 
      // everything is settled
      BallTimeInTrough = CurrentTime;
    } else {
      
      // Make sure the ball stays on the sensor for at least
      // 0.5 seconds to be sure that it's not bouncing or passing through
      if ((CurrentTime - BallTimeInTrough) > 750) {

        if (BallFirstSwitchHitTime == 0 && NumTiltWarnings <= MaxTiltWarnings) {
          // Nothing hit yet, so return the ball to the player
          BSOS_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime);
          BallTimeInTrough = 0;
          LastTroughSwitchCheck = CurrentTime;
          returnState = MACHINE_STATE_NORMAL_GAMEPLAY;
        } else {
          // if we haven't used the ball save, and we're under the time limit, then save the ball
          if (BallSaveEndTime && CurrentTime<(BallSaveEndTime+BALL_SAVE_GRACE_PERIOD)) {
            BSOS_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 100);
            AutoPlungeTime = CurrentTime + 100;
            LastTroughSwitchCheck = AutoPlungeTime;
            QueueNotification(SOUND_EFFECT_VP_SWIM_AGAIN, 4);
            BSOS_SetLampState(SHOOT_AGAIN, 0);
            BallTimeInTrough = CurrentTime;
            returnState = MACHINE_STATE_NORMAL_GAMEPLAY;

            // Only 1 ball save if one ball in play
            if (NumberOfBallsInPlay==1) {
              BallSaveEndTime = 0;
            }
          } else if ( RescueFromTheDeepEndTime!=0 && CurrentTime<(RescueFromTheDeepEndTime+BALL_SAVE_GRACE_PERIOD) ) {
            BSOS_PushToTimedSolenoidStack(SOL_OUTHOLE, 4, CurrentTime + 100);
            AutoPlungeTime = CurrentTime + 100;
            LastTroughSwitchCheck = AutoPlungeTime;
            QueueNotification(SOUND_EFFECT_VP_RESCUE_FROM_THE_DEEP, 6);
            RescueFromTheDeepAvailable = false;
            BallTimeInTrough = CurrentTime;
            returnState = MACHINE_STATE_NORMAL_GAMEPLAY;
          } else {

            NumberOfBallsInPlay -= 1;
            if (NumberOfBallsInPlay==0) {
              ShowPlayerScores(0xFF, false, false);
              FeedingFrenzySpins[CurrentPlayer] += CurrentFeedingFrenzy;
              ExploreTheDepthsHits[CurrentPlayer] += CurrentExploreTheDepths;
              SharpShooterHits[CurrentPlayer] += CurrentSharpShooter;
              Audio.StopAllAudio();
              returnState = MACHINE_STATE_COUNTDOWN_BONUS;
            }
          }
        }
      }
    }
  } else {
    BallTimeInTrough = 0;
  }

  return returnState;
}



unsigned long CountdownStartTime = 0;
unsigned long LastCountdownReportTime = 0;
unsigned long BonusCountDownEndTime = 0;

int CountdownBonus(boolean curStateChanged) {

  // If this is the first time through the countdown loop
  if (curStateChanged) {
    BSOS_SetLampState(BALL_IN_PLAY, 1, 0, 250);

    CountdownStartTime = CurrentTime;
    ShowBonusOnTree(Bonus);

    LastCountdownReportTime = CountdownStartTime;
    BonusCountDownEndTime = 0xFFFFFFFF;
  }

  if ((CurrentTime - LastCountdownReportTime) > 200) {

    if (Bonus > 0) {

      // Only give sound & score if this isn't a tilt
      if (NumTiltWarnings <= MaxTiltWarnings) {
        PlaySoundEffect(SOUND_EFFECT_BONUS_COUNT + (BonusX-1));
        CurrentScores[CurrentPlayer] += (unsigned long)1000*((unsigned long)BonusX);
      }

      Bonus -= 1;
      ShowBonusOnTree(Bonus);
    } else if (BonusCountDownEndTime == 0xFFFFFFFF) {
      PlaySoundEffect(SOUND_EFFECT_BALL_OVER);
      BSOS_SetLampState(BONUS_1, 0);
      BonusCountDownEndTime = CurrentTime + 1000;
    }
    LastCountdownReportTime = CurrentTime;
  }

  if (CurrentTime > BonusCountDownEndTime) {

    // Reset any lights & variables of goals that weren't completed

    BonusCountDownEndTime = 0xFFFFFFFF;
    return MACHINE_STATE_BALL_OVER;
  }

  return MACHINE_STATE_COUNTDOWN_BONUS;
}



void CheckHighScores() {
  unsigned long highestScore = 0;
  int highScorePlayerNum = 0;
  for (int count = 0; count < CurrentNumPlayers; count++) {
    if (CurrentScores[count] > highestScore) highestScore = CurrentScores[count];
    highScorePlayerNum = count;
  }

  if (highestScore > HighScore) {
    HighScore = highestScore;
    if (HighScoreReplay) {
      AddCredit(false, 3);
      BSOS_WriteULToEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE) + 3);
    }
    BSOS_WriteULToEEProm(BSOS_HIGHSCORE_EEPROM_START_BYTE, highestScore);
    BSOS_WriteULToEEProm(BSOS_TOTAL_HISCORE_BEATEN_START_BYTE, BSOS_ReadULFromEEProm(BSOS_TOTAL_HISCORE_BEATEN_START_BYTE) + 1);

    for (int count = 0; count < 4; count++) {
      if (count == highScorePlayerNum) {
        BSOS_SetDisplay(count, CurrentScores[count], true, 2);
      } else {
        BSOS_SetDisplayBlank(count, 0x00);
      }
    }

    BSOS_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime, true);
    BSOS_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime + 300, true);
    BSOS_PushToTimedSolenoidStack(SOL_KNOCKER, 3, CurrentTime + 600, true);
  }
}






unsigned long MatchSequenceStartTime = 0;
unsigned long MatchDelay = 150;
byte MatchDigit = 0;
byte NumMatchSpins = 0;
byte ScoreMatches = 0;

int ShowMatchSequence(boolean curStateChanged) {
  if (!MatchFeature) return MACHINE_STATE_ATTRACT;

  if (curStateChanged) {
    MatchSequenceStartTime = CurrentTime;
    MatchDelay = 1500;
//    MatchDigit = random(0, 10);
    MatchDigit = CurrentTime%10;
    NumMatchSpins = 0;
    BSOS_SetLampState(MATCH, 1, 0);
    BSOS_SetDisableFlippers();
    ScoreMatches = 0;
    BSOS_SetLampState(BALL_IN_PLAY, 0);
  }

  if (NumMatchSpins < 40) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      MatchDigit += 1;
      if (MatchDigit > 9) MatchDigit = 0;
      //PlaySoundEffect(10+(MatchDigit%2));
      PlaySoundEffect(SOUND_EFFECT_MATCH_SPIN);
      BSOS_SetDisplayBallInPlay((int)MatchDigit * 10);
      MatchDelay += 50 + 4 * NumMatchSpins;
      NumMatchSpins += 1;
      BSOS_SetLampState(MATCH, NumMatchSpins % 2, 0);

      if (NumMatchSpins == 40) {
        BSOS_SetLampState(MATCH, 0);
        MatchDelay = CurrentTime - MatchSequenceStartTime;
      }
    }
  }

  if (NumMatchSpins >= 40 && NumMatchSpins <= 43) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      if ( (CurrentNumPlayers > (NumMatchSpins - 40)) && ((CurrentScores[NumMatchSpins - 40] / 10) % 10) == MatchDigit) {
        ScoreMatches |= (1 << (NumMatchSpins - 40));
        AddSpecialCredit();
        MatchDelay += 1000;
        NumMatchSpins += 1;
        BSOS_SetLampState(MATCH, 1);
      } else {
        NumMatchSpins += 1;
      }
      if (NumMatchSpins == 44) {
        MatchDelay += 5000;
      }
    }
  }

  if (NumMatchSpins > 43) {
    if (CurrentTime > (MatchSequenceStartTime + MatchDelay)) {
      return MACHINE_STATE_ATTRACT;
    }
  }

  for (int count = 0; count < 4; count++) {
    if ((ScoreMatches >> count) & 0x01) {
      // If this score matches, we're going to flash the last two digits
      if ( (CurrentTime / 200) % 2 ) {
        BSOS_SetDisplayBlank(count, BSOS_GetDisplayBlank(count) & 0x0F);
      } else {
        BSOS_SetDisplayBlank(count, BSOS_GetDisplayBlank(count) | 0x30);
      }
    }
  }

  return MACHINE_STATE_MATCH_MODE;
}



int RunGamePlayMode(int curState, boolean curStateChanged) {
  int returnState = curState;
  unsigned long scoreAtTop = CurrentScores[CurrentPlayer];
  unsigned long scoreMultiplier = 1;
  if (MiniGamesRunning) {
    scoreMultiplier = (unsigned long)CountBits(MiniGamesRunning);
  }

  // Very first time into gameplay loop
  if (curState == MACHINE_STATE_INIT_GAMEPLAY) {
    returnState = InitGamePlay();
  } else if (curState == MACHINE_STATE_INIT_NEW_BALL) {
    returnState = InitNewBall(curStateChanged, CurrentPlayer, CurrentBallInPlay);
  } else if (curState == MACHINE_STATE_NORMAL_GAMEPLAY) {
    returnState = ManageGameMode();
  } else if (curState == MACHINE_STATE_COUNTDOWN_BONUS) {
    returnState = CountdownBonus(curStateChanged);
    ShowPlayerScores(CurrentPlayer, (BallFirstSwitchHitTime==0)?true:false, (BallFirstSwitchHitTime>0 && ((CurrentTime-LastTimeScoreChanged)>2000))?true:false);
  } else if (curState == MACHINE_STATE_BALL_OVER) {
    StandupsHit[ CurrentPlayer] = CurrentStandupsHit;
    if (SamePlayerShootsAgain) {
      returnState = MACHINE_STATE_INIT_NEW_BALL;
    } else {
      CurrentPlayer += 1;
      if (CurrentPlayer >= CurrentNumPlayers) {
        CurrentPlayer = 0;
        CurrentBallInPlay += 1;
      }
      // Reset score at top since player changed
      CurrentStandupsHit = StandupsHit[CurrentPlayer];
      scoreAtTop = CurrentScores[CurrentPlayer];

      if (CurrentBallInPlay > BallsPerGame) {
        CheckHighScores();
        PlaySoundEffect(SOUND_EFFECT_GAME_OVER);
        SetPlayerLamps(0);
        for (int count = 0; count < CurrentNumPlayers; count++) {
          BSOS_SetDisplay(count, CurrentScores[count], true, 2);
        }

        returnState = MACHINE_STATE_MATCH_MODE;
      }
      else returnState = MACHINE_STATE_INIT_NEW_BALL;
    }
  } else if (curState == MACHINE_STATE_MATCH_MODE) {
    returnState = ShowMatchSequence(curStateChanged);
  }

  byte switchHit;
  unsigned long lastBallFirstSwitchHitTime = BallFirstSwitchHitTime;

  if (NumTiltWarnings <= MaxTiltWarnings) {
    while ( (switchHit = BSOS_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY ) {

      switch (switchHit) {
        case SW_SLAM:
          //          BSOS_DisableSolenoidStack();
          //          BSOS_SetDisableFlippers(true);
          //          BSOS_TurnOffAllLamps();
          //          BSOS_SetLampState(GAME_OVER, 1);
          //          delay(1000);
          //          return MACHINE_STATE_ATTRACT;
          break;
        case SW_TILT:
          // This should be debounced
          if ((CurrentTime - LastTiltWarningTime) > TILT_WARNING_DEBOUNCE_TIME) {
            LastTiltWarningTime = CurrentTime;
            NumTiltWarnings += 1;
            if (NumTiltWarnings > MaxTiltWarnings) {
              BSOS_DisableSolenoidStack();
              if (BSOS_ReadSingleSwitchState(SW_SHOOTER_LANE)) {
                // Ball stuck in shooter lane, so kick it
                BSOS_FireContinuousSolenoid(0x10, 15);
              }
              BSOS_SetDisableFlippers(true);
              BSOS_TurnOffAllLamps();
              BSOS_SetLampState(TILT, 1);
              Audio.StopAllAudio();
            }
            PlaySoundEffect(SOUND_EFFECT_TILT_WARNING);
          }
          break;
        case SW_SELF_TEST_SWITCH:
          returnState = MACHINE_STATE_TEST_LIGHTS;
          SetLastSelfTestChangedTime(CurrentTime);
          break;
        case SW_SHOOTER_LANE:
          if (AutoPlungeTime) {
            AutoPlungeTime = 0;
            PlaySoundEffect(SOUND_EFFECT_ADD_CREDIT);
            BSOS_FireContinuousSolenoid(0x10, 12);
            LastTroughSwitchCheck = CurrentTime;
          }
          break;
        case SW_LEFT_INLANE:
          CurrentScores[CurrentPlayer] += ((unsigned long)RolloverValue)*(unsigned long)1000;
          AddToBonus(1);
          PlaySoundEffect(SOUND_EFFECT_LEFT_INLANE);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_RIGHT_INLANE:
          CurrentScores[CurrentPlayer] += 3000;
          AddToBonus(3);
          PlaySoundEffect(SOUND_EFFECT_RIGHT_INLANE);
          if (RescueFromTheDeepAvailable) {
            RescueFromTheDeepEndTime = CurrentTime + RESCUE_FROM_THE_DEEP_TIME;
          }
          if (NumberOfStandupClears==1 && !ExtraBallCollected) {
            ExtraBallCollected = true;
            // Set shoot again or give score
            if (TournamentScoring) {
              CurrentScores[CurrentPlayer] += (unsigned long)ExtraBallValue;
            } else {
              SamePlayerShootsAgain = true;
              QueueNotification(SOUND_EFFECT_VP_SWIM_AGAIN, 4);
            }
          }
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_RIGHT_OUTLANE:
          CurrentScores[CurrentPlayer] += 500;
          PlaySoundEffect(SOUND_EFFECT_RIGHT_OUTLANE);
          if (NumberOfStandupClears==StandupSpecialLevel && !SpecialCollected) {
            SpecialCollected = true;
            // Set shoot again or give score
            if (TournamentScoring) {
              CurrentScores[CurrentPlayer] += (unsigned long)SpecialValue;
            } else {
              
            }
          }
          if (BallSaveEndTime!=0) {
            BallSaveEndTime += 3000;
          }          
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
        break;
        case SW_10_PTS:
          CurrentScores[CurrentPlayer] += 10;
          PlaySoundEffect(SOUND_EFFECT_10PT_SWITCH);
          break;
        case SW_LEFT_SPINNER:
          if (GameMode==GAME_MODE_SKILL_SHOT) {
            CurrentScores[CurrentPlayer] += 10000;            
            PlaySoundEffect(SOUND_EFFECT_LEFT_SPINNER);
          } else if ((MiniGamesRunning & MINI_GAME_FEEDING_FRENZY_FLAG)) {
            CurrentScores[CurrentPlayer] += (unsigned long)5000*(unsigned long)scoreMultiplier;
            PlaySoundEffect(SOUND_EFFECT_FEEDING_FRENZY);
            if (CurrentFeedingFrenzy<255) CurrentFeedingFrenzy += 1;
          } else {
            unsigned long scoreAddition = 0;
            if (LastStandupTargetHit&STANDUP_AMBER_MASK) scoreAddition += 400;
            if (LastStandupTargetHit&STANDUP_WHITE_MASK) scoreAddition += 400;
            if (LastStandupTargetHit&STANDUP_PURPLE_MASK) scoreAddition += 1000;
            if (CurrentStandupsHit&STANDUP_AMBER_MASK) scoreAddition += 400;
            if (CurrentStandupsHit&STANDUP_WHITE_MASK) scoreAddition += 400;
            if (CurrentStandupsHit&STANDUP_PURPLE_MASK) scoreAddition += 1000;
            CurrentScores[CurrentPlayer] += (200 + (unsigned long)scoreAddition);
            if (LastSpinnerHitTime!=0 && LastSpinnerSide==2) AlternatingSpinnerCount += 1;
            LastSpinnerHitTime = CurrentTime;
            LastSpinnerSide = 1;
            PlaySoundEffect(SOUND_EFFECT_LEFT_SPINNER);
          }
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_RIGHT_SPINNER:
          if ((MiniGamesRunning & MINI_GAME_FEEDING_FRENZY_FLAG)) {
            CurrentScores[CurrentPlayer] += (unsigned long)5000*(unsigned long)scoreMultiplier;
            PlaySoundEffect(SOUND_EFFECT_FEEDING_FRENZY);
            if (CurrentFeedingFrenzy<255) CurrentFeedingFrenzy += 1;
          } else if (GameMode!=GAME_MODE_SKILL_SHOT) {
            unsigned long scoreAddition = 0;
            if (LastStandupTargetHit&STANDUP_YELLOW_MASK) scoreAddition += 400;
            if (LastStandupTargetHit&STANDUP_GREEN_MASK) scoreAddition += 400;
            if (LastStandupTargetHit&STANDUP_PURPLE_MASK) scoreAddition += 1000;
            if (CurrentStandupsHit&STANDUP_YELLOW_MASK) scoreAddition += 400;
            if (CurrentStandupsHit&STANDUP_GREEN_MASK) scoreAddition += 400;
            if (CurrentStandupsHit&STANDUP_PURPLE_MASK) scoreAddition += 1000;
            CurrentScores[CurrentPlayer] += (200 + (unsigned long)scoreAddition);
            PlaySoundEffect(SOUND_EFFECT_RIGHT_SPINNER);
            if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
            if (LastSpinnerHitTime!=0 && LastSpinnerSide==1) AlternatingSpinnerCount += 1;
            LastSpinnerHitTime = CurrentTime;
            LastSpinnerSide = 2;
          }
          break;
        case SW_SAUCER:
          // We only count a saucer hit if it hasn't happened in the last 500ms
          // (software debounce)
          if (SaucerHitTime==0 || (CurrentTime-SaucerHitTime)>500) {
            SaucerHitTime = CurrentTime;
            ShowSaucerHit = SaucerValue;

            if (JackpotLit) {
              FeedingFrenzySpins[CurrentPlayer] += CurrentFeedingFrenzy;
              ExploreTheDepthsHits[CurrentPlayer] += CurrentExploreTheDepths;
              SharpShooterHits[CurrentPlayer] += CurrentSharpShooter;
              CurrentFeedingFrenzy = 0;
              CurrentExploreTheDepths = 0;
              CurrentSharpShooter = 0;
              QueueNotification(SOUND_EFFECT_VP_JACKPOT, 3);
              CurrentScores[CurrentPlayer] += (unsigned long)FeedingFrenzySpins[CurrentPlayer]*1000;
              CurrentScores[CurrentPlayer] += (unsigned long)ExploreTheDepthsHits[CurrentPlayer]*10000;
              CurrentScores[CurrentPlayer] += (unsigned long)SharpShooterHits[CurrentPlayer]*10000;
              JackpotLit = false;
            } else {
              CurrentScores[CurrentPlayer] += 1000*((unsigned long)SaucerValue);
              switch(SaucerValue) {
                case 5: PlaySoundEffect(SOUND_EFFECT_SAUCER_HIT_5K); break;
                case 10: PlaySoundEffect(SOUND_EFFECT_SAUCER_HIT_10K); break;
                case 20: PlaySoundEffect(SOUND_EFFECT_SAUCER_HIT_20K); break;
                case 30: PlaySoundEffect(SOUND_EFFECT_SAUCER_HIT_30K); break;
              }
            }
  
            if (GameMode!=GAME_MODE_SKILL_SHOT) {
              NextSaucerReduction = CurrentTime + SAUCER_DISPLAY_DURATION + 10000;
              if (SaucerValue==5) SaucerValue = 10;
              else if (SaucerValue<30) SaucerValue += 10;
            }
            if (GameMode==GAME_MODE_MINI_GAME_QUALIFIED) {
              SetGameMode(GAME_MODE_MINI_GAME_ENGAGED);
              BSOS_PushToTimedSolenoidStack(SOL_SAUCER, 5, CurrentTime + MODE_START_DISPLAY_DURATION); 
            } else {
              BSOS_PushToTimedSolenoidStack(SOL_SAUCER, 5, CurrentTime + SAUCER_DISPLAY_DURATION); 
            }
          }
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_ROLLOVER:
          if (GameMode==GAME_MODE_SKILL_SHOT) {
            CurrentScores[CurrentPlayer] += 8000;
            RolloverValue = 6;
            PlaySoundEffect(SOUND_EFFECT_ROLLOVER_SKILL_SHOT);
          } else {
            CurrentScores[CurrentPlayer] += 1000*((unsigned long)RolloverValue);
            PlaySoundEffect(SOUND_EFFECT_ROLLOVER);
            RolloverValue += 2;
            if (RolloverValue>14) RolloverValue = 14;
          }
          RolloverFlashEndTime = CurrentTime + ROLLOVER_FLASH_DURATION;
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_OUTHOLE:
          break;
        case SW_DROP_TARGET_1:
        case SW_DROP_TARGET_2:
        case SW_DROP_TARGET_3:
        case SW_DROP_TARGET_4:
        case SW_DROP_TARGET_5:
          if (GameMode!=GAME_MODE_SKILL_SHOT || (CurrentTime-GameModeStartTime)>500) {
            HandleDropTargetHit(switchHit, scoreMultiplier);
            if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          }
          break;
        case SW_TOP_BUMPER:
          CurrentScores[CurrentPlayer] += (unsigned long)100*(unsigned long)scoreMultiplier;
          PlaySoundEffect(SOUND_EFFECT_TOP_BUMPER_HIT);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_BOTTOM_BUMPER:
          CurrentScores[CurrentPlayer] += (unsigned long)100*(unsigned long)scoreMultiplier;
          PlaySoundEffect(SOUND_EFFECT_BOTTOM_BUMPER_HIT);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_WHITE:
        case SW_GREEN:
        case SW_AMBER:
        case SW_YELLOW:
        case SW_PURPLE:
          HandleStandupHit(switchHit, scoreMultiplier);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_UL_SLING:
        case SW_UR_SLING:
          CurrentScores[CurrentPlayer] += 10;
          AddToBonus(1);
          PlaySoundEffect(SOUND_EFFECT_UPPER_SLING);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_LL_SLING:
        case SW_LR_SLING:
          CurrentScores[CurrentPlayer] += 10;
          PlaySoundEffect(SOUND_EFFECT_LOWER_SLING);
          if (BallFirstSwitchHitTime == 0) BallFirstSwitchHitTime = CurrentTime;
          break;
        case SW_COIN_1:
        case SW_COIN_2:
        case SW_COIN_3:
          AddCoinToAudit(switchHit);
          AddCredit(true, 1);
          break;
        case SW_CREDIT_RESET:
          if (CurrentBallInPlay < 2) {
            // If we haven't finished the first ball, we can add players
            AddPlayer();
          } else {
            // If the first ball is over, pressing start again resets the game
            if (Credits >= 1 || FreePlayMode) {
              if (!FreePlayMode) {
                Credits -= 1;
                BSOS_WriteByteToEEProm(BSOS_CREDITS_EEPROM_BYTE, Credits);
                BSOS_SetDisplayCredits(Credits);
              }
              returnState = MACHINE_STATE_INIT_GAMEPLAY;
            }
          }
          if (DEBUG_MESSAGES) {
            Serial.write("Start game button pressed\n\r");
          }
          break;
        }
      }
    } else {
      // We're tilted, so just wait for outhole
      while ( (switchHit = BSOS_PullFirstFromSwitchStack()) != SWITCH_STACK_EMPTY ) {
        switch (switchHit) {
          case SW_SELF_TEST_SWITCH:
            returnState = MACHINE_STATE_TEST_LIGHTS;
            SetLastSelfTestChangedTime(CurrentTime);
            break;
          case SW_SAUCER:
            BSOS_PushToSolenoidStack(SOL_SAUCER, 5, true); 
            break;
          case SW_COIN_1:
          case SW_COIN_2:
          case SW_COIN_3:
            AddCoinToAudit(switchHit);
            AddCredit(true, 1);
            break;
        }
      }
    }

//  if (bonusAtTop != Bonus) {
//    ShowBonusOnTree(Bonus);
//  }

  if (lastBallFirstSwitchHitTime==0 && BallFirstSwitchHitTime!=0) {
    BallSaveEndTime = BallFirstSwitchHitTime + ((unsigned long)BallSaveNumSeconds)*1000;
  }
  if (CurrentTime>(BallSaveEndTime+BALL_SAVE_GRACE_PERIOD)) {
    BallSaveEndTime = 0;
  }

  if (!ScrollingScores && CurrentScores[CurrentPlayer] > BALLY_STERN_OS_MAX_DISPLAY_SCORE) {
    CurrentScores[CurrentPlayer] -= BALLY_STERN_OS_MAX_DISPLAY_SCORE;
  }

  if (scoreAtTop != CurrentScores[CurrentPlayer]) {
    LastTimeScoreChanged = CurrentTime;
    if (!TournamentScoring) {
      for (int awardCount = 0; awardCount < 3; awardCount++) {
        if (AwardScores[awardCount] != 0 && scoreAtTop < AwardScores[awardCount] && CurrentScores[CurrentPlayer] >= AwardScores[awardCount]) {
          // Player has just passed an award score, so we need to award it
          if (((ScoreAwardReplay >> awardCount) & 0x01) == 0x01) {
            AddSpecialCredit();
          } else if (!ExtraBallCollected) {
            ExtraBallCollected = true;
            SamePlayerShootsAgain = true;
            BSOS_SetLampState(SHOOT_AGAIN, SamePlayerShootsAgain);
            QueueNotification(SOUND_EFFECT_VP_EXTRA_BALL, 5);
          }
        }
      }
    }
  
  }

  return returnState;
}


void loop() {

  BSOS_DataRead(0);
  CurrentTime = millis();
  int newMachineState = MachineState;

  if (MachineState < 0) {
    newMachineState = RunSelfTest(MachineState, MachineStateChanged);
  } else if (MachineState == MACHINE_STATE_ATTRACT) {
    newMachineState = RunAttractMode(MachineState, MachineStateChanged);
  } else {
    newMachineState = RunGamePlayMode(MachineState, MachineStateChanged);
  }

  if (newMachineState != MachineState) {
    MachineState = newMachineState;
    MachineStateChanged = true;
  } else {
    MachineStateChanged = false;
  }

  BSOS_ApplyFlashToLamps(CurrentTime);
  BSOS_UpdateTimedSolenoidStack(CurrentTime);
  Audio.Update(CurrentTime);

}
