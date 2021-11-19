#include "processCore.h"
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
int seekCallsToTargetEevee(u32 originSeed, u32 TargetSeed){
    return findGap(originSeed,LCGn_BACK(TargetSeed,1002),1);
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
int testRerolls(u32 testSeed,int &callsToTarget, std::vector<u32>&rerollSeeds,std::vector<int>m_criteria){
  bool maxedOut = false;
  int rolls = 0;
  int i = 0;
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
  //The searching code below is super janky, but it works. Seems to perform reasonably well. Ran a 850,000
  std::vector<u32>temporary  = {seed};
  std::vector<int> m_criteria = {-1, -1, -1, -1, -1, -1};
  testRerolls(seed,ctt,temporary,m_criteria);
  while (!temporary.size() > 2){
    if (ctt % 2 == 0){
      return true;
    } else {
      std::cout << "hit multiple rolls before 1009";
      if (temporary.size() > 1){
        ctt+= findGap(temporary.back(),temporary.at(temporary.size()-2),0);
        temporary.pop_back();
      } else {
        temporary.clear();
      }
    }
  }
  return false;
}
bool CTTAcceptable2(u32 seed,int ctt,std::vector<int>&counts){
  if (ctt >= 1009 || ctt % 2 == 0){
    return true;
  } else {
    counts.at(0)++;
  }
  std::vector<u32>temporary  = {seed};
  std::vector<int> m_criteria = {-1, -1, -1, -1, -1, -1};
  testRerolls(seed,ctt,temporary,m_criteria);
  if(temporary.size() == 2){
    counts.at(2)++;
    if (ctt % 2 == 0){
      counts.at(1)++;
      return true;
    } else {
      counts.at(7)++;
    }
  }
  while (temporary.size() > 2){
    counts.at(3)++;
    if (ctt % 2 == 0){
      counts.at(4)++;
      return true;
    } else {
      if (temporary.size() > 1){
        ctt+= findGap(temporary.back(),temporary.at(temporary.size()-2),0);
        temporary.pop_back();
      } else {
        temporary.clear();
      }
    }
    //needs testing?
  }
  if (ctt % 2 == 0){
    counts.at(5)++;
      return true;
  }
  counts.at(6)++;
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
void readReqsConfig(PokemonRequirements &inputReqs, std::ifstream &config){
int inputInt = 0;
    for (int i = 0; i < 11; i++)
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
    std::cout << "type the name of a type, or enter 'any' to accept all types: ";
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
    std::cout << "Want to choose a specific gender? Type 'Male' or 'M', 'Female' or 'F', or 'Any': ";
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
    std::cout << "Want the target to be shiny? enter Yes, No, or Any:";
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
    
    std::cout << "All requirements recorded! Good luck!\n";

    //config filewrite
    configW 
    << inputReqs.hpIV    <<"\n"<< inputReqs.atkIV   <<"\n"
    << inputReqs.defIV   <<"\n"<< inputReqs.spAtkIV <<"\n"
    << inputReqs.spDefIV <<"\n"<< inputReqs.speedIV <<"\n"
    << natureIDXs.size()<<"\n";
    for (unsigned int i = 0; i < natureIDXs.size(); i++)
    {
      configW << natureIDXs.at(i) << "\n";
    }
    configW << HPTIDXs.size()<<"\n";
    for (unsigned int i = 0; i < HPTIDXs.size(); i++)
    {
      configW << HPTIDXs.at(i) << "\n";
    }
    configW << inputReqs.hiddenPowerPower <<"\n"
    << inputReqs.genderIndex <<"\n"
    << inputReqs.isShiny;
    configW.close();
}
PokemonRequirements setPokeReqs(){
  //intended for initial run
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
      // std::cout <<"REQS: \n"
    // << requirements.hpIV <<"\n"
    // << requirements.atkIV <<"\n"
    // << requirements.defIV<<"\n"
    // << requirements.spAtkIV<<"\n"
    // << requirements.spDefIV<<"\n"
    // << requirements.speedIV<<"\n"
    // <<"Valid Natures: ";
    // for (unsigned int i = 0; i < 25; i++)
    // {
    //   if (requirements.validNatures[i] == true){
    //     std::cout << naturesList[i] << "\n";
    //   }
    // }
    // std::cout <<"Valid HPs: ";
    // for (unsigned int i = 0; i < 16; i++)
    // {
    //   if (requirements.validHPTypes[i] == true){
    //     std::cout << hpTypes[i] << "\n";
    //   }
    // }
    // std::cout
    // << requirements.hiddenPowerPower <<"\n"
    // << requirements.genderIndex <<"\n"
    // << requirements.isShiny <<"\n";
}
int main(){
    u32 seed = 0;
    u32 listingSeed = 0;
    PokemonRequirements requirements = setPokeReqs();
    PokemonProperties eevee;
    const int eeveeGenderRatio = 0x1F;
    int ctt = 0;
    int count = 0;
    bool cttPass = false;
    std::vector<int>counts = {0,0,0,0,0,0,0,0};
    for (int i = 0; i < INT32_MAX; i++)
    {
        seed = i;
        LCGn(seed,1002);
        while(!(foundRunnable(eevee,requirements) && cttPass)){
        listingSeed = LCG(seed); //seed incremented and stored
        condensedGenerateMon(eevee,seed,eeveeGenderRatio);
        if (foundRunnable(eevee,requirements)){
          cttPass = CTTAcceptable2(seed,seekCallsToTargetEevee(i,listingSeed),counts);
        }
        seed = listingSeed; //seed restored
    }
        eevee = {};
        //std::cout <<std::hex << seed << std::endl;
        ctt = seekCallsToTargetEevee(i,listingSeed);
        if (ctt > 100000000){
          std::cout << "Failure compare: " << i << " and " << seed << std::endl;
        }
        if (i % 1000 == 0){
          printf("#Seeds under1k9: %d, succeed 1 roll: %d, fail 1 roll: %d, total 1 roll: %d\ntotal >1 roll: %d,succeed >1 roll: %d, bounced loop success: %d\n total failure %d\n",
          counts.at(0),counts.at(1),counts.at(7),counts.at(2),counts.at(3),counts.at(4),counts.at(5),counts.at(6));
          std::cout << "I : " << i << " - SEEDS REM: " << INT32_MAX - i << std::endl;
        }
        //std::cout << "CTT: " <<std::dec << ctt << std::endl;
        CTTAcceptable2(seed,ctt,counts);
    }
    std::cout << "Success!";
    return 0;
}