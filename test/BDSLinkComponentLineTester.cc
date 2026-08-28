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
      std::cerr << "Usage: BDSLinkComponentLineTester "
                << "<gmad-file> <element-name> <energy|field|field-x|field-y>"
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

  const G4double expectedLength = 0.5 * CLHEP::m;
  const G4double measuredLength = link.GetChordLengthOfLinkElement(elementName);
  if (std::abs(measuredLength - expectedLength) > 1e-9 * CLHEP::mm)
    {
      std::cerr << "Wrong component-line length for " << elementName
                << ": " << measuredLength/CLHEP::m << " m" << std::endl;
      return 1;
    }

  const G4double kineticEnergy = 100 * CLHEP::GeV;
  auto* particle = new BDSParticleDefinition(G4Proton::ProtonDefinition(),
                                             0,
                                             kineticEnergy,
                                             0,
                                             1);
  const G4double x0 = 1 * CLHEP::mm;
  const G4double xp0 = 1e-3;
  const G4double zp0 = std::sqrt(1 - xp0*xp0);
  BDSParticleCoordsFull coordinates(x0,
                                    0,
                                    0,
                                    xp0,
                                    0,
                                    zp0,
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
      std::cerr << "Expected one link sampler hit for " << elementName << std::endl;
      return 1;
    }

  const BDSHitSamplerLink* hit = (*hits)[0];
  if (check == "energy")
    {
      if (std::abs(hit->coords.totalEnergy - particle->TotalEnergy()) < 1 * CLHEP::keV)
        {
          std::cerr << "RF cavity did not change the particle energy" << std::endl;
          return 1;
        }
    }
  else if (check == "field")
    {
      if (std::hypot(hit->coords.xp - xp0, hit->coords.yp) < 1e-12)
        {
          std::cerr << "No measurable field effect through " << elementName
                    << ": xp=" << hit->coords.xp
                    << ", yp=" << hit->coords.yp << std::endl;
          return 1;
        }
    }
  else if (check == "field-x")
    {
      if (std::abs(hit->coords.xp - xp0) < 1e-12)
        {
          std::cerr << "No horizontal RF kick through " << elementName
                    << ": xp=" << hit->coords.xp << std::endl;
          return 1;
        }
    }
  else if (check == "field-y")
    {
      if (std::abs(hit->coords.yp) < 1e-12)
        {
          std::cerr << "No vertical RF kick through " << elementName
                    << ": yp=" << hit->coords.yp << std::endl;
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
