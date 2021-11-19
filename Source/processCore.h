#include <iostream>
#include <iomanip>
#include <array>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <math.h>

//TODO: before launching, move this into the game specific folders and update the paths.

//Typedef block
typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int64_t s64;

enum region {USA,EUR,JPN};
enum emuVer {STABLE,MODERN}; //Stable == 5.0, only matters for xd so far.
enum coloSecondary {QUILAVA,CROCONAW,BAYLEEF}; //xd only has teddy
//enum secondaryMon {TEDDIURSA, QUILAVA, CROCONAW, BAYLEEF};
enum strCase {upper,lower};

struct PokemonProperties
  {
    int hpIV = 0;
    int atkIV = 0;
    int defIV = 0;
    int spAtkIV = 0;
    int spDefIV = 0;
    int speedIV = 0;
    int hpStartingStat = 0;
    int hiddenPowerTypeIndex = 0;
    int hiddenPowerPower = 0;
    int genderIndex = 0;
    int natureIndex = 0;
    int isShiny = 0;
    u32 trainerId = 0;
  };
struct PokemonRequirements
{
    int hpIV = 0;
    int atkIV = 0;
    int defIV = 0;
    int spAtkIV = 0;
    int spDefIV = 0;
    int speedIV = 0;
    std::array<bool,25> validNatures;
    std::array<bool,16> validHPTypes;
    int hiddenPowerPower = 0;
    int genderIndex = 0;
    int isShiny = 0;  
    bool forceEven = 0;
};

//extra functions block
u32 modpow32(u32 base, u32 exp)
{
  u32 result = 1;
  while (exp > 0)
  {
    if (exp & 1)
      result = result * base;
    base = base * base;
    exp >>= 1;
  }
  return result;
}
int median(std::vector<int> &v)
{
    size_t n = v.size() / 2;
    nth_element(v.begin(), v.begin()+n, v.end());
    return v[n];
}
std::string formatCase(std::string &str, strCase ulCase){
for (unsigned int i = 0; i < str.length(); i++)
    {
      if (ulCase){
        str.at(i) = tolower(str.at(i));
      } else {
        str.at(i) = toupper(str.at(i));
      }
    }
return str;
}

//RNG Block
u32 LCG(u32& seed){
  seed = seed * 214013 + 2531011;
  return seed;
}
u32 LCG_BACK(u32 &seed) {
    seed = seed * 0xB9B33155 + 0xA170F641;
    return seed;
}
u32 LCGn(u32& seed, const u32 n)
  {
    u32 ex = n - 1;
    u32 q = 0x343fd;
    u32 factor = 1;
    u32 sum = 0;
    while (ex > 0)
    {
      if (!(ex & 1))
      {
        sum = sum + (factor * modpow32(q, ex));
        ex--;
      }
      factor *= (1 + q);
      q *= q;
      ex /= 2;
    }
    seed = (seed * modpow32(0x343fd, n)) + (sum + factor) * 0x269EC3;
    return seed;
  }
u32 LCGn_BACK(u32&seed, const u32 n){
    u32 ex = n - 1;
    u32 q = 0xB9B33155;
    u32 factor = 1;
    u32 sum = 0;
    while (ex > 0)
    {
      if (!(ex & 1))
      {
        sum = sum + (factor * modpow32(q, ex));
        ex--;
      }
      factor *= (1 + q);
      q *= q;
      ex /= 2;
    }
    seed = (seed * modpow32(0xB9B33155, n)) + (sum + factor) * 0xA170F641;
    return seed;
}
float LCGPercentage(u32& seed){
  float percentResult = 0;
  u32 hiSeed = 0;
  LCG(seed);
  hiSeed = seed >> 16;
  percentResult = static_cast<float>(hiSeed)/65536;
  return percentResult;
}
u16 rollRNGwithHiSeed(u32 &seed)
{ //mostly used in the NTSC naming screen, may have uses elsewhere, like blink.
  LCG(seed);
  u16 hiSeed = seed >> 16;
  if (static_cast<double>(hiSeed) / 65536.0 < 0.1){
    LCGn(seed, 4);
  }
  return hiSeed; //debugging.
}


u32 simplePID (u32 &seed){
    u32 hId = LCG(seed) >> 16;
    u32 lId = LCG(seed) >> 16;
    u32 PID = (hId << 16) | (lId);
    return PID;
}
bool isPidShiny(const u16 TID, const u16 SID, const u32 PID)
  {
    return ((TID ^ SID ^ (PID & 0xFFFF) ^ (PID >> 16)) < 8);
  }
void setIDs (u32 &seed, u32 &TID, u32 &SID){
    TID = LCG(seed) >> 16; //lTrainerId
    SID = LCG(seed) >> 16; //hTrainerId
}
int getPidGender(const u8 genderRatio, const u32 pid)
  {
    return genderRatio > (pid & 0xff) ? 1 : 0;
  }
void extractIVs(PokemonProperties& properties, u32& seed)
  {
    // HP, ATK, DEF IV
    LCG(seed);
    properties.hpIV = (seed >> 16) & 31;
    properties.atkIV = (seed >> 21) & 31;
    properties.defIV = (seed >> 26) & 31;
    // SPEED, SPATK, SPDEF IV
    LCG(seed);
    properties.speedIV = (seed >> 16) & 31;
    properties.spAtkIV = (seed >> 21) & 31;
    properties.spDefIV = (seed >> 26) & 31;
  }
void fillStarterGenHiddenPowerInfo(PokemonProperties& starter)
  {
    int typeSum = (starter.hpIV & 1) + 2 * (starter.atkIV & 1) + 4 * (starter.defIV & 1) +
                  8 * (starter.speedIV & 1) + 16 * (starter.spAtkIV & 1) +
                  32 * (starter.spDefIV & 1);
    starter.hiddenPowerTypeIndex = typeSum * 15 / 63;
    int powerSum = ((starter.hpIV & 2) >> 1) + 2 * ((starter.atkIV & 2) >> 1) +
                   4 * ((starter.defIV & 2) >> 1) + 8 * ((starter.speedIV & 2) >> 1) +
                   16 * ((starter.spAtkIV & 2) >> 1) + 32 * ((starter.spDefIV & 2) >> 1);
    starter.hiddenPowerPower = (powerSum * 40 / 63) + 30;
  }

//basic generation -- only asks for gender ratio does not account for xd anti-shiny.
//Still unsure if I should put this here in the header, might need to modify it too much.
void generateMon(uint32_t inputSeed, int genderRatio){
//   uint32_t TID = 0; //Tid isn't important here since TID is already set. *might matter for shiny check on quil.
  uint32_t PID = 0;
  uint32_t seed = inputSeed;
  uint32_t outSeed = 0;
  std::string nature = " ";
  std::string hiddenPowerType = " ";
  //Gender ratio reference:  0x7F for teddy (50/50) and 0x1F for johto starters (87.5/12.5)
  const std::string naturesList[25] = {"Hardy","Lonely","Brave","Adamant","Naughty","Bold","Docile","Relaxed",
    "Impish","Lax","Timid","Hasty","Serious","Jolly","Naive","Modest","Mild","Quiet","Bashful",
    "Rash","Calm","Gentle","Sassy","Careful","Quirky"};
  const std::string hpTypes[16] = {"Fighting", "Flying", "Poison", "Ground", "Rock", "Bug", "Ghost", 
    "Steel", "Fire", "Water", "Grass", "Electric",  "Psychic", "Ice","Dragon","Dark"};
  
  std::cout << std::left;

    //Some tid/sid stuff, possibly a dummy pid
    seed = LCGn(seed,4);

    outSeed = seed;
    //Would be High bits/Secret ID, used for Shiny.

    // LCG(seed); These do not appear on shadow generation.
    // LCG(seed); //originally from Dummy ID, leftover.

    //IVS
    LCG(seed);
    int hp = (seed >> 16) & 31;
    int atk = (seed >> 21) & 31;
    int def = (seed >> 26) & 31; 
 
    LCG(seed); 
    int spe = (seed >> 16) & 31;
    int spa = (seed >> 21) & 31;
    int spd = (seed >> 26) & 31; 

    
    LCG(seed); //Ability call
    //PID STUFF blind, considers no weights:
    uint32_t hId = LCG(seed) >> 16;
    uint32_t lId = LCG(seed) >> 16;
    PID = (hId << 16) | (lId);

    std::string displayNature = naturesList[PID % 25];
    bool pidGender = genderRatio > (PID & 0xFF) ? 1 : 0;
    std::string displayGender;
    if (pidGender){
        displayGender = "Female";
    } else {
        displayGender = "Male";
    }
    std::cout 
    << "Seed"  
    << ": " 
    << std::hex 
    << std::setw(8) << outSeed
    << " PID: " 
    << std::setw(8) << PID 
    << std::dec
    << "  " << std::setw(2) << hp << " " << std::setw(2) << atk << " " << std::setw(2) << def << " " << std::setw(2)
    << spa << " " << std::setw(2) << spd << " " << std::setw(2) << spe << "  "
    << std::setw(7) << displayNature << "  " << displayGender 
    << std::endl;
}
//implement condensed generation

//File Reading
std::vector<int> decimalReadNumbersFromFile(std::string fileName)
{
    int lineRead = 0;
    std::vector<int> data; //Setup
    std::ifstream file(fileName);
    std::cout << "Read some data! \n";
    if (file.fail())
    {
        std::cout << "File inaccessible";
        exit(EXIT_FAILURE);
    }
    while (!(file.fail()))
    {  
        file >> lineRead; 
        data.push_back(lineRead);
    }
    file.close();
    return data;
}
std::vector<u32> hexReadNumbersFromFile(std::string fileName)
{
    u32 value;
    std::string lineRead = "";
    std::stringstream hexConvert;
    std::vector<u32> data; //Setup
    std::ifstream file(fileName);
    std::cout << "Read some data! \n";
    if (file.fail())
    {
        std::cout << "File inaccessible";
        exit(EXIT_FAILURE);
    }
    while (!(file.fail()))
    {                 
        getline(file,lineRead); 
        hexConvert << std::hex << lineRead; //cuz i just HAD to have my docs in hexa and not int...;
        hexConvert >> value;
        hexConvert.clear();
        hexConvert.str("");
        data.push_back(value);
        // std::cout << "Lines read: " << data.size() << endl;
    }
    file.close();
    return data;
}
std::vector<std::string> readStringFromFile(std::string fileName)
{
    std::vector<std::string> data; //Setup
    std::string lineRead = "";
    std::ifstream file(fileName);
    if (file.fail())
    {
        std::cout << "File inaccessible";
        exit(EXIT_FAILURE);
    }
    while (!(file.fail()))
    {
        getline(file,lineRead);
        data.push_back(lineRead);
    }
    file.close();
    std::cout << "lines from " + fileName + " read: " << data.size() << std::endl;
    return data;
}

//Debugging
int findGap(u32 behind, u32 ahead, bool forward){
  int counter = 0;
  if (forward){ //what happens when the origin/behind is actually ahead of target/ahead? infinite loop?
    while(behind != ahead){
      LCG(behind);
      counter++;
      if (counter > 100000000){
        std::cout <<"findGap() Error!";
        break; 
      }
    }
  } else { //do i need to swap the params here? I just want to prevent an infinite loop.
    while(behind != ahead){
      LCG_BACK(behind);
      counter++;
      if (counter > 100000000){
        std::cout <<"findGap() Error!";
        break; 
      }
    }
  }
  return counter;
}
void debugPrint2DVec(std::vector<std::vector<int>>set){
  //FOR DEBUG:
        for (unsigned int i = 0; i < set.size(); i++){
          for (unsigned int j = 0; j < set.at(i).size(); j++){
            std::cout << set.at(i).at(j) << ", ";
          }
          std::cout << std::endl;
        }
}
void debugPrintVec(std::vector<int>set){
    for (unsigned int i = 0; i < set.size(); i++)
    {
        if(i <set.size()-1){
          std::cout << set.at(i) << ", ";
        }
    }
    std::cout << std::endl;
}