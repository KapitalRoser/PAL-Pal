#include "../processCore.h"

struct Stats
  {
    int hp;
    int atk;
    int def;
    int spAtk;
    int spDef;
    int speed;
  };
enum class WantedShininess
  {
    notShiny,
    shiny,
    any
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
u32 generatePokemonPID(u32& seed, const u32 hTrainerId,
                                                     const u32 lTrainerId, const u32 dummyId,
                                                     const WantedShininess shininess,
                                                     const s8 wantedGender, const u32 genderRatio,
                                                     const s8 wantedNature)
{
  u32 pid = 0;
  bool goodId = false;
  int nbrPidAttemptLeft = 3145728;
  while (nbrPidAttemptLeft > 0)
  {
    // A personality ID is generated as candidate, high then low 16 bits
    u32 hId = LCG(seed) >> 16;
    u32 lId = LCG(seed) >> 16;
    pid = (hId << 16) | (lId);
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

std::array<u8, 6> generateEVs(u32& seed, const bool allowUnfilledEV,
                                                            const bool endPrematurely)
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
u32 rollRNGToBattleMenu(u32 seed)
{
  // Before getting to the quick battle menu, the reason this is deterministic is because the only
  // thing that is consequential is a dummy pokemon generation, but there's no criteria on the pid
  // meaning the first pid generated will always be the one used
  seed = LCGn(seed, 1139);
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
      generatePokemonPID(seed, hTrainerId, lTrainerId, 0, WantedShininess::notShiny,-1,0,-1);
      generateEVs(seed, true, true);
    }
  }
  return seed;
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
      generatePokemonPID(seed, hTrainerId, lTrainerId, 0, WantedShininess::notShiny,-1,0,-1);
      generateEVs(seed, true, true);
    }
  }
  return seed;
}

bool generateBattleTeam(u32 &seed, const std::vector<int> criteria,std::vector<int>&m_criteria)
{
  // Player trainer name generation
  LCG(seed);
  // Player team index
  int playerTeamIndex = (LCG(seed) >> 16) % 5;
  if (playerTeamIndex != criteria[0] && criteria[0] != -1)
    return false;
  // Enemy team index
  int enemyTeamIndex = (LCG(seed) >> 16) % 5;
  if (enemyTeamIndex != criteria[1] && criteria[1] != -1)
    return false;
  // modulo 6 ???
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
    generatePokemonPID(seed, hTrainerId, lTrainerId, 0, WantedShininess::notShiny,-1,0,-1);
    std::array<u8, 6> EVs = generateEVs(seed, false, false);
    int hp = EVs[0] / 4 + hpIV + s_quickBattleTeamMaxBaseHPStat[enemyTeamIndex + 5][i];
    if (hp != criteria[4 + i] && criteria[4 + i] != -1)
      return false;
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
    generatePokemonPID(seed, hTrainerId, lTrainerId, 0, WantedShininess::notShiny,-1,0,-1);
    std::array<u8, 6> EVs = generateEVs(seed, false, false);
    int hp = EVs[0] / 4 + hpIV + s_quickBattleTeamMaxBaseHPStat[playerTeamIndex][i];
    if (hp != criteria[2 + i] && criteria[2 + i] != -1)
      return false;
    obtainedPlayerHP.push_back(hp);
  }
    m_criteria.clear();
    m_criteria.push_back(playerTeamIndex);
    m_criteria.push_back(enemyTeamIndex);
    m_criteria.insert(m_criteria.end(), obtainedPlayerHP.begin(),
                                   obtainedPlayerHP.end());
    m_criteria.insert(m_criteria.end(), obtainedEnemyHP.begin(),
                                   obtainedEnemyHP.end());
  return true;
}
std::vector<int> obtainTeamGenerationCritera(u32& seed)
{
  std::vector<int> criteria;
  // Player trainer name generation
  LCG(seed);
  // Player team index
  criteria.push_back((LCG(seed) >> 16) % 5);
  // Enemy team index
  criteria.push_back((LCG(seed) >> 16) % 5);
  // modulo 6 ???
  LCG(seed);

  // Enemy gen

  // The player trainer ID is generated, low then high 16 bits
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
    generatePokemonPID(seed, hTrainerId, lTrainerId, 0, WantedShininess::notShiny,-1,0,-1);
    generateEVs(seed, false, false);
  }

  // modulo 6 ???
  LCG(seed);

  // Player gen

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
    generatePokemonPID(seed, hTrainerId, lTrainerId, 0, WantedShininess::notShiny,-1,0,-1);
    generateEVs(seed, false, false);
  }
  return criteria;
}
std::string getLastObtainedCriteriasString(std::vector<int>m_criteria)
{
  std::string criteriasString = "Player team leader: ";
  switch (m_criteria[0])
  {
  case Mewtwo:
    criteriasString += "MEWTWO";
    break;
  case Mew:
    criteriasString += "MEW";
    break;
  case Deoxys:
    criteriasString += "DEOXYS";
    break;
  case Rayquaza:
    criteriasString += "RAYQUAZA";
    break;
  case Jirachi:
    criteriasString += "JIRACHI";
    break;
  }

  criteriasString += "\nComputer team leader: ";
  switch (m_criteria[1])
  {
  case Articuno:
    criteriasString += "ARTICUNO";
    break;
  case Zapdos:
    criteriasString += "ZAPDOS";
    break;
  case Moltres:
    criteriasString += "MOLTRES";
    break;
  case Kangaskhan:
    criteriasString += "KANGASKHAN";
    break;
  case Latias:
    criteriasString += "LATIAS";
    break;
  }

  criteriasString += "\nPlayer Pokemon's HP: " + std::to_string(m_criteria[2]) + " " +
                     std::to_string(m_criteria[3]) + "\n";
  criteriasString += "Computer Pokemon's HP: " + std::to_string(m_criteria[4]) + " " +
                     std::to_string(m_criteria[5]);

  return criteriasString;
}

std::vector<u32> autoRollN(u32&seed,int n,std::vector<int>m_criteria){
  //Reroll loop!
  std::vector<u32>rerollSeeds;
  for (int i = 0; i < n; i++)
  { 
    generateBattleTeam(seed,m_criteria,m_criteria);
    //std::cout << getLastObtainedCriteriasString() << "\n";
    rerollSeeds.push_back(seed);
    //std::cout << std::hex << "Seed: "<< seed << std::dec << "\n\n";
    m_criteria = {-1, -1, -1, -1, -1, -1};
  }
  return rerollSeeds;
}

std::string getBattleTeamInfo(u32 seed){
  std::vector<int> m_criteria = {-1, -1, -1, -1, -1, -1};
  generateBattleTeam(seed,m_criteria,m_criteria);
  return getLastObtainedCriteriasString(m_criteria);
}

int main(){
  std::vector<int> m_criteria = {-1, -1, -1, -1, -1, -1};
  // std::vector<int> dummy_criteria = {-1, -1, -1, -1, -1, -1}; //ill be honest why are these even here?
  u32 initialSeed = 0x652ECBBD;
  u32 postMenuSeed = 0x0;
  //for some reason seed == 0 doesn't work from title screen? should be A0281F1C.
  int rollsToRun = 10;
  u32 seed = initialSeed;
  bool fromBoot = false;
  bool validMemcard = true;
  std::vector<u32> rerollSeeds;
  std::vector<int>rollsRaw;
  std::vector<int>rollsSorted;

  postMenuSeed = myRollRNGToBattleMenu(seed,fromBoot,validMemcard);
  std::cout << "Seed after battle menu calls: " << std::hex << seed << "\n\n" << std::dec;
  

rerollSeeds = autoRollN(seed,rollsToRun,m_criteria);

std::cout << "Generated team: " << getBattleTeamInfo(seed);
//distance finder:
seed = postMenuSeed;
for (unsigned int i = 0; i < rerollSeeds.size(); i++)
{
  rollsRaw.push_back(findGap(seed,rerollSeeds.at(i),true));
}


//debugPrintVec(rollsRaw);

//STATS:
rollsSorted = rollsRaw;
std::sort(rollsSorted.begin(),rollsSorted.end());
int max = rollsSorted.back();
int min = rollsSorted.front();
u32 minSeed = 0;
u32 maxSeed = 0;
float mean = 0;
int prev = max;
int mode;
int maxcount = 0;
int currcount = 0;
for (const auto n : rollsSorted) {
    if (n == prev) {
        ++currcount;
        if (currcount > maxcount) {
            maxcount = currcount;
            mode = n;
        }
    } else {
        currcount = 1;
    }
    prev = n;
}
for (unsigned int i = 0; i < rollsSorted.size(); i++)
{
  mean += rollsSorted.at(i);
}
mean = mean / rollsSorted.size();

for (unsigned int i = 1; i < rollsRaw.size(); i++)
{
  if (rollsRaw.at(i) == min){
    minSeed = rerollSeeds.at(i-1);
  }
  if (rollsRaw.at(i) == max){
    maxSeed = rerollSeeds.at(i-1);
  }
}



std::cout << "Final Seed: " <<std::hex << rerollSeeds.back() << std::dec
<< "\nMax calls: " << max << " at " << std::hex << maxSeed << std::dec << "\nMin calls: " << min << " at " << std::hex<< minSeed << std::dec 
<< "\nMean: " << mean << "\nMode: " << mode << " ("<< maxcount << ")"; 
//max always appears to be 2269 and min appears to be 73
//601 standard mode and 732.xxx is the mean.














    return 0;
}

/*
2 calls followed by X


*/