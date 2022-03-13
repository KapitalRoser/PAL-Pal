#ifndef SEARCHLOGIC_H
#define SEARCHLOGIC_H

#include "processCore.h"

enum instr {reroll,memcard,rumble,name};

enum class WantedShininess
  {
    notShiny,
    shiny,
    any
  };
enum gender
{
  Male = 0,
  Female,
  AnyGender
};
enum BattleNowTeamLeaderPlayer
  {
    Mewtwo = 0,
    Mew,
    Deoxys,
    Rayquaza,
    Jirachi
  };
enum BattleNowTeamLeaderEnemy
{
  Articuno = 0,
  Zapdos,
  Moltres,
  Kangaskhan,
  Latias
};
static const std::array<std::array<int, 2>, 10> s_quickBattleTeamMaxBaseHPStat = {{{{322, 340}},
                                                                                   {{310, 290}},
                                                                                   {{210, 620}},
                                                                                   {{320, 230}},
                                                                                   {{310, 310}},
                                                                                   {{290, 310}},
                                                                                   {{290, 270}},
                                                                                   {{290, 250}},
                                                                                   {{320, 270}},
                                                                                   {{270, 230}}}};

//These two structs are purely for UI purposes.
struct battleNowTeamInfo {
std::string playerLead = "";
std::string playerOther = "";
std::string oppLead = ""; //technically redundant but I like having all 4 visually displayed.
std::string oppOther = "";//As long as having the more pokes doesn't increase stress on end user then this is fine.

int playerLeadHp = 0;
int playerOtherHp = 0;
int oppLeadHp = 0;
int oppOtherHp = 0;

u32 preSeed;
u32 postSeed;
};

struct path {
    std::vector<int>instructions = {0,0,0,0};
    battleNowTeamInfo battleNow;
};

struct searchResult {
    PokemonProperties candidate;
    u32 titleSeed;
    u32 listingSeed;
    path optimalPath;
};


u32 complexPID(u32& seed, const u32 hTrainerId, const u32 lTrainerId, const u32 dummyId, const WantedShininess shininess,
const s8 wantedGender, const u32 genderRatio, const s8 wantedNature);
std::array<u8, 6> generateEVs(u32& seed, const bool allowUnfilledEV, const bool endPrematurely);
std::vector<int> mGenerateBattleTeam(u32 &seed, const std::vector<int> criteria,std::vector<int>m_criteria);
void condensedGenerateMon (PokemonProperties &main, u32 &seed, int genderRatio);
int seekCallsToTargetEevee(u32 originSeed, u32 TargetSeed);
int testRerolls(u32 testSeed,int &callsToTarget, std::vector<u32>&rerollSeeds);
bool CTTAcceptable(u32 seed,int ctt);
bool foundRunnable(PokemonProperties candidate, PokemonRequirements reqs);
u32 myRollRNGToBattleMenu(u32 &seed, bool fromBoot, bool validMemcard);
int standardPathFind(int rem, std::vector<int>&instructions, std::vector<u32> &rerollSeeds);

int completePath(int rem, int instructions[4]); //i think deprecated?
void popReroll(int &rem, int instructions[4],std::vector<u32>&rerollSeeds);//maybe deprecated, maybe useful in stdPathFind
int evenPathFind(int rem, int instructions[4],std::vector<u32>&rerollSeeds);//definitely deprecated but I worked so hard on this.
std::vector<u32> autoRollN(u32&seed,int n,std::vector<int>m_criteria);//unused but could be useful in another application.

void readReqsConfig(PokemonRequirements &inputReqs, std::ifstream &config);
PokemonRequirements setPokeReqs();
battleNowTeamInfo getBattleNowResultString(std::vector<int>m_criteria);
battleNowTeamInfo getBattleTeamInfo(u32 seed);
searchResult findTarget (u32 inputSeed, bool firstRun);
path findPath(u32 inputSeed, u32 targetSeed);
searchResult searchForEevee (u32 inputSeed, u32 currentSeed, bool firstRun);


#endif // SEARCHLOGIC_H
