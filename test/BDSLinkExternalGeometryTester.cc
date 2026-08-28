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
  if (argc != 2)
    {
      std::cerr << "Usage: BDSLinkExternalGeometryTester <gmad-file>" << std::endl;
      return 1;
    }

  BDSBunchSixTrackLink bunch;
  BDSIMLink link(&bunch);
  std::vector<std::string> arguments = {
    "bdsim",
    "--file=" + std::string(argv[1]),
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
      std::cerr << "BDSLink failed to initialise external geometry" << std::endl;
      return 1;
    }

  const G4double expectedLength = 1.1 * CLHEP::m;
  if (std::abs(link.GetChordLengthOfLinkElement("external_box") - expectedLength) > CLHEP::nm)
    {
      std::cerr << "External-geometry element has the wrong length" << std::endl;
      return 1;
    }

  const G4double kineticEnergy = 100 * CLHEP::GeV;
  auto* particle = new BDSParticleDefinition(G4Proton::ProtonDefinition(),
                                             0,
                                             kineticEnergy,
                                             0,
                                             1);
  const G4double initialX = 10 * CLHEP::mm;
  const G4double initialY = -2 * CLHEP::mm;
  BDSParticleCoordsFull coordinates(initialX,
                                    initialY,
                                    0,
                                    0,
                                    0,
                                    1,
                                    0,
                                    0,
                                    particle->TotalEnergy(),
                                    1);
  bunch.AddParticle(particle, coordinates, 17, 17);

  link.SelectLinkElement("external_box");
  link.BeamOn(1);

  const BDSHitsCollectionSamplerLink* hits = link.SamplerHits();
  if (!hits || hits->entries() != 1)
    {
      std::cerr << "Expected one particle after the external geometry" << std::endl;
      return 1;
    }

  const BDSHitSamplerLink* hit = (*hits)[0];
  if (std::abs(hit->coords.x - initialX) > CLHEP::nm ||
      std::abs(hit->coords.y - initialY) > CLHEP::nm ||
      std::abs(hit->coords.xp) > 1e-12 ||
      std::abs(hit->coords.yp) > 1e-12)
    {
      std::cerr << "Vacuum external geometry changed the transverse coordinates: x="
                << hit->coords.x / CLHEP::mm << " mm, y="
                << hit->coords.y / CLHEP::mm << " mm, xp="
                << hit->coords.xp << ", yp=" << hit->coords.yp << std::endl;
      return 1;
    }

  return 0;
}
