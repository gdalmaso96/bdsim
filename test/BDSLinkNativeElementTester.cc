/*
Beam Delivery Simulation (BDSIM) Copyright (C) Royal Holloway,
University of London 2001 - 2024.

This file is part of BDSIM.
*/

#include "BDSBunchSixTrackLink.hh"
#include "BDSHitSamplerLink.hh"
#include "BDSIMLink.hh"
#include "BDSParticleCoordsFull.hh"
#include "BDSParticleDefinition.hh"

#include "CLHEP/Units/SystemOfUnits.h"

#include "G4Proton.hh"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
  if (argc != 5)
    {
      std::cerr << "Usage: BDSLinkNativeElementTester "
                << "<gmad-file> <element-name> <length-m> <transport|absorbed>"
                << std::endl;
      return 1;
    }

  const std::string inputFile   = argv[1];
  const std::string elementName = argv[2];
  const G4double expectedLength = std::stod(argv[3]) * CLHEP::m;
  const std::string outcome     = argv[4];

  BDSBunchSixTrackLink bunch;
  BDSIMLink link(&bunch);
  std::vector<std::string> arguments = {
    "bdsim",
    "--file=" + inputFile,
    "--output=None",
    "--batch",
    "--seed=1234"
  };
  std::vector<char*> argumentPointers;
  for (auto& argument : arguments)
    {argumentPointers.push_back(argument.data());}

  link.Initialise((G4int)argumentPointers.size(), argumentPointers.data(), false);
  if (!link.Initialised())
    {
      std::cerr << "BDSLink failed to initialise" << std::endl;
      return 1;
    }

  const G4double measuredLength = link.GetChordLengthOfLinkElement(elementName);
  if (std::abs(measuredLength - expectedLength) > 1e-9 * CLHEP::mm)
    {
      std::cerr << "Wrong link length for " << elementName
                << ": " << measuredLength/CLHEP::m << " m, expected "
                << expectedLength/CLHEP::m << " m" << std::endl;
      return 1;
    }

  const G4double kineticEnergy = 100 * CLHEP::GeV;
  auto* particle = new BDSParticleDefinition(G4Proton::ProtonDefinition(),
                                             0,
                                             kineticEnergy,
                                             0,
                                             1);
  BDSParticleCoordsFull coordinates(1 * CLHEP::mm,
                                    0,
                                    0,
                                    0,
                                    0,
                                    1,
                                    0,
                                    0,
                                    particle->TotalEnergy(),
                                    1);
  constexpr G4int externalParticleID = 17;
  bunch.AddParticle(particle,
                    coordinates,
                    externalParticleID,
                    externalParticleID);

  const G4int expectedSamplerID = link.GetLinkIndex(elementName);
  link.SelectLinkElement(elementName);
  link.BeamOn(1);

  const BDSHitsCollectionSamplerLink* hits = link.SamplerHits();
  const BDSHitSamplerLink* primaryHit = nullptr;
  if (hits)
    {
      for (G4int i = 0; i < (G4int)hits->entries(); ++i)
        {
          const BDSHitSamplerLink* hit = (*hits)[i];
          if (hit->parentID == 0)
            {
              primaryHit = hit;
              break;
            }
        }
    }

  if (outcome == "transport")
    {
      if (!primaryHit)
        {
          std::cerr << "Primary particle did not leave " << elementName;
          if (hits)
            {
              std::cerr << "; sampler contains " << hits->entries() << " hit(s)";
              for (G4int i = 0; i < (G4int)hits->entries(); ++i)
                {
                  const BDSHitSamplerLink* hit = (*hits)[i];
                  std::cerr << " [external=" << hit->externalParticleID
                            << ", parent=" << hit->parentID
                            << ", track=" << hit->trackID
                            << ", sampler=" << hit->samplerID << "]";
                }
            }
          std::cerr << std::endl;
          return 1;
        }
      if (primaryHit->samplerID != expectedSamplerID)
        {
          std::cerr << "Hit belongs to sampler " << primaryHit->samplerID
                    << ", expected " << expectedSamplerID << std::endl;
          return 1;
        }
    }
  else if (outcome == "absorbed")
    {
      if (primaryHit)
        {
          std::cerr << "Primary particle unexpectedly left " << elementName << std::endl;
          return 1;
        }
    }
  else
    {
      std::cerr << "Unknown outcome: " << outcome << std::endl;
      return 1;
    }

  return 0;
}
