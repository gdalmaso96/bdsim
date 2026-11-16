/*
Beam Delivery Simulation (BDSIM) Copyright (C) BDSIM Collaboration, 2001 - 2026.

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

#include "BDSIMLink.hh"
#include "BDSLinkBunch.hh"

#include "G4LogicalVolume.hh"
#include "G4PhysicalVolumeStore.hh"
#include "G4VPhysicalVolume.hh"

#include "CLHEP/Units/SystemOfUnits.h"

#include <cmath>

#include <iostream>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
  if (argc != 4 && argc != 6)
    {
      std::cerr << "Usage: BDSLinkPlacementTester <gmad-file> "
                << "<reference-element> <placement-volume> "
                << "[expected-local-x-m expected-local-z-m]" << std::endl;
      return 1;
    }

  const std::string inputFile = argv[1];
  const std::string referenceElement = argv[2];
  const std::string placementVolume = argv[3];

  BDSLinkBunch bunch;
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
    {argumentPointers.push_back(&argument[0]);}

  link.Initialise((G4int)argumentPointers.size(), argumentPointers.data(), false);
  if (!link.Initialised())
    {
      std::cerr << "BDSLink failed to initialise" << std::endl;
      return 1;
    }

  if (link.GetLinkIndex(referenceElement) < 0)
    {
      std::cerr << "Reference element was not registered with BDSLink" << std::endl;
      return 1;
    }

  G4VPhysicalVolume* placement =
    G4PhysicalVolumeStore::GetInstance()->GetVolume(placementVolume, false);
  if (!placement)
    {
      std::cerr << "Placement volume was not constructed" << std::endl;
      return 1;
    }

  const G4String motherName = placement->GetMotherLogical()->GetName();
  if (motherName.find(referenceElement) == std::string::npos)
    {
      std::cerr << "Placement mother is \"" << motherName
                << "\", expected the opaque box of \"" << referenceElement
                << "\"" << std::endl;
      return 1;
    }

  if (argc == 6)
    {
      const G4ThreeVector translation = placement->GetObjectTranslation();
      const G4double expectedX = std::stod(argv[4]) * CLHEP::m;
      const G4double expectedZ = std::stod(argv[5]) * CLHEP::m;
      if (std::abs(translation.x() - expectedX) > 1 * CLHEP::nm ||
          std::abs(translation.z() - expectedZ) > 1 * CLHEP::nm)
        {
          std::cerr << "Placement has local translation " << translation
                    << ", expected x=" << expectedX << " and z=" << expectedZ
                    << std::endl;
          return 1;
        }
    }

  return 0;
}
