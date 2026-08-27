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
  if (argc != 4)
    {
      std::cerr << "Usage: BDSLinkStraightElementTester <gmad-file> <element-name> <transport>" << std::endl;
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

  const G4double kineticEnergy = 100 * CLHEP::GeV;
  auto* particle = new BDSParticleDefinition(G4Proton::ProtonDefinition(),
                                             0,
                                             kineticEnergy,
                                             0,
                                             1);
  const G4double x0  = 1 * CLHEP::mm;
  const G4double y0  = -2 * CLHEP::mm;
  const G4double xp0 = 1e-3;
  const G4double yp0 = -2e-3;
  const G4double zp0 = std::sqrt(1 - xp0*xp0 - yp0*yp0);
  BDSParticleCoordsFull coordinates(x0,
                                    y0,
                                    0,
                                    xp0,
                                    yp0,
                                    zp0,
                                    0,
                                    0,
                                    particle->TotalEnergy(),
                                    1);
  bunch.AddParticle(particle, coordinates, 17, 17);

  const G4int expectedSamplerID = link.GetLinkIndex(elementName);
  link.SelectLinkElement(elementName);
  link.BeamOn(1);

  const BDSHitsCollectionSamplerLink* hits = link.SamplerHits();
  if (!hits || hits->entries() != 1)
    {
      std::cerr << "Expected one link sampler hit for " << elementName << std::endl;
      return 1;
    }
  const BDSHitSamplerLink* hit = (*hits)[0];
  if (hit->samplerID != expectedSamplerID)
    {
      std::cerr << "Hit belongs to sampler " << hit->samplerID
                << ", expected " << expectedSamplerID << std::endl;
      return 1;
    }

  if (check == "transport")
    {
      const G4double dx = hit->coords.x - x0;
      const G4double dy = hit->coords.y - y0;
      if (dx < 0.49 * CLHEP::mm || dx > 0.51 * CLHEP::mm ||
          dy > -0.98 * CLHEP::mm || dy < -1.02 * CLHEP::mm ||
          std::abs(hit->coords.xp - xp0) > 1e-12 ||
          std::abs(hit->coords.yp - yp0) > 1e-12)
        {
          std::cerr << "Unexpected drift through " << elementName
                    << ": dx=" << dx/CLHEP::mm << " mm"
                    << ", dy=" << dy/CLHEP::mm << " mm"
                    << ", xp=" << hit->coords.xp
                    << ", yp=" << hit->coords.yp << std::endl;
          return 1;
        }
    }
  else
    {
      std::cerr << "Unknown check: " << check << std::endl;
      return 1;
    }

  return 0;
}
