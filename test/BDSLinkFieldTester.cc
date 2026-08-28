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
  if (argc != 3 && argc != 5)
    {
      std::cerr << "Usage: BDSLinkFieldTester <gmad-file> <element-name> [expected-x expected-xp]"
                << std::endl;
      return 1;
    }

  const std::string inputFile   = argv[1];
  const std::string elementName = argv[2];

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

  const G4double kineticEnergy = 100 * CLHEP::GeV;
  auto* particle = new BDSParticleDefinition(G4Proton::ProtonDefinition(),
                                             0,
                                             kineticEnergy,
                                             0,
                                             1);
  BDSParticleCoordsFull coordinates(10 * CLHEP::mm,
                                    2 * CLHEP::mm,
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
  link.BeamOn(1);

  const BDSHitsCollectionSamplerLink* hits = link.SamplerHits();
  if (!hits || hits->entries() != 1)
    {
      std::cerr << "Expected one link sampler hit" << std::endl;
      return 1;
    }

  const BDSHitSamplerLink* hit = (*hits)[0];
  const G4double deflection = std::hypot(hit->coords.xp, hit->coords.yp);
  if (deflection < 1e-6)
    {
      std::cerr << "The field was not applied: xp=" << hit->coords.xp
                << ", yp=" << hit->coords.yp << std::endl;
      return 1;
    }

  if (argc == 5)
    {
      const G4double expectedX  = std::stod(argv[3]) * CLHEP::m;
      const G4double expectedXP = std::stod(argv[4]);
      // The standalone reference is read from BDSIM's float sampler output.
      if (std::abs(hit->coords.x - expectedX) > 10 * CLHEP::nm ||
          std::abs(hit->coords.xp - expectedXP) > 1e-9)
        {
          std::cerr << "Field-map tracking differs from standalone BDSIM: x="
                    << hit->coords.x / CLHEP::m << ", xp=" << hit->coords.xp
                    << std::endl;
          return 1;
        }
    }

  return 0;
}
