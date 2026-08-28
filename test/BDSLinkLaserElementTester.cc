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

#include "G4Electron.hh"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace
{
  const BDSHitSamplerLink* PrimaryHit(const BDSHitsCollectionSamplerLink* hits)
  {
    if (!hits)
      {return nullptr;}
    for (G4int i = 0; i < (G4int)hits->entries(); ++i)
      {
        const auto* hit = (*hits)[i];
        if (hit->parentID == 0)
          {return hit;}
      }
    return nullptr;
  }
}

int main(int argc, char** argv)
{
  if (argc != 4)
    {
      std::cerr << "Usage: BDSLinkLaserElementTester "
                << "<gmad-file> <element-name> <transport|legacy-compton>"
                << std::endl;
      return 1;
    }

  const std::string inputFile   = argv[1];
  const std::string elementName = argv[2];
  const std::string check       = argv[3];

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

  const G4double expectedLength = elementName == "legacyLaser" ? 0.01 * CLHEP::m : 0.1 * CLHEP::m;
  const G4double measuredLength = link.GetChordLengthOfLinkElement(elementName);
  if (std::abs(measuredLength - expectedLength) > 1e-9 * CLHEP::mm)
    {
      std::cerr << "Wrong link length for " << elementName
                << ": " << measuredLength/CLHEP::m << " m" << std::endl;
      return 1;
    }

  const G4double kineticEnergy = 250 * CLHEP::GeV;
  auto* particle = new BDSParticleDefinition(G4Electron::ElectronDefinition(),
                                             0,
                                             kineticEnergy,
                                             0,
                                             1);
  BDSParticleCoordsFull coordinates(0,
                                    0,
                                    0,
                                    0,
                                    0,
                                    1,
                                    0,
                                    0,
                                    particle->TotalEnergy(),
                                    1);
  bunch.AddParticle(particle, coordinates, 17, 17);

  link.SelectLinkElement(elementName);
  const G4int repetitions = check == "legacy-compton" ? 2 : 1;
  for (G4int event = 0; event < repetitions; ++event)
    {
      link.BeamOn(1);
      const auto* primaryHit = PrimaryHit(link.SamplerHits());
      if (!primaryHit)
        {
          std::cerr << "Primary particle did not leave " << elementName
                    << " on event " << event << std::endl;
          return 1;
        }

      if (check == "legacy-compton" &&
          std::abs(primaryHit->coords.totalEnergy - particle->TotalEnergy()) < 1 * CLHEP::keV)
        {
          std::cerr << "Legacy laser did not scatter the electron on event "
                    << event << std::endl;
          return 1;
        }
      if (event + 1 < repetitions)
        {link.ClearSamplerHits();}
    }

  if (check != "transport" && check != "legacy-compton")
    {
      std::cerr << "Unknown check: " << check << std::endl;
      return 1;
    }

  return 0;
}
