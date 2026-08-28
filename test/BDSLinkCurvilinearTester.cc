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

#include "CLHEP/Units/SystemOfUnits.h"

#include "G4Proton.hh"
#include "G4ThreeVector.hh"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
  if (argc != 2)
    {
      std::cerr << "Expected the link bend GMAD file" << std::endl;
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

  const G4int samplerID = link.GetLinkIndex("probe");
  const BDSLinkComponent* linkComponent = link.GetLinkComponent(samplerID);
  const BDSLinkOpaqueBox* bend = linkComponent ? linkComponent->Component() : nullptr;
  if (!bend || !bend->Angled())
    {
      std::cerr << "The parser-built bend was not retained as an angled link component"
                << std::endl;
      return 1;
    }

  const G4RotationMatrix relativeRotation =
    bend->TransformToStart().getRotation().inverse() *
    bend->TransformToOutput().getRotation();
  const G4ThreeVector outputAxis = relativeRotation * G4ThreeVector(0, 0, 1);
  const G4double measuredAngle = std::acos(std::clamp(outputAxis.z(), -1.0, 1.0));
  const G4double expectedAngle = 10 * CLHEP::degree;
  if (std::abs(measuredAngle - expectedAngle) > 1e-12)
    {
      std::cerr << std::setprecision(17)
                << "Wrong output reference rotation: " << measuredAngle
                << ", expected " << expectedAngle << std::endl;
      return 1;
    }

  // The pole-face geometry extends beyond the nominal reference planes. The
  // link must track through that native envelope in addition to its usual
  // navigation and sampler margins.
  if (bend->InputTrackingOffset() <= BDSGlobalConstants::Instance()->LengthSafety() ||
      bend->OutputTrackingOffset() <= 2.5 * BDSSamplerCustom::ChordLength())
    {
      std::cerr << "The bend tracking offsets do not enclose its native geometry"
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

  link.SelectLinkElement("probe");
  link.BeamOn(1);

  const BDSHitsCollectionSamplerLink* hits = link.SamplerHits();
  if (!hits || hits->entries() != 1)
    {
      std::cerr << "Expected one link sampler hit" << std::endl;
      return 1;
    }

  const BDSHitSamplerLink* hit = (*hits)[0];
  const G4double transversePosition = std::hypot(hit->coords.x, hit->coords.y);
  const G4double transverseDirection = std::hypot(hit->coords.xp, hit->coords.yp);
  if (transversePosition > 1e-5 * CLHEP::mm ||
      transverseDirection > 1e-7 ||
      std::abs(hit->coords.z) > 1e-9 * CLHEP::mm)
    {
      std::cerr << std::setprecision(17)
                << "Reference particle was not returned in the bend output frame\n"
                << "  x, y, z: " << hit->coords.x << ", " << hit->coords.y
                << ", " << hit->coords.z << "\n"
                << "  xp, yp: " << hit->coords.xp << ", " << hit->coords.yp
                << "\n  offsets: " << bend->InputTrackingOffset() << ", "
                << bend->OutputTrackingOffset()
                << "\n  track, parent: " << hit->trackID << ", " << hit->parentID
                << std::endl;
      return 1;
    }

  return 0;
}
