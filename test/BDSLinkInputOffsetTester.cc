/*
Beam Delivery Simulation (BDSIM) Copyright (C) Royal Holloway,
University of London 2001 - 2024.

This file is part of BDSIM.
*/

#include "BDSBunchSixTrackLink.hh"
#include "BDSGlobalConstants.hh"
#include "BDSHitSamplerLink.hh"
#include "BDSIMLink.hh"
#include "BDSLinkComponent.hh"
#include "BDSLinkOpaqueBox.hh"
#include "BDSParticleCoordsFull.hh"
#include "BDSParticleDefinition.hh"
#include "BDSSamplerCustom.hh"

#include "CLHEP/Units/PhysicalConstants.h"
#include "CLHEP/Units/SystemOfUnits.h"

#include "G4Proton.hh"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
  if (argc != 2)
    {
      std::cerr << "Expected the link test GMAD file" << std::endl;
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

  const G4int expectedSamplerID = link.GetLinkIndex("probe");
  const BDSLinkComponent* linkComponent = link.GetLinkComponent(expectedSamplerID);
  if (!linkComponent || !linkComponent->Component())
    {
      std::cerr << "Could not retrieve the parser-built link component" << std::endl;
      return 1;
    }
  const G4double expectedInputOffset = BDSGlobalConstants::Instance()->LengthSafety();
  if (std::abs(linkComponent->Component()->InputTrackingOffset() - expectedInputOffset) >
      std::numeric_limits<G4double>::epsilon())
    {
      std::cerr << "The link component does not use the BDSIM length safety at input" << std::endl;
      return 1;
    }
  const G4double expectedOutputOffset = 2.5 * BDSSamplerCustom::ChordLength();
  if (std::abs(linkComponent->Component()->OutputTrackingOffset() - expectedOutputOffset) >
      std::numeric_limits<G4double>::epsilon())
    {
      std::cerr << "The link component does not account for its output sampler" << std::endl;
      return 1;
    }

  const G4double kineticEnergy = 100 * CLHEP::GeV;
  auto* particle = new BDSParticleDefinition(G4Proton::ProtonDefinition(),
                                             0,
                                             kineticEnergy,
                                             0,
                                             1);
  const G4double xIn = 1 * CLHEP::mm;
  const G4double yIn = -2 * CLHEP::mm;
  const G4double tIn = 3 * CLHEP::ns;
  const G4double xp = 0.1;
  const G4double yp = -0.2;
  const G4double zp = std::sqrt(1 - xp*xp - yp*yp);
  BDSParticleCoordsFull coordinates(xIn,
                                    yIn,
                                    0,
                                    xp,
                                    yp,
                                    zp,
                                    tIn,
                                    0,
                                    particle->TotalEnergy(),
                                    1);
  bunch.AddParticle(particle, coordinates, 17, 17);

  link.SelectLinkElement("probe");
  link.BeamOn(1);

  const BDSHitsCollectionSamplerLink* hits = link.SamplerHits();
  if (!hits || hits->entries() != 1)
    {
      std::cerr << "Expected one link sampler hit" << std::endl;
      return 1;
    }
  const BDSHitSamplerLink* hit = (*hits)[0];
  if (hit->samplerID != expectedSamplerID)
    {
      std::cerr << "Hit belongs to sampler " << hit->samplerID
                << ", expected " << expectedSamplerID << std::endl;
      return 1;
    }

  // Both link backsteps must cancel their respective safety distances so the
  // returned coordinates lie on the nominal output plane.
  const G4double trackedLength = 0.1 * CLHEP::m;
  const G4double expectedX = xIn + trackedLength*xp/zp;
  const G4double expectedY = yIn + trackedLength*yp/zp;
  const G4double expectedT = tIn + trackedLength /
                             (particle->Beta()*CLHEP::c_light*zp);
  const G4double positionTolerance = 1e-8 * CLHEP::mm;
  const G4double timeTolerance = 1e-9 * CLHEP::ns;
  if (std::abs(hit->coords.x - expectedX) > positionTolerance ||
      std::abs(hit->coords.y - expectedY) > positionTolerance ||
      std::abs(hit->coords.T - expectedT) > timeTolerance)
    {
      std::cerr << std::setprecision(17)
                << "Input backstep changed the existing link output convention\n"
                << "  x: " << hit->coords.x << " expected " << expectedX << "\n"
                << "  y: " << hit->coords.y << " expected " << expectedY << "\n"
                << "  T: " << hit->coords.T << " expected " << expectedT
                << std::endl;
      return 1;
    }
  return 0;
}
