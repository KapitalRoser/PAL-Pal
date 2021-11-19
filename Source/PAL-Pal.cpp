#include "processCore.h"
const std::string hpTypes[16] = {"Fighting", "Flying", "Poison", "Ground", "Rock", "Bug", "Ghost", 
    "Steel", "Fire", "Water", "Grass", "Electric",  "Psychic", "Ice","Dragon","Dark"};
enum hpTypeID {Fighting, Flying, Poison, Ground, Rock, Bug, 
  Ghost, Steel, Fire, Water, Grass, Electric, Psychic, Ice, 
  Dragon, Dark}; //could remake this into a map lol.

const std::string naturesList[25] = {"Hardy","Lonely","Brave","Adamant","Naughty","Bold","Docile","Relaxed",
"Impish","Lax","Timid","Hasty","Serious","Jolly","Naive","Modest","Mild","Quiet","Bashful",
"Rash","Calm","Gentle","Sassy","Careful","Quirky"};
enum natureID {Hardy,Lonely,Brave,Adamant,Naughty,Bold,Docile,
Relaxed,Impish,Lax,Timid,Hasty,Serious,Jolly,Naive,Modest,Mild,
Quiet,Bashful,Rash,Calm,Gentle,Sassy,Careful,Quirky};

enum instr {memcard,rumble,name,reroll};

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
int standardPathFind(int rem, int instructions[4], std::vector<u32> &rerollSeeds){
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
bool verifyu32(std::string formatted){
  //std::cout << "String to verify:" << formatted << std::endl;
  for (unsigned int i = 0; i < formatted.length(); i++)
  {
    if (formatted.at(i) < '0' || (formatted.at(i) > '9' && formatted.at(i) < 'A') || formatted.at(i) > 'F'){
      std::cout << "ERROR, invalid character detected at: " << formatted.at(i) << ", please try again.";
      
      return false;
    }
  }
  return true;
}
u32 getInputSeed(){
  u32 userSeed;
  std::string userInput = "";
  std::stringstream hexConvert;
  bool validInput = false;
  while(!validInput){
    std::cout << "\nEnter the seed produced by the seed finder: ";
    getline(std::cin,userInput);
    for (unsigned int i = 0; i < userInput.length(); i++){
      userInput.at(i) = toupper(userInput.at(i));
    }
    validInput = verifyu32(userInput);
    std::cout << std::endl;
  }
  hexConvert << std::hex << userInput;
  hexConvert >> userSeed;
  hexConvert.clear();
  hexConvert.str("");
  return userSeed;
}
void readReqsConfig(PokemonRequirements &inputReqs, std::ifstream &config){
int inputInt = 0;
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
          int n = inputInt;
          for (int i = 0; i < n; i++)
          {
            config >> inputInt;
            inputReqs.validNatures[inputInt] = true;
          }
          break;
        }
      case 7: //HPType
        {
          config >> inputInt;
          int n = inputInt;
          for (int i = 0; i < n; i++)
          {
            config >> inputInt;
            inputReqs.validHPTypes[inputInt] = true;
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
      case 11: //Force rumble
        config >> inputInt; //bool
        inputReqs.forceEven = inputInt;
        break;
      default:
        break;
      }
    }
}
void writeReqsConfig(PokemonRequirements&inputReqs){
    std::ofstream configW("palpalConfig.txt");
    //time to write the file!
    std::vector<std::string> strReqs = {"HP","ATK","DEF","SPA","SPD","SPE"};
    std::vector<int>IVreqs;
    unsigned int IV = 0;
    //do I really need to cleanse non-numeric inputs...
    std::string Nature;
    std::string IVInput = "";
    //save these to a config file for later access?
    std::cout << "\n~~~~~~~~~~~SETUP REQUIREMENTS~~~~~~~~~~~\n\n";
    //first, IVS
    for (int i = 0; i < 6; i++)
    //Add support for Any and Done commands
    {
      std::cout << "Enter minimum " << strReqs.at(i) << " IV: ";
      std::cin >> IV;
      while (IV < 0 || IV > 31 || std::cin.fail()){
        std::cin.clear();
        std::cout << "Invalid input, please try again: ";
        std::cin >> IV;
      }
      switch (i)
      {
      case 0:
        inputReqs.hpIV = IV;
        break;
      case 1:
        inputReqs.atkIV = IV;
        break;
      case 2:
        inputReqs.defIV = IV;
        break;
      case 3:
        inputReqs.spAtkIV= IV;
        break;
      case 4:
        inputReqs.spDefIV = IV;
        break;
      case 5:
        inputReqs.speedIV = IV;
        break;
      default:
        break;
      }
    }
    std::cin.get();
    //next Nature
    std::cout << "IVs set successfully!\n";
    std::cout << inputReqs.hpIV << "/"
    << inputReqs.atkIV << "/"
    << inputReqs.defIV << "/"
    << inputReqs.spAtkIV << "/"
    << inputReqs.spDefIV << "/"
    << inputReqs.speedIV << "\n";
    
    
    std::cout << "Enter acceptable nature(s): \n";
  
    bool allNaturesEntered = false;
    std::string inputNature = "";
    std::vector<int> natureIDXs;
    std::cout << "type the name of a nature, or enter 'any' to accept all natures: ";
    while (!allNaturesEntered){
      getline(std::cin,inputNature);
      formatCase(inputNature,lower);
      inputNature.at(0) = toupper(inputNature.at(0)); //capitalize
      std::cout << "Nature entered: " << inputNature << std::endl;

      //exit statements:
      if (inputNature == "Done"){
        allNaturesEntered = true;
      } else if (inputNature == "Any"){
        for (int i = 0; i < 25; i++)
        {
          inputReqs.validNatures[i] = true;
          allNaturesEntered = true;
        }
      } else {
        bool foundAny = false;
        for (int i = 0; i < 25; i++)
        {
          if (inputNature == naturesList[i]){
            foundAny = true;
            if (inputReqs.validNatures[i] == false){
              inputReqs.validNatures[i] = true;
              natureIDXs.push_back(i);
              std::cout <<"Added "<< naturesList[i] <<"!\n";
            } else {
              std::cout << "You have already added " << inputNature << ".\n";
            }
          }
        }
        if (!foundAny){
          std::cout << "Nature not found - invalid input.\n";
        }
        std::cout << "Enter more natures or use commands 'Any' or 'Done': ";
        int full = 0;
        for (int i = 0; i < 25; i++)
        {
          if (inputReqs.validNatures[i] == true){
            full++;
          }
        }
        if (full == 25){
          std::cout << "You really took the time to manually enter every nature...wow." 
          <<"fine! all natures are enabled, moving on.";
          allNaturesEntered = true;
        }
      }
    }
    //next Hidden Power Type and strength
    //one day implement a way to filter for seperate HP strengths for different HPs. 
    //i.e 68 for psychic and 65 for electric or something.
    //would need to restructure pokereqs a bit.
    std::cout << "Natures entered successfully!\n\n";
    std::cout << "Enter acceptable Hidden Power(s): \n";
    bool allHPTypesEntered = false;
    std::string inputHPT = "";
    std::vector<int> HPTIDXs;
    std::cout << "Enter the name of a type, or enter 'any' to accept all types: ";
    while (!allHPTypesEntered){
      getline(std::cin,inputHPT);
      formatCase(inputHPT,lower);
      inputHPT.at(0) = toupper(inputHPT.at(0)); //capitalize
      std::cout << "HPT entered: " << inputHPT << std::endl;

      //exit statements:
      if (inputHPT == "Done"){
        allHPTypesEntered = true;
      } else if (inputHPT == "Any"){
        for (int i = 0; i < 25; i++)
        {
          inputReqs.validHPTypes[i] = true;
          allHPTypesEntered = true;
        }
      } else {
        bool foundAny = false;
        for (int i = 0; i < 25; i++)
        {
          if (inputHPT == hpTypes[i]){
            foundAny = true;
            if(inputReqs.validHPTypes[i] == false){
              std::cout <<"Added "<< hpTypes[i] <<"!\n";
              inputReqs.validHPTypes[i] = true;
              HPTIDXs.push_back(i);
            } else {
              std::cout << "You have already added " << hpTypes[i] << ".\n";
            }
          }
        }
        if (!foundAny){
          std::cout << "Hidden Power Type not found - invalid input.\n";
        }
        std::cout << "Enter more types or use commands 'Any' or 'Done': ";
        int full = 0;
        for (int i = 0; i < 16; i++)
        {
          if (inputReqs.validHPTypes[i] == true){
            full++;
          }
        }
        if (full == 16){
          std::cout << "You really took the time to manually enter every Hidden Power type...wow." 
          <<"fine! all HPs are enabled, moving on.";
          allHPTypesEntered = true;
        }
      }
    }
  //Now to select power
    int inputPower = 0;
    std::cout << "\nEnter minimum strength of Hidden Power: ";
    std::cin >> inputPower;
    while(inputPower < 0 || inputPower > 70){
      std::cout << "\nInvalid input, please try again: ";
      std::cin >> inputPower;
    }
    inputReqs.hiddenPowerPower = inputPower;
    std::cout << "\nHPP entered: " << inputReqs.hiddenPowerPower << "\n";
    std::cin.get();
    //next gender
    std::string inputGender = "";
    std::cout << "Want to choose a specific gender? Enter 'Male' or 'M', 'Female' or 'F', or 'Any': ";
    getline(std::cin,inputGender);
    formatCase(inputGender,lower);
    inputGender.at(0) = toupper(inputGender.at(0));
    if (inputGender == "Male" || inputGender == "M"){
      inputReqs.genderIndex = 0;
    } else if (inputGender == "Female" || inputGender == "F"){
      inputReqs.genderIndex = 1;
    } else {
      inputReqs.genderIndex = 2;
    }
    std::cout << "gender entered succesfully!\n";
    //finally shinystatus
    std::string inputShiny = "";
    std::cout << "Want the target to be shiny? Enter Yes, No, or Any:";
    getline(std::cin,inputShiny);
    formatCase(inputShiny,lower);
    inputShiny.at(0) = toupper(inputShiny.at(0));
    if (inputShiny == "No" || inputShiny == "N"){
      inputReqs.isShiny = 0;
    } else if (inputShiny == "Yes" || inputShiny == "Y"){
      inputReqs.isShiny = 1;
    } else {
      inputReqs.isShiny = 2;
    }
    std::cout << "Shiny status entered succesfully!\n";
    //finally shinystatus
    std::string inputEven = "";
    std::cout << "Want to force the number of Rumble Switches to be even-numbered? Enter Yes or No:";
    getline(std::cin,inputEven);
    formatCase(inputEven,lower);
    inputEven.at(0) = toupper(inputEven.at(0));
    if (inputEven == "Yes" || inputEven == "Y"){
      inputReqs.forceEven = true;
    } else{
      inputReqs.forceEven = false;
    }
    std::cout << "All requirements recorded! Good luck!\n";

    //config filewrite
    configW 
    << inputReqs.hpIV    <<"\n"<< inputReqs.atkIV   <<"\n"
    << inputReqs.defIV   <<"\n"<< inputReqs.spAtkIV <<"\n"
    << inputReqs.spDefIV <<"\n"<< inputReqs.speedIV <<"\n"
    << natureIDXs.size()<<"\n";
    for (unsigned int i = 0; i < natureIDXs.size(); i++){
      configW << natureIDXs.at(i) << "\n";
    }
    configW << HPTIDXs.size()<<"\n";
    for (unsigned int i = 0; i < HPTIDXs.size(); i++){
      configW << HPTIDXs.at(i) << "\n";
    }
    configW << inputReqs.hiddenPowerPower <<"\n"
    << inputReqs.genderIndex <<"\n"
    << inputReqs.isShiny<<"\n"
    << inputReqs.forceEven;
    configW.close();
}
PokemonRequirements setPokeReqs(){
  //intended for initial run
  //maybe make a version check, and store version inside config at the top, so that fresh installs don't need to redo their configs.
  PokemonRequirements inputReqs;
  inputReqs.validNatures.fill(0);
  inputReqs.validHPTypes.fill(0);
  //first open config file
  std::ifstream configR("palpalConfig.txt");
  if (configR.fail()){
    std::cout << "Creating new Config file!\n";
    configR.close();
    writeReqsConfig(inputReqs);
    return inputReqs;
  } else {
    std::cout << "Successfully read Config file!\n";
    readReqsConfig(inputReqs,configR);
    return inputReqs;
  }
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
std::string getBattleTeamInfo(u32 seed){
  std::vector<int> m_criteria = {-1, -1, -1, -1, -1, -1};
  m_criteria = mGenerateBattleTeam(seed,m_criteria,m_criteria);
  return getLastObtainedCriteriasString(m_criteria);
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
void printLogo(){
  std::cout << "\n* * * * * * * * * * * * * * * * * * * *\n";
  std::cout << R"( _____        _          _____      _ 
|  __ \ /\   | |        |  __ \    | |
| |__) /  \  | |  ______| |__) |_ _| |
|  ___/ /\ \ | | |______|  ___/ _` | |
| |  / ____ \| |____    | |  | (_| | |
|_| /_/    \_\______|   |_|   \__,_|_|)";
std::cout << "\n\n* * * * * * * * * * * * * * * * * * * *\n\n";
}
int main(){
  //an "Oops I got lost" - feature might be a nice addition to the main tool,
  //which can find ur seed based on 1 or at most 2 rolls of input again.
  //Also a forward and backwards tabber to move through the rolls (with the team members)
  //unlike with seed alone like it is now.

  // const int namingValue = 2;
  // const int rumbleValue = 40; //note in colo this is 20 calls instead.
  // const int memcardValue = 1009;

  //STATIC BUILD COMMAND: DON'T LOSE THIS: g++ -Wall -o PAL-Pal PAL-Pal.cpp -static

  std::vector<int> m_criteria = {-1, -1, -1, -1, -1, -1};
  std::vector<u32> previousResults;
  u32 userInputRerollSeed = 0x0;
  u32 seed = 0x0;
  u32 listingSeed = 0x0;
  u32 titleSeed = 0x0;
  int instructions[4] = {0,0,0,0};
  const int eeveeGenderRatio = 0x1F;
  std::string commands[255] = {"Reject","Restore","Reset","Settings","Exit"};
  std::string currentCommand = "";
  PokemonProperties eevee;
  PokemonRequirements requirements;
  requirements.validHPTypes.fill(false);
  requirements.validNatures.fill(false);
  bool firstRun = true;
  const int minimumCTT = 1002;

  //initialize
  printLogo();
  userInputRerollSeed = getInputSeed();
  seed = userInputRerollSeed;
  listingSeed = seed;
  requirements = setPokeReqs(); 
  bool searchActive = true;
  while(searchActive){
    //search for runnable eevee.
    // int eeveesSearched = 0;
    if (firstRun){
      LCGn(seed,minimumCTT);
      firstRun = false;
    }
    bool cttPass = false;
    int eeveeSearched = 0;

    while(!(foundRunnable(eevee,requirements) && cttPass)){
        listingSeed = LCG(seed); //seed incremented and stored
        eeveeSearched++;
        condensedGenerateMon(eevee,seed,eeveeGenderRatio);
        if (foundRunnable(eevee,requirements)){
          cttPass = CTTAcceptable(seed,seekCallsToTargetEevee(userInputRerollSeed,listingSeed));
        }
        seed = listingSeed; //seed restored
    }

  //WHEN CONFIRMED VALID: 
  LCGn_BACK(seed,1000 + 2); // generation minimum
  titleSeed = seed;
  //find *primary* calls to target.
  int callsToTarget = seekCallsToTargetEevee(userInputRerollSeed,listingSeed);
  printPokeInfo(eevee,listingSeed,callsToTarget);
  std::cout << "Target (seed at title screen): " << std::hex << titleSeed << std::dec << std::endl;
  std::cout << "If you want to see this Espeon and Teddy in the main program,\n then enter the above seed in the 'Enter seed manually' box.\n";
  std::cout << "\n+ + + + + + + + + + + + \n\n";

  //~~~~~~~~~~~~~~~~~~Target seeking is complete!~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  //~~~~~~~~~~~~~~~~~~~Now find a path to target.~~~~~~~~~~~~~~~~~~~~~~~~~~~~

  seed = userInputRerollSeed;
  std::vector<u32> rerollSeeds = {userInputRerollSeed};
  u32 testSeed = userInputRerollSeed;
  instructions[3] = testRerolls(testSeed,callsToTarget,rerollSeeds,m_criteria);
  seed = rerollSeeds.back();


  int rem = callsToTarget;
  //Default search, no forceEven consideration.
  if (requirements.forceEven){
    rem = evenPathFind(rem,instructions,rerollSeeds);
  } else {
    rem = standardPathFind(rem,instructions,rerollSeeds);
  }
  
  

  //2nd Printing block
  rerollSeeds.pop_back();
  seed = rerollSeeds.back();
  std::cout << "At reroll #" << instructions[3] << " the seed is: "<<std::hex << seed << std::dec <<" and the team generated will be: \n"
  << getBattleTeamInfo(seed) 
  << "\n\n+ + + + + + + + + + + + \n\n";
  printInstructions(instructions);
  std::cout << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n";

  //WRAP UP BLOCK
  eevee = {};
  instructions[0] = 0;
  instructions[1] = 0;
  instructions[2] = 0;
  instructions[3] = 0;
  std::cout <<"\nThe current result has been saved.\nEnter a command to continue:\nCommand: ";
  getline(std::cin,currentCommand);
  formatCase(currentCommand,lower);
  currentCommand.at(0) = toupper(currentCommand.at(0));
  //std::cout <<"RESULTS STORED: " << previousResults.size() << "\n";
  bool validCommand = false;
  while (!validCommand)
  {
    if (currentCommand == commands[3]){
      //Settings
      validCommand = true;
      requirements = {};
      writeReqsConfig(requirements);
    } else if (currentCommand == commands[0]){
      //Reject
      validCommand = true;
      searchActive = true;
      seed = listingSeed;
      previousResults.push_back(listingSeed);
    } else if (currentCommand == commands[1]){
      //Restore
      validCommand = true;
      searchActive = true;
      for (unsigned int i = 0; i < previousResults.size(); i++)
      {
        std::cout << std::hex << previousResults.at(i) << std::endl;
      }
      if (previousResults.size() > 0){
        previousResults.pop_back();
      }
      seed = LCG_BACK(previousResults.back());
      if (previousResults.size() == 0){
        seed = userInputRerollSeed;
      }
    } else if(currentCommand == commands[2]){
      //Reset
      validCommand = true;
      firstRun = true;
      userInputRerollSeed = getInputSeed();
      seed = userInputRerollSeed;
      listingSeed = seed;
    } else if (currentCommand == commands[4]){
      //Exit
      validCommand = true;
      searchActive = false;
    } else {
      std::cout << "Invalid command. Please try again: ";
      getline(std::cin,currentCommand);
      formatCase(currentCommand,lower);
      currentCommand.at(0) = toupper(currentCommand.at(0));

    }
  }
  }
  return 0;
}

    /*

    METHOD:
    Get the seed of the NTSC REROLL at the bottom, on the final reroll's page. Unfortunately can't copy/paste.

    Get Seed of Target espeon.

    Reroll up to the target like you normally would.

    Program calc's distance from reroll seed to target, and then gives you instructions to get to a value 2 frames before
    the target seed, which is 1000 frames before the seed you see in the program.

    From there, follow the program's instructions: 
    [0] = memcard reload. take memcard out and continue without saving. then reinsert memcard, back to main menu and re enter. 1009 calls.
    [1] = Rumble switch. Go into options menu, flip it from on to off or vice versa, and accept the save prompt. 40 calls.
    [2] = Naming screen back outs. Enter name screen and press b to return to main menu. 2 calls.
    [3] = Traditional re-roll. WIP. Unknown call amount. Will know once implemented.


    example: target is ntsc 268 rerolls out. 
    351C941E initial.
    8A55887A 10 rerolls to ntsc target.
    Frame 562.

    F9FD38FF on title screen.
    Assume starting in reroll menu
    Starting off with a reroll focused system, may eventually move to memcard focused.
    lets just see what happens.
    0.73s vs 3.73, memcard is worth 5x rerolls, ish in time, but less than that in calls, typically.
    plus no benefit of seeing where you are exactly.

    Reroll: 0.73
    Semiroll: 2.00
    Memcard: 3.73
    Rumble: 6.36 6.2 if really fast lol)
    NameScreen: 3.30


    
*/  
  
/*
may need to locally implement rerolling function T_T

    // LCGn(seed,2);
    // std::cout << "Initial 2 calls, result: " << std::hex << seed << std::endl;
    // for (int i = 0; i < 724; i++)
    // {
    //     rollRNGwithHiSeed(seed);
    // }


    // for (int i = 0; i < target; i++)
    // {
    // rollRNGwithHiSeed(seed);
    // debugSeed = seed;
    // if (i == target -1){
    //     std::cout <<"AIM FOR THIS: " << std::hex << debugSeed << std::endl;
    // }
    // LCGn(debugSeed,1000);
    // }
    

    // for (int i = 0; i < 116; i++){
    //     rollRNGwithHiSeed(seed);
    // }
    // std::cout << "NameScreen Seed: " << std::hex << seed << std::endl;
    */