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
      std::cerr << "Usage: BDSLinkComponentIndexTester <gmad-file>" << std::endl;
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
      std::cerr << "BDSLink failed to initialise" << std::endl;
      return 1;
    }

  const std::string shortName = "short_drift";
  const std::string longName  = "long_drift";
  const G4int shortID = link.GetLinkIndex(shortName);
  const G4int longID  = link.GetLinkIndex(longName);
  if (shortID < 0 || longID < 0 || shortID == longID)
    {
      std::cerr << "Parser-defined link elements do not have distinct link IDs" << std::endl;
      return 1;
    }

  const G4double tolerance = 1 * CLHEP::nm;
  const G4double expectedShortLength = 0.2 * CLHEP::m;
  const G4double expectedLongLength  = 0.7 * CLHEP::m;
  const G4double shortChord = link.GetChordLengthOfLinkElement(shortName);
  const G4double shortArc   = link.GetArcLengthOfLinkElement(shortName);
  const G4double longChord  = link.GetChordLengthOfLinkElement(longName);
  const G4double longArc    = link.GetArcLengthOfLinkElement(longName);
  if (std::abs(shortChord - expectedShortLength) > tolerance ||
      std::abs(shortArc   - expectedShortLength) > tolerance ||
      std::abs(longChord  - expectedLongLength)  > tolerance ||
      std::abs(longArc    - expectedLongLength)  > tolerance)
    {
      std::cerr << "Named link-element length lookup returned the wrong component: "
                << "short chord/arc = " << shortChord/CLHEP::m << "/"
                << shortArc/CLHEP::m << " m, long chord/arc = "
                << longChord/CLHEP::m << "/" << longArc/CLHEP::m << " m"
                << std::endl;
      return 1;
    }

  const G4double kineticEnergy = 100 * CLHEP::GeV;
  auto* particle = new BDSParticleDefinition(G4Proton::ProtonDefinition(),
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

  link.SelectLinkElement(longName);
  link.BeamOn(1);
  const BDSHitsCollectionSamplerLink* hits = link.SamplerHits();
  if (!hits || hits->entries() != 1)
    {
      std::cerr << "The selected parser-defined link element was not tracked" << std::endl;
      return 1;
    }

  return 0;
}
