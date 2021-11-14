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

u32 complexPID(u32& seed, const u32 hTrainerId,
                                                     const u32 lTrainerId, const u32 dummyId,
                                                     const WantedShininess shininess,
                                                     const s8 wantedGender, const u32 genderRatio,
                                                     const s8 wantedNature)
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
void determineFinalInstructions(int instructions[4], int &remainingCalls){
  //ASSUMES EVEN
  while (remainingCalls > 0){
    if (remainingCalls >= 40){
      instructions[1] = remainingCalls / 40; //integer division intentional, want to always round down for safety.
      remainingCalls -= instructions[1]*40;
    } else {
      //std::cout <<"REM: " << remainingCalls << "\n";
      if (remainingCalls > 1){
        instructions[2] = remainingCalls / 2;
        remainingCalls -= instructions[2]*2;
        //std::cout << "BACKOUTS: " << instructions[2]*2;
      } else {
          std::cout << "ERROR: REM IS UNEVEN\n";
          remainingCalls = 0;
      }
    }
  }
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
u32 singleRoll(u32&seed,std::vector<int>m_criteria){
  mGenerateBattleTeam(seed,m_criteria,m_criteria);
  return seed;
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

// bool getReqString(int n){
//   bool workingItem[n];
//   std::string inputStr;
//   std::cout << "Enter acceptable terms(s): \n";
//   bool allStringsEntered = false;
//   int sizeOfLoop = 25;
//   std::cout << "type the name of the requirement, or enter 'any' to accept all natures: ";
//   while (!allStringsEntered){
//     getline(std::cin,inputStr);
//     formatCase(inputStr,lower);
//     inputStr.at(0) = toupper(inputStr.at(0)); //capitalize
//     std::cout << "item entered: " << inputStr << std::endl;

//     //exit statements:
//     if (inputStr == "Done"){
//       allStringsEntered = true;
//     } else if (inputStr == "Any"){
//       for (int i = 0; i < n; i++)
//       {
//         workingItem[i] = true;
//         allStringsEntered = true;
//       }
//     } else {
//       bool foundAny = false;
//       for (int i = 0; i < n; i++)
//       {
//         if (inputStr == naturesList[i]){
//           std::cout <<"Added "<< naturesList[i] <<"!\n";
//           foundAny = true;
//           workingItem[i] = true;
//         }
//       }
//       if (!foundAny){
//         std::cout << "Nature not found - invalid input.";
//       }
//       std::cout << "Enter more items or use commands 'Any' or 'Done': ";
//       int full = 0;
//       for (int i = 0; i < n; i++)
//       {
//         if (workingItem[i] == true){
//           full++;
//         }
//       }
//       if (full == n){
//         std::cout << "You really took the time to manually enter every nature/hidden power...wow." 
//         <<"fine! all natures/hidden powers are enabled, moving on.";
//         allStringsEntered = true;
//       }
//     }
//   }

//   return workingItem;
// }
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

void printPokeInfo(){

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
    //an Oops I got lost - feature might be a nice addition to the main tool,
    //which can find ur seed based on 1 or at most 2 rolls of input again.
    //Also a forward and backwards tabber to move through the rolls (with the team members)
    //unlike with seed alone like it is now.

    // const int namingValue = 2;
    // const int rumbleValue = 40; //note in colo this is 20 calls instead.
    // const int memcardValue = 1009;
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
    

    // requirements.spAtkIV = 31;
    // requirements.speedIV = 22;
    // requirements.hiddenPowerPower = 68;
    // requirements.validHPTypes[Psychic] = true;
    // requirements.validHPTypes[Electric] = true;
    // requirements.validNatures[Mild] = true;
    // requirements.validNatures[Modest] = true;
    // requirements.validNatures[Rash] = true;
    // requirements.isShiny = false;

    printLogo();

    userInputRerollSeed = getInputSeed();
    seed = userInputRerollSeed;
    listingSeed = seed;
    
    requirements = setPokeReqs(); //What a chonky function.

  bool searchActive = true;
  //minimum calls in order to be able to hit the target.
  LCGn(seed,1002);

  while(searchActive){
    //search for runnable eevee.
    // int eeveesSearched = 0;
    while(!foundRunnable(eevee,requirements)){
        // eeveesSearched++;
        listingSeed = LCG(seed);
        condensedGenerateMon(eevee,seed,eeveeGenderRatio);
        seed = listingSeed;
    }
    // std::cout << "Eevees searched: " << eeveesSearched << "\n";
    // std::cout << "Reached: " << std::hex << listingSeed << std::endl;
    LCGn_BACK(seed,1002);
    titleSeed = seed;
    std::cout << "Target (seed at title screen): " << std::hex << titleSeed << std::dec << std::endl;
   
    //find calls to target.
    int callsToTarget = 0;
    callsToTarget = findGap(userInputRerollSeed,titleSeed,1);
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
        std::cout << "Male";
    } else {
        std::cout << "Female";
    }
    std::cout << "\nShiny: ";
    if (eevee.isShiny){
      std::cout << "True";
    } else {
      std::cout << "False";
    }
    std::cout << "\n";
    std::cout << "\n+ + + + + + + + + + + + \n\n";

//~~~~~~~~~~~~~~~~~~Target seeking is complete!~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//~~~~~~~~~~~~~~~~~~~Now find a path to target.~~~~~~~~~~~~~~~~~~~~~~~~~~~~

seed = userInputRerollSeed; //MUST RESET SEED BACK TO MENU SO THAT PATHFINDING WORKS.

//std::cout << "INITIAL CTT: " << callsToTarget << "\n";
std::vector<u32> rerollSeeds = {seed};
//goal, bring findGap(seed,titleSeed,1) to zero.
u32 testSeed = seed;
int distance = 0;
bool maxedOut = false;
while(!maxedOut){
  // std::cout << "rem CTT: " << callsToTarget << "\n";
  // std::cout << "rerolls done: " << instructions[3] << "\n"; 
  mGenerateBattleTeam(testSeed,m_criteria,m_criteria);
  distance = findGap(rerollSeeds.back(),testSeed,1);
  if (distance <= callsToTarget){
    //success,apply rolls
    rerollSeeds.push_back(testSeed);
    //std::cout << "Seed ctt before application:" << findGap(seed,titleSeed,1) << std::endl;
    seed = testSeed;
    //std::cout << "Seed ctt after application:" << findGap(seed,titleSeed,1) << std::endl;
    callsToTarget -= distance;
    instructions[3]++;
  } else {
    //break case
    maxedOut = true;
  }
}
//error check
//std::cout << "Approach error Check! " << std::hex << seed;
// if (findGap(seed,titleSeed,1) != callsToTarget){
//   std::cout << "ERROR! CTT: " << callsToTarget << " WHILE SEED IS: " << findGap(seed,titleSeed,1) << " CALLS AWAY!";
// } else {
//   std::cout <<"PASSED ERROR CHECK!\n";
// }


//~~~~~ OPTIMIZE PATH BLOCK - WIP ~~~~~~~~~~~~~~
// This manip works a bit differently than NTSC, since ntsc simply scans until it gets lucky, so the
//specific counter isn't that important, except for debugging.
//This manip identifies a target in the vast swirling ocean of RNG and hunts it down directly.
int rem = callsToTarget;
while(rem > 0){
  //std::cout << "Post-Reroll ctt:" << rem << "\n";
  if (rem % 2 == 0){//if rem is even
    while (rem >= 2018){
      instructions[0] += 2; // will continue to stay even at this rate.
      rem-= 2018;
    }
  determineFinalInstructions(instructions,rem);
  } else { //rem is odd
    if (rem >=1009){
      instructions[0]++; //congratulations, now the count is even
      rem-= 1009; //on next loop will not trigger because is now even.
    } else {
      //subtract a reroll and try again
      instructions[3]--;
      rem += findGap(rerollSeeds.back(),rerollSeeds.at(rerollSeeds.size()-2),0); 
      rerollSeeds.pop_back();
      seed = rerollSeeds.back();
    }   
  }
}
//note seed is set to the value at the end of rerolling.

    

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
    std::cout << "\n\n";
    std::cout <<"The current result has been saved.\nEnter a command to continue:\nCommand: ";
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
    
    
    
      //assuming most things don't need to be cleared, cuz theres a looooot of variables.

      //reset instructions
      // for (int i = 0; i < 3; i++)
      // {
      //   instructions[i] = 0;
      // }
      // currentCommand = "";
    }
    //process other commands.
    
    // std::cout << "\nEnter any key to exit...\n";
    // getchar();
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