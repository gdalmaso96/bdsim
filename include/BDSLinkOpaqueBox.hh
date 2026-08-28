/* 
Beam Delivery Simulation (BDSIM) Copyright (C) Royal Holloway, 
University of London 2001 - 2024.

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
#ifndef BDSLINKOPAQUEBOX_H
#define BDSLINKOPAQUEBOX_H
#include "BDSAcceleratorComponent.hh"
#include "BDSUtilities.hh"

#include "G4ThreeVector.hh"
#include "G4Transform3D.hh"
#include "G4Types.hh"

#include "BDSGeometryComponent.hh"

#include <utility>

class BDSAcceleratorComponent;
class BDSBeamline;
class BDSSamplerCustom;

/**
 * @brief Wrapper box for an accelerator component.
 * 
 * @author Laurie Nevay
 */

class BDSLinkOpaqueBox: public BDSGeometryComponent
{
public:
  /// inputTrackingOffsetIn is the distance upstream of the nominal input plane
  /// where particles are injected into Geant4.
  BDSLinkOpaqueBox(BDSAcceleratorComponent* acceleratorComponentIn,
                   BDSTiltOffset* tiltOffsetIn,
                   G4double outputSamplerRadiusIn,
                   G4double inputTrackingOffsetIn,
                   G4double outputTrackingOffsetIn,
                   BDSAcceleratorComponent* inputGuardIn = nullptr,
                   BDSAcceleratorComponent* outputGuardIn = nullptr);
  virtual ~BDSLinkOpaqueBox();

  /// Default constructor
  BDSLinkOpaqueBox() = delete;

  /// Copy constructor
  BDSLinkOpaqueBox(const BDSLinkOpaqueBox &other) = delete;
  /// Copy assignment operator
  BDSLinkOpaqueBox& operator=(const BDSLinkOpaqueBox &other) = delete;

  inline const G4ThreeVector& OffsetToStart()    const {return offsetToStart;}
  inline const G4Transform3D& TransformToStart() const {return transformToStart;}
  inline const G4Transform3D& TransformToOutput() const {return transformToOutput;}
  /// Add the native component reference frames to the link field-coordinate
  /// beamline at this wrapper's world placement.
  void AppendFieldReferenceElements(BDSBeamline*         target,
                                    const G4Transform3D& opaqueToGlobal,
                                    G4double&            referenceS,
                                    G4int&               referenceIndex) const;

  /// Clearance required for the native geometry on either side of its
  /// nominal input and output reference planes.
  static std::pair<G4double, G4double> FaceClearances(
    BDSAcceleratorComponent* component,
    const BDSTiltOffset*     tiltOffset);

  /// Place the output sampler
  G4int PlaceOutputSampler();
  
  /// @{ Accessor
  G4double ArcLength()   const {return arcLength;}
  G4double ChordLength() const {return chordLength;}
  G4bool   Angled()      const {return BDS::IsFinite(angle);}
  G4String LinkName()    const {return component ? component->GetName() : "unknown";}
  G4double InputTrackingOffset() const {return inputTrackingOffset;}
  G4double OutputTrackingOffset() const {return outputTrackingOffset;}
  /// @}

private:
  BDSAcceleratorComponent* component;
  BDSBeamline*              componentBeamline;
  G4double                 outputSamplerRadius;
  G4double                 inputTrackingOffset;
  G4double                 outputTrackingOffset;
  G4double                 arcLength;
  G4double                 chordLength;
  G4double                 angle;
  G4ThreeVector            offsetToStart;
  G4Transform3D            transformToStart;
  G4Transform3D            transformToOutput;
  G4Transform3D            nativeToOpaque;
  BDSSamplerCustom*        sampler;
};

#endif
