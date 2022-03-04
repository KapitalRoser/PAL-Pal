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

};

struct searchResult {
    PokemonProperties candidate;
    u32 titleSeed;
    u32 listingSeed;
    u32 rerollSeed;
    std::vector<int>instructions = {0,0,0,0};
    //std::vector<u32>candidateSeeds; //when I go "next", this will get replaced??? -- should I be updating the same object or creating a parent?
    //this should probably be a parent vector???
    //Include info on reroll result. leads + hps.
    battleNowTeamInfo battleNow;

};

u32 complexPID(u32& seed, const u32 hTrainerId, const u32 lTrainerId, const u32 dummyId, const WantedShininess shininess,
const s8 wantedGender, const u32 genderRatio, const s8 wantedNature)
{
  u32 pid = 0;
  int nbrPidAttemptLeft = 3145728;
  while (nbrPidAttemptLeft > 0)
  {
    // A personality ID is generated as candidate, high then low 16 bits
    pid = simplePID(seed);
    nbrPidAttemptLeft--;

    // If we want a gender AND the gender of the pokemon is uncertain
    if (wantedGender != -1 && !(genderRatio == 0xff || genderRatio == 0xfe || genderRatio == 0x00))
    {
      if (wantedGender == 2)
      {
        u8 pokemonIdGender = genderRatio > (dummyId & 0xff) ? 1 : 0;
        u8 idGender = genderRatio > (pid & 0xff) ? 1 : 0;
        if (pokemonIdGender != idGender)
          continue;
      }
      else
      {
        u8 idGender = genderRatio > (pid & 0xff) ? 1 : 0;
        if (idGender != wantedGender)
          continue;
      }
    }

    // Reroll until we have the correct nature in the perosnality ID
    if (wantedNature != -1 && pid % 25 != wantedNature)
      continue;

    if (shininess != WantedShininess::any)
    {
      if (shininess == WantedShininess::notShiny)
      {
        if (!isPidShiny(lTrainerId, hTrainerId, pid))
          return pid;
      }
      else if (shininess == WantedShininess::shiny)
      {
        if (isPidShiny(lTrainerId, hTrainerId, pid))
          return pid;
      }
    }
    else
    {
      return pid;
    }
  }
  return pid;
}
std::array<u8, 6> generateEVs(u32& seed, const bool allowUnfilledEV, const bool endPrematurely)
{
  std::array<u8, 6> EVs = {0};
  int sumEV = 0;
  for (int j = 0; j < 101; j++)
  {
    sumEV = 0;
    for (int k = 0; k < 6; k++)
    {
      EVs[k] += (LCG(seed) >> 16) & 0xFF;
      sumEV += EVs[k];
    }
    if (allowUnfilledEV)
    {
      if (sumEV <= 510)
      {
        return EVs;
      }
      else
      {
        if (j >= 100)
          continue;
        EVs.fill(0);
        continue;
      }
    }
    else if (sumEV == 510)
    {
      return EVs;
    }
    else if (sumEV <= 490)
    {
      continue;
    }
    else if (sumEV >= 530)
    {
      if (j >= 100)
        continue;
      EVs.fill(0);
      continue;
    }
    else
    {
      break;
    }
  }
  bool goodSum = false;
  while (!goodSum && !endPrematurely)
  {
    for (int k = 0; k < 6; k++)
    {
      if (sumEV < 510)
      {
        if (EVs[k] < 255)
        {
          EVs[k]++;
          sumEV++;
        }
      }
      else if (sumEV > 510)
      {
        if (EVs[k] != 0)
        {
          EVs[k]--;
          sumEV--;
        }
      }

      if (sumEV == 510)
        goodSum = true;
    }
  }
  return EVs;
}
std::vector<int> mGenerateBattleTeam(u32 &seed, const std::vector<int> criteria,std::vector<int>m_criteria)
{
  // Player trainer name generation
  LCG(seed);
  // Player team index
  int playerTeamIndex = (LCG(seed) >> 16) % 5;
  if (playerTeamIndex != criteria[0] && criteria[0] != -1)
    return {};
  // Enemy team index
  int enemyTeamIndex = (LCG(seed) >> 16) % 5;
  if (enemyTeamIndex != criteria[1] && criteria[1] != -1)
    return {};
  // modulo 6 -- decides arena!
  LCG(seed);

  // Enemy gen

  std::vector<int> obtainedEnemyHP;
  // The enemy trainer ID is generated, low then high 16 bits
  u32 lTrainerId = LCG(seed) >> 16;
  u32 hTrainerId = LCG(seed) >> 16;
  // Pokemon gen
  for (int i = 0; i < 2; i++)
  {
    // Dummy personality ID (doesn't matter)
    LCG(seed);
    LCG(seed);
    // HP, ATK, DEF IV
    LCG(seed);
    u32 hpIV = (seed >> 16) & 31;
    // SPEED, SPATK, SPDEF IV
    LCG(seed);
    // Ability
    LCG(seed);
    complexPID(seed, hTrainerId, lTrainerId, 0, WantedShininess::notShiny,-1,0,-1);
    std::array<u8, 6> EVs = generateEVs(seed, false, false);
    int hp = EVs[0] / 4 + hpIV + s_quickBattleTeamMaxBaseHPStat[enemyTeamIndex + 5][i];
    if (hp != criteria[4 + i] && criteria[4 + i] != -1)
      return {};
    obtainedEnemyHP.push_back(hp);
  }

  // modulo 6 ???
  LCG(seed);

  // Player gen

  std::vector<int> obtainedPlayerHP;
  // The player trainer ID is generated, low then high 16 bits
  lTrainerId = LCG(seed) >> 16;
  hTrainerId = LCG(seed) >> 16;
  // Pokemon gen
  for (int i = 0; i < 2; i++)
  {
    // Dummy personality ID (doesn't matter)
    LCG(seed);
    LCG(seed);
    // HP, ATK, DEF IV
    LCG(seed);
    u32 hpIV = (seed >> 16) & 31;
    // SPEED, SPATK, SPDEF IV
    LCG(seed);
    // Ability
    LCG(seed);
    complexPID(seed, hTrainerId, lTrainerId, 0, WantedShininess::notShiny,-1,0,-1);
    std::array<u8, 6> EVs = generateEVs(seed, false, false);
    int hp = EVs[0] / 4 + hpIV + s_quickBattleTeamMaxBaseHPStat[playerTeamIndex][i];
    if (hp != criteria[2 + i] && criteria[2 + i] != -1)
      return {};
    obtainedPlayerHP.push_back(hp);
  }
    m_criteria.clear();
    m_criteria.push_back(playerTeamIndex);
    m_criteria.push_back(enemyTeamIndex);
    m_criteria.insert(m_criteria.end(), obtainedPlayerHP.begin(),
                                   obtainedPlayerHP.end());
    m_criteria.insert(m_criteria.end(), obtainedEnemyHP.begin(),
                                   obtainedEnemyHP.end());
  return m_criteria;
}
void condensedGenerateMon (PokemonProperties &main, u32 &seed, int genderRatio){
    u32 TID, SID;
    setIDs(seed,TID,SID);
    main.trainerId = TID;
    LCGn(seed,2); //Dummy Pid
    extractIVs(main,seed);
    LCG(seed); //Ability
    fillStarterGenHiddenPowerInfo(main);
    u32 PID = complexPID(seed, SID, TID, 0, WantedShininess::any,-1,0,-1);
    //u32 PID = simplePID(seed); //for xd, is there any difference between using complexpid and simplepid? Can't tell any in-game filters
    main.isShiny = isPidShiny(TID,SID,PID);
    main.genderIndex = getPidGender(genderRatio,PID);
    main.natureIndex = PID % 25;
}
int seekCallsToTargetEevee(u32 originSeed, u32 TargetSeed){
  return findGap(originSeed,LCGn_BACK(TargetSeed,1002),1); //intended for generation, otherwise change constant
}
int testRerolls(u32 testSeed,int &callsToTarget, std::vector<u32>&rerollSeeds,std::vector<int>m_criteria){
  bool maxedOut = false;
  int rolls = 0;
  while(!maxedOut){
  mGenerateBattleTeam(testSeed,m_criteria,m_criteria);
  int distance = findGap(rerollSeeds.back(),testSeed,1);
  if (distance <= callsToTarget){
    rerollSeeds.push_back(testSeed);
    callsToTarget -= distance;
    rolls++;
  } else {
    //break case
    maxedOut = true;
  }
}
return rolls;
//Don't forget to update seed.
}
bool CTTAcceptable(u32 seed,int ctt){
if (ctt >= 1009 || ctt % 2 == 0){
    return true;
  }
  //This system is pretty janky, but it works.
  std::vector<u32>temporary  = {seed};
  std::vector<int> m_criteria = {-1, -1, -1, -1, -1, -1};
  testRerolls(seed,ctt,temporary,m_criteria);
  if(temporary.size() == 2){
    if (ctt % 2 == 0){
      return true;
    }
  }
  while (temporary.size() > 2){
    if (ctt % 2 == 0){
      return true;
    } else {
      if (temporary.size() > 1){
        ctt+= findGap(temporary.back(),temporary.at(temporary.size()-2),0);
        temporary.pop_back();
      } else {
        temporary.clear();
      }
    }
  }
  if (ctt % 2 == 0){
      return true;
  }
  return false;
}
bool foundRunnable(PokemonProperties candidate, PokemonRequirements reqs){
  return (
    candidate.hpIV >= reqs.hpIV &&
    candidate.atkIV >= reqs.atkIV &&
    candidate.defIV >= reqs.defIV &&
    candidate.spAtkIV >= reqs.spAtkIV &&
    candidate.spDefIV >= reqs.spDefIV &&
    candidate.speedIV >= reqs.speedIV &&
    candidate.hiddenPowerPower >= reqs.hiddenPowerPower &&
    reqs.validHPTypes[candidate.hiddenPowerTypeIndex] &&
    reqs.validNatures[candidate.natureIndex] &&
    (reqs.isShiny == 2 || reqs.isShiny == candidate.isShiny)
  );
}
u32 myRollRNGToBattleMenu(u32 &seed, bool fromBoot, bool validMemcard)
{
  // bool fromBoot = false; //false would be from title screen.
  // bool validMemcard = true;
  if (fromBoot){
    LCGn(seed, 1137);
  }
  if (!validMemcard){
    LCGn(seed,1009); //w no memcard.
  }
  LCG(seed);
  LCG(seed);

  // Some dummy team gen going on...
  for (int i = 0; i < 4; i++)
  {
    // The player trainer ID is generated, low then high 16 bits
    u32 lTrainerId = LCG(seed) >> 16;
    u32 hTrainerId = LCG(seed) >> 16;
    for (int j = 0; j < 2; j++)
    {
      // Dummy personality ID (doesn't matter)
      LCG(seed);
      LCG(seed);
      // HP, ATK, DEF IV
      LCG(seed);
      // SPEED, SPATK, SPDEF IV
      LCG(seed);
      // Ability
      LCG(seed);
      complexPID(seed, hTrainerId, lTrainerId, 0, WantedShininess::notShiny,-1,0,-1);
      generateEVs(seed, true, true);
    }
  }
  return seed;
}
int standardPathFind(int rem, std::vector<int>&instructions, std::vector<u32> &rerollSeeds){
  while (rem > 0){
    if (rem % 2 == 0){
      if (rem >= 2018){
        instructions[memcard] = 2 * (rem / 2018);
      }
      if (rem >= 40){
        instructions[rumble] = rem / 40;
      }
      rem = rem % 40;
      if (rem > 1){
        instructions[name] = rem / 2;
      } else if (rem == 1 || rem < 0) {
        std::cout << "ERROR: REM IS: " << rem << std::endl;
      }
      return rem;
    } else {
      if (rem >= 1009){
        instructions[memcard]++;
        //applying multiple 1009's on an odd number would cancel out and make it odd again.
        rem -= 1009;
      } else {
        instructions[reroll]--;
        rem += findGap(rerollSeeds.back(),rerollSeeds.at(rerollSeeds.size()-2),0);
        rerollSeeds.pop_back();
      }
    }
  }
  return rem;
  //Algorithm optimizes for rerolls, not for IRL time.
  //Algorithm begins with maximum rerolls done.
}
int completePath(int rem, int instructions[4]){
  //assumes conditions are met.
  if (rem >= 40){
    instructions[rumble] = rem / 40;
  }
  rem = rem % 40;
  if (rem > 1){
    instructions[name] = rem / 2;
  } else if (rem == 1 || rem < 0){
    std::cout << "ERROR: REM IS: " << rem << std::endl;
  }
  return rem; //should equal 0;
}
void popReroll(int &rem, int instructions[4],std::vector<u32>&rerollSeeds){
      instructions[memcard] = 0;
      instructions[rumble] = 0;
      instructions[name] = 0;

      instructions[reroll]--;
      rem += findGap(rerollSeeds.back(),rerollSeeds.at(rerollSeeds.size()-2),0);
      rerollSeeds.pop_back();
}
int evenPathFind(int rem, int instructions[4],std::vector<u32>&rerollSeeds){
 while(rem > 0){
   if (rem % 2 == 1){
    if (rem >= 1009){
      instructions[memcard]++;
      rem-= 1009; //congrats, now even no matter what. Stop worrying.
    } else {
      popReroll(rem,instructions,rerollSeeds);
      continue;
      //why not account for <1009 and odd? -- already did that when determining if runnable.
    }
  }
  if (rem >= 2018){
    instructions[memcard] += 2 * (rem / 2018);
    rem = rem % 2018; //cuts down on copious amounts of rumbles -- typically due to earlier reroll popping.
  }
  while(rem / 40 % 2 == 1 && instructions[memcard] > 1){
      instructions[memcard] -= 2;
      rem+=2018;
    } // should remain even from before
  if (rem / 40 % 2 == 0){
    return completePath(rem,instructions); //nice!
  } else {
    if (instructions[reroll] > 0){
      popReroll(rem,instructions,rerollSeeds);
    } else {
      rem = completePath(rem,instructions); //the worst case
      instructions[rumble]--;
      instructions[name]+= 20; //T_T I'm so sorry I hope it never comes to this.
    }
  }
 }
   /*
  EQUATION:
  if odd, + 1009, else ignore.
  B*2018 + (A / 40 % 2 == 0)
  */
 return rem;
 //deprecated, even is not necessary to force.
}
std::vector<u32> autoRollN(u32&seed,int n,std::vector<int>m_criteria){
  //Reroll loop!
  std::vector<u32>rerollSeeds;
  for (int i = 0; i < n; i++)
  {
    mGenerateBattleTeam(seed,m_criteria,m_criteria);
    //std::cout << getLastObtainedCriteriasString() << "\n";
    rerollSeeds.push_back(seed);
    //std::cout << std::hex << "Seed: "<< seed << std::dec << "\n\n";
    m_criteria = {-1, -1, -1, -1, -1, -1}; //refresh criteria slots.
  }
  return rerollSeeds;
}
void readReqsConfig(PokemonRequirements &inputReqs, std::ifstream &config){
int inputInt = 0;
std::string versionRead;
config >> versionRead; //version check;
    for (int i = 0; i < 12; i++)
    {
      switch (i)
      {
      case 0: //IVs
        config >> inputInt;
        inputReqs.hpIV = inputInt;
        break;
      case 1:
        config >> inputInt;
        inputReqs.atkIV = inputInt;
        break;
      case 2:
        config >> inputInt;
        inputReqs.defIV = inputInt;
        break;
      case 3:
        config >> inputInt;
        inputReqs.spAtkIV = inputInt;
        break;
      case 4:
        config >> inputInt;
        inputReqs.spDefIV = inputInt;
        break;
      case 5:
        config >> inputInt;
        inputReqs.speedIV = inputInt;
        break;
      case 6: //nature
        {
          config >> inputInt; //tells how many natures are valid
          if (inputInt == 25){
            //Deprecated, unless I want to add an all natures button
            //handles 'any'
            inputReqs.validNatures.fill(true);
          } else {
            int n = inputInt;
            for (int i = 0; i < n; i++)
            {
              config >> inputInt;
              inputReqs.validNatures[inputInt] = true;
            }
          }
          break;
        }
      case 7: //HPType
        {
          config >> inputInt;
          if (inputInt == 16){
            //handles 'any'
            inputReqs.validHPTypes.fill(true);
          } else {
            int n = inputInt;
            for (int i = 0; i < n; i++)
            {
              config >> inputInt;
              inputReqs.validHPTypes[inputInt] = true;
            }
          }
          break;
        }
      case 8: //power
        config >> inputInt;
        inputReqs.hiddenPowerPower = inputInt;
        break;
      case 9: //gender
        config >> inputInt;
        inputReqs.genderIndex = inputInt;
        break;
      case 10: //shiny
        config >> inputInt;
        inputReqs.isShiny = inputInt;
        break;
      default:
          //backup for incomplete config file?
        break;
      }
    }
}
PokemonRequirements setPokeReqs(){
    const std::string CONFIG_FILENAME = "config.txt";
    PokemonRequirements inputReqs;
    inputReqs.validNatures.fill(0);
    inputReqs.validHPTypes.fill(0);
    std::string readVersion = "";
    std::ifstream configR(CONFIG_FILENAME); //At this point, everything should be vetted. This is processing not validation.
    readReqsConfig(inputReqs,configR);
    configR.close();
    return inputReqs;

}

battleNowTeamInfo getBattleNowResultString(std::vector<int>m_criteria)
{
  std::string criteriasString;
  battleNowTeamInfo teamValues;
  switch (m_criteria[0])
  {
  case Mewtwo:
    teamValues.playerLead = "Mewtwo";
    teamValues.playerOther = "Entei";
    break;
  case Mew:
    teamValues.playerLead = "Mew";
    teamValues.playerOther = "Raikou";
    break;
  case Deoxys:
    teamValues.playerLead = "Deoxys";
    teamValues.playerOther = "Blissey";
    break;
  case Rayquaza:
    teamValues.playerLead = "Rayquaza";
    teamValues.playerOther = "Claydol";
    break;
  case Jirachi:
    teamValues.playerLead = "Jirachi";
    teamValues.playerOther = "Suicune";
    break;
  }
  switch (m_criteria[1])
  {
  case Articuno:
    teamValues.oppLead = "Articuno";
    teamValues.oppOther = "Swampert";
    break;
  case Zapdos:
    teamValues.oppLead = "Zapdos";
    teamValues.oppOther = "Blaziken";
    break;
  case Moltres:
    teamValues.oppLead = "Moltres";
    teamValues.oppOther = "Sceptile";
    break;
  case Kangaskhan:
    teamValues.oppLead = "Kangaskhan";
    teamValues.oppOther = "Latios";
    break;
  case Latias:
    teamValues.oppLead = "Latias";
    teamValues.oppOther = "Gengar";
    break;
  }

  teamValues.playerLeadHp = m_criteria[2];
  teamValues.playerOtherHp = m_criteria[3];
  teamValues.oppLeadHp = m_criteria[4];
  teamValues.oppOtherHp = m_criteria[5];

  return teamValues;
}
battleNowTeamInfo getBattleTeamInfo(u32 seed){
  std::vector<int> m_criteria = {-1, -1, -1, -1, -1, -1};
  m_criteria = mGenerateBattleTeam(seed,m_criteria,m_criteria);
  return getBattleNowResultString(m_criteria);
}

void printPokeInfo(PokemonProperties eevee, u32 listingSeed, int callsToTarget){
    std::cout << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n";
    std::cout << "First runnable Eevee found at: " << std::hex << listingSeed << std::dec<< " which is " << callsToTarget << " calls away\n";
    std::cout << "TID: " << eevee.trainerId
    <<  "\n" << eevee.hpIV
    << " / " << eevee.atkIV
    << " / " << eevee.defIV
    << " / " << eevee.spAtkIV
    << " / " << eevee.spDefIV
    << " / " << eevee.speedIV
    << " \nHidden Power: " << hpTypes[eevee.hiddenPowerTypeIndex]
    << ": " << eevee.hiddenPowerPower
    << "\nNature: " << naturesList[eevee.natureIndex];
    std::cout<<"\nGender: ";
    if (eevee.genderIndex){
        std::cout << "Female";
    } else {
        std::cout << "Male";
    }
    std::cout << "\nShiny: ";
    if (eevee.isShiny){
      std::cout << "True";
    } else {
      std::cout << "False";
    }
    std::cout << "\n";
    std::cout << "\n+ + + + + + + + + + + + \n\n";
}
void printInstructions(int instructions[4]){
    // std::cout << "Instruction set: " << instructions[3] << "/" << instructions[0]
    // << "/" << instructions[1] << "/" << instructions[2] << std::endl;
    std::cout << "To reach target, peform: \n"
    << instructions[3] << " reroll(s),\n"
    << instructions[0] << " memory card reload(s),\n"
    << instructions[1] << " rumble switch(es),\n"
    << instructions[2] << " naming screen back-out(s).\n";

    // debugSeed = userInputRerollSeed;
    // std::cout << std::hex << LCGn(debugSeed,memcardValue*instructions[0]);
    // std::cout << "\n" << LCGn(debugSeed,rumbleValue*instructions[1]);
    // std::cout << "\n" << LCGn(debugSeed,namingValue*instructions[2]);
}

//int main(){
//  //an "Oops I got lost" - feature might be a nice addition to the main tool,
//  //which can find ur seed based on 1 or at most 2 rolls of input again.
//  //Also a forward and backwards tabber to move through the rolls (with the team members)
//  //unlike with seed alone like it is now.

//  //BUG FIX: If you've just set or updated the config file, then it isn't saved unless the "Exit" command is used.
//    //Fix: Retyping version as a string, as this allows not only full precision (was dropping the 0) but also allows the use of letters or symbols
//    //in the version name, if so desired.
//  //BUG FIX: Restore sometimes not working or skipping a seed.
//  //BUG FIX: "Seed on Reroll" is sometimes wrong -- 99ce3998 -> supposedly 93549ca5 but actually --> 27B73530. Process still works.
//  //FEATURE CHANGE: Consider removing force even, as setting the rumble back to off then hitting "No" on the save prompt will keep the setting
//  //for the run. without turning back on. However will revert on soft/hard reset, such as in the case of a S+Q.
//  //FEATURE ADD: Ability to hunt for specific titleSeed, as this is what Shiny Hunters care about. Hunting a shiny eevee is already done.

//  // const int namingValue = 2;
//  // const int rumbleValue = 40; //note in colo this is 20 calls instead.
//  // const int memcardValue = 1009;

//  //STATIC BUILD COMMAND: DON'T LOSE THIS: g++ -Wall -o PAL-Pal PAL-Pal.cpp -static
//  const std::string BUILD_VERSION = "0.20";

//  std::vector<int> m_criteria = {-1, -1, -1, -1, -1, -1};
//  std::vector<u32> previousResults;
//  u32 userInputRerollSeed = 0x0;
//  u32 seed = 0x0;
//  u32 listingSeed = 0x0;
//  u32 titleSeed = 0x0;
//  int instructions[4] = {0,0,0,0};
//  const int eeveeGenderRatio = 0x1F;
//  std::string commands[255] = {"Reject","Restore","Reset","Settings","Exit"};
//  std::string currentCommand = "";
//  PokemonProperties eevee;
//  PokemonRequirements requirements;
//  requirements.validHPTypes.fill(false);
//  requirements.validNatures.fill(false);
//  bool firstRun = true;
//  const int minimumCTT = 1002;

//  //initialize
//  printLogo();
//  userInputRerollSeed = getInputSeed();
//  seed = userInputRerollSeed;
//  listingSeed = seed;
//  requirements = setPokeReqs(BUILD_VERSION);
//  bool searchActive = true;
//  while(searchActive){
//    //search for runnable eevee.
//    // int eeveesSearched = 0;
//    if (firstRun){
//      LCGn(seed,minimumCTT);
//      firstRun = false;
//    }
//    bool cttPass = false;
//    int eeveeSearched = 0;

//    while(!(foundRunnable(eevee,requirements) && cttPass)){
//        listingSeed = LCG(seed); //seed incremented and stored
//        eeveeSearched++;
//        condensedGenerateMon(eevee,seed,eeveeGenderRatio);
//        if (foundRunnable(eevee,requirements)){
//          cttPass = CTTAcceptable(seed,seekCallsToTargetEevee(userInputRerollSeed,listingSeed));
//        }
//        seed = listingSeed; //seed restored
//    }

//  //WHEN CONFIRMED VALID:
//  LCGn_BACK(seed,1000 + 2); // generation minimum
//  titleSeed = seed;
//  //find *primary* calls to target.
//  int callsToTarget = seekCallsToTargetEevee(userInputRerollSeed,listingSeed);
//  printPokeInfo(eevee,listingSeed,callsToTarget);
//  std::cout << "Target (seed at title screen): " << std::hex << titleSeed << std::dec << std::endl;
//  std::cout << "If you want to see this Espeon and Teddy in the main program,\n then enter the above seed in the 'Enter seed manually' box.\n";
//  std::cout << "\n+ + + + + + + + + + + + \n\n";

//  //~~~~~~~~~~~~~~~~~~Target seeking is complete!~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//  //~~~~~~~~~~~~~~~~~~~Now find a path to target.~~~~~~~~~~~~~~~~~~~~~~~~~~~~

//  seed = userInputRerollSeed;
//  std::vector<u32> rerollSeeds = {userInputRerollSeed};
//  u32 testSeed = userInputRerollSeed;
//  instructions[3] = testRerolls(testSeed,callsToTarget,rerollSeeds,m_criteria);
//  seed = rerollSeeds.back();


//  int rem = callsToTarget;
//  //Default search, no forceEven consideration.
//  if (requirements.forceEven){
//    rem = evenPathFind(rem,instructions,rerollSeeds);
//  } else {
//    rem = standardPathFind(rem,instructions,rerollSeeds);
//  }



//  //2nd Printing block
//  rerollSeeds.pop_back();
//  seed = rerollSeeds.back();
//  std::cout << "At reroll #" << instructions[3] << " the seed is: "<<std::hex << seed << std::dec <<" and the team generated will be: \n"
//  << getBattleTeamInfo(seed)
//  << "\n\n+ + + + + + + + + + + + \n\n";
//  printInstructions(instructions);
//  std::cout << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

//  //WRAP UP BLOCK
//  eevee = {};
//  instructions[0] = 0;
//  instructions[1] = 0;
//  instructions[2] = 0;
//  instructions[3] = 0;
//  std::cout <<"\nThe current result has been saved.\nEnter a command to continue:\nCommand: ";
//  getline(std::cin,currentCommand);
//  handleEmptyString(currentCommand);
//  formatCase(currentCommand,lower);
//  currentCommand.at(0) = toupper(currentCommand.at(0));
//  //std::cout <<"RESULTS STORED: " << previousResults.size() << "\n";
//  bool validCommand = false;
//  while (!validCommand)
//  {
//    if (currentCommand == commands[3]){
//      //Settings
//      validCommand = true;
//      requirements = {};
//      writeReqsConfig(requirements,BUILD_VERSION);
//    } else if (currentCommand == commands[0]){
//      //Reject
//      validCommand = true;
//      searchActive = true;
//      seed = listingSeed;
//      previousResults.push_back(listingSeed);
//    } else if (currentCommand == commands[1]){
//      //Restore
//      validCommand = true;
//      searchActive = true;
//      for (unsigned int i = 0; i < previousResults.size(); i++)
//      {
//        std::cout << std::hex << previousResults.at(i) << std::endl;
//      }
//      if (previousResults.size() > 0){
//        previousResults.pop_back();
//      }
//      seed = LCG_BACK(previousResults.back());
//      if (previousResults.size() == 0){
//        seed = userInputRerollSeed;
//      }
//    } else if(currentCommand == commands[2]){
//      //Reset
//      validCommand = true;
//      firstRun = true;
//      userInputRerollSeed = getInputSeed();
//      seed = userInputRerollSeed;
//      listingSeed = seed;
//    } else if (currentCommand == commands[4]){
//      //Exit
//      validCommand = true;
//      searchActive = false;
//    } else {
//      std::cout << "Invalid command. Please try again: ";
//      getline(std::cin,currentCommand);
//      handleEmptyString(currentCommand);
//      formatCase(currentCommand,lower);
//      currentCommand.at(0) = toupper(currentCommand.at(0));

//    }
//  }
//  }
//  return 0;
//}

searchResult findCandidate (u32 inputSeed, bool firstRun){
    searchResult returnData;
    PokemonProperties eevee;
    const int eeveeGenderRatio = 0x1F;
    const int minimumCTT = 1002; //generation minimum
    u32 seed = inputSeed;
    u32 listingSeed = inputSeed; //used as backup
//    u32 titleSeed = 0; //Output
    PokemonRequirements requirements = setPokeReqs();
    if (requirements.spAtkIV == 0){
    std::cout << "FAIL TO READ";
    }
      //search for runnable eevee.
      if (firstRun){
        LCGn(seed,minimumCTT);
      }
      bool cttPass = false;
      int eeveeSearched = 0; //debug, still needed?

      while(!(foundRunnable(eevee,requirements) && cttPass)){
          listingSeed = LCG(seed); //seed incremented and stored
          eeveeSearched++;
          condensedGenerateMon(eevee,seed,eeveeGenderRatio);
          if (foundRunnable(eevee,requirements)){
            cttPass = CTTAcceptable(seed,seekCallsToTargetEevee(inputSeed,listingSeed));
          }
          seed = listingSeed; //seed restored
      }

      returnData.candidate = eevee;
      returnData.titleSeed = LCGn_BACK(seed,minimumCTT);
      returnData.listingSeed = listingSeed; //should this just be candidateSeeds.back()?
      //returnData.candidateSeeds.push_back(listingSeed);
      return returnData; //Found eevee at seed.
}

searchResult searchForEevee (u32 inputSeed, bool firstRun){
    u32 seed = inputSeed;
    std::vector<u32>rerollSeeds = {inputSeed}; //bank to work off of after max rerolls found
    std::vector<int>m_criteria = {-1,-1,-1,-1,-1,-1}; //format for battleNowInfo

    searchResult returnData = findCandidate(seed,firstRun); //creates a new eevee each run?

    //pathfinding
    int callsToTarget = findGap(inputSeed,returnData.titleSeed,1); //don't need to show to user unless debugging?
    returnData.instructions[reroll] = testRerolls(seed,callsToTarget,rerollSeeds,m_criteria);
    standardPathFind(callsToTarget,returnData.instructions,rerollSeeds);//Default search, no forceEven consideration.
    //battlenowTeaminfo stuff
    //rerollSeeds.pop_back();
    seed = rerollSeeds.at(rerollSeeds.size()-2);
    returnData.rerollSeed = rerollSeeds.back();
    returnData.battleNow = getBattleTeamInfo(seed);
    return returnData; //Found eevee at seed.
}

