#pragma once
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
#include <stdio.h>

typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int64_t s64;
typedef std::vector<int>::iterator iterInt;

enum region {NTSCU,PAL60,PAL50,NTSCJ}; //Switch to NTSC, PAL and JPN? NTSC-U, PAL, NTSC-J is most correct but LONG.
enum emuVer {STABLE,MODERN}; //Stable == 5.0, only matters for xd so far.
enum coloSecondary {QUILAVA,CROCONAW,BAYLEEF}; //xd only has teddy
//enum secondaryMon {TEDDIURSA, QUILAVA, CROCONAW, BAYLEEF};
enum strCase {upper,lower};


const std::string hpTypes[16] = {"Fighting", "Flying", "Poison", "Ground", "Rock", "Bug", "Ghost",
    "Steel", "Fire", "Water", "Grass", "Electric",  "Psychic", "Ice","Dragon","Dark"};

const std::string naturesList[25] = {"Hardy","Lonely","Brave","Adamant","Naughty","Bold","Docile","Relaxed",
"Impish","Lax","Timid","Hasty","Serious","Jolly","Naive","Modest","Mild","Quiet","Bashful",
"Rash","Calm","Gentle","Sassy","Careful","Quirky"};


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
//    bool forceEven = 0;
};

//general functions block
u32 modpow32(u32 base, u32 exp);
int median(std::vector<int> &v);
std::string formatCase(std::string &str, strCase ulCase);
int consultPattern(int i, int offset, std::vector<int>pattern);
int ultraUltraCondensedCP(int i, int off, std::vector<int>pat);
//RNG Block
u32 LCG(u32& seed);
u32 LCG_BACK(u32 &seed);
u32 LCGn(u32& seed, const u32 n);
u32 LCGn_BACK(u32&seed, const u32 n);
double LCG_PullHi16 (uint32_t &seed);
float LCGPercentage(u32& seed);
u16 rollRNGwithHiSeed(u32 &seed);

//Pokemon Related
u32 simplePID (u32 &seed);
bool isPidShiny(const u16 TID, const u16 SID, const u32 PID);
void setIDs (u32 &seed, u32 &TID, u32 &SID);
int getPidGender(const u8 genderRatio, const u32 pid);
void extractIVs(PokemonProperties& properties, u32& seed);
void fillStarterGenHiddenPowerInfo(PokemonProperties& starter);
void coreDummyCam(uint32_t& seed,int camAngle1,int camAngle2,bool isFirst);

void generateMon(uint32_t inputSeed, int genderRatio);


//File Reading
std::vector<int> decimalReadNumbersFromFile(std::string fileName);
std::vector<u32> hexReadNumbersFromFile(std::string fileName);
std::vector<std::string> readStringFromFile(std::string fileName);


//Debugging
int findGap(u32 behind, u32 ahead, bool forward);
void debugPrint2DVec(std::vector<std::vector<int>>set);
void debugPrintVec(std::vector<int>set);
