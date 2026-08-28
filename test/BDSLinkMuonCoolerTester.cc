/*
Beam Delivery Simulation (BDSIM) Copyright (C) Royal Holloway,
University of London 2001 - 2026.

This file is part of BDSIM.

BDSIM is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published
by the Free Software Foundation version 3 of the License.

BDSIM is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with BDSIM.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "BDSBunchSixTrackLink.hh"
#include "BDSHitSamplerLink.hh"
#include "BDSIMLink.hh"
#include "BDSParticleCoordsFull.hh"
#include "BDSParticleDefinition.hh"

#include "CLHEP/Units/SystemOfUnits.h"

#include "G4MuonPlus.hh"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
  if (argc != 2)
    {
      std::cerr << "Usage: BDSLinkMuonCoolerTester <gmad-file>" << std::endl;
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

  const G4double expectedLength = 3 * CLHEP::m;
  const G4double measuredLength = link.GetChordLengthOfLinkElement("mc1");
  if (std::abs(measuredLength - expectedLength) > 1e-9 * CLHEP::mm)
    {
      std::cerr << "Wrong muon-cooler length: " << measuredLength/CLHEP::m
                << " m" << std::endl;
      return 1;
    }

  const G4double momentum = 200 * CLHEP::MeV;
  auto* particle = new BDSParticleDefinition(G4MuonPlus::MuonPlusDefinition(),
                                             0,
                                             0,
                                             momentum,
                                             1);
  const G4double xp0 = 0;
  const G4double zp0 = std::sqrt(1 - xp0*xp0);
  BDSParticleCoordsFull coordinates(0,
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

  link.SelectLinkElement("mc1");
  link.BeamOn(1);

  const BDSHitsCollectionSamplerLink* hits = link.SamplerHits();
  if (!hits || hits->entries() != 1)
    {
      std::cerr << "Expected one muon-cooler sampler hit" << std::endl;
      return 1;
    }

  const BDSHitSamplerLink* hit = (*hits)[0];
  // Reference values from regular BDSIM using the same seed, particle and
  // cooling-channel definition, with its world material set to G4_Galactic
  // to match the isolated link world.
  const G4double expectedX = -2.02447271347 * CLHEP::m;
  const G4double expectedY = -0.355428278446 * CLHEP::m;
  const G4double expectedXP = -0.717630922794;
  const G4double expectedYP = -0.171604260802;
  const G4double expectedEnergy = 0.212289527059 * CLHEP::GeV;
  if (std::abs(hit->coords.x - expectedX) > 10 * CLHEP::um ||
      std::abs(hit->coords.y - expectedY) > 10 * CLHEP::um ||
      std::abs(hit->coords.xp - expectedXP) > 1e-5 ||
      std::abs(hit->coords.yp - expectedYP) > 1e-5 ||
      std::abs(hit->coords.totalEnergy - expectedEnergy) > 10 * CLHEP::keV)
    {
      std::cerr << "Muon-cooler tracking differs from regular BDSIM:\n"
                << "  x=" << hit->coords.x / CLHEP::m
                << " m, y=" << hit->coords.y / CLHEP::m << " m\n"
                << "  xp=" << hit->coords.xp << ", yp=" << hit->coords.yp << "\n"
                << "  energy=" << hit->coords.totalEnergy / CLHEP::GeV << " GeV"
                << std::endl;
      return 1;
    }

  return 0;
}
