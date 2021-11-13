#include "../processCore.h"
enum class WantedShininess
  {
    notShiny,
    shiny,
    any
  };
u32 generatePokemonPID(u32& seed, const u32 hTrainerId,
                                                     const u32 lTrainerId, const u32 dummyId,
                                                     u16* counter, const WantedShininess shininess,
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
                                                            const bool endPrematurely, u16* counter)
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

u32 rollRNGToBattleMenu(u32 seed, u16* counter)
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
      generatePokemonPID(seed, hTrainerId, lTrainerId, 0, counter, WantedShininess::notShiny,-1,0,-1);
      generateEVs(seed, true, true, counter);
    }
  }


  return seed;
}





int main(){

  std::cout << "Success!";




    return 0;
}