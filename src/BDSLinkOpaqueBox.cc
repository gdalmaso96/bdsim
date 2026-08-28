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
#include "BDSAcceleratorComponent.hh"
#include "BDSApertureInfo.hh"
#include "BDSApertureType.hh"
#include "BDSBeamline.hh"
#include "BDSBeamlineElement.hh"
#include "BDSColours.hh"
#include "BDSDebug.hh"
#include "BDSException.hh"
#include "BDSExtent.hh"
#include "BDSExtentGlobal.hh"
#include "BDSGlobalConstants.hh"
#include "BDSLine.hh"
#include "BDSLinkOpaqueBox.hh"
#include "BDSMaterials.hh"
#include "BDSSamplerCustom.hh"
#include "BDSSamplerPlacementRecord.hh"
#include "BDSSamplerPlane.hh"
#include "BDSSamplerRegistry.hh"
#include "BDSSDManager.hh"
#include "BDSSDSamplerLink.hh"
#include "BDSTiltOffset.hh"
#include "BDSUtilities.hh"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4RotationMatrix.hh"
#include "G4PVPlacement.hh"
#include "G4SubtractionSolid.hh"
#include "G4ThreeVector.hh"
#include "G4Types.hh"
#include "G4UserLimits.hh"
#include "G4VisAttributes.hh"
#include "G4TwoVector.hh"

#include "CLHEP/Units/SystemOfUnits.h"
#include <algorithm>
#include <cmath>
#include <limits>

BDSLinkOpaqueBox::BDSLinkOpaqueBox(BDSAcceleratorComponent* acceleratorComponentIn,
				   BDSTiltOffset* tiltOffsetIn,
				   G4double outputSamplerRadiusIn,
				   G4double inputTrackingOffsetIn,
				   G4double outputTrackingOffsetIn):
  BDSGeometryComponent(nullptr, nullptr),
  component(acceleratorComponentIn),
  componentBeamline(nullptr),
  outputSamplerRadius(outputSamplerRadiusIn),
  inputTrackingOffset(inputTrackingOffsetIn),
  outputTrackingOffset(outputTrackingOffsetIn),
  arcLength(component ? component->GetArcLength() : 0),
  chordLength(component ? component->GetChordLength() : 0),
  angle(component ? component->GetAngle() : 0),
  nativeToOpaque(G4Transform3D::Identity),
  sampler(nullptr)
{
  if (inputTrackingOffset < 0 || outputTrackingOffset < 0)
    {throw BDSException(__METHOD_NAME__, "link tracking offsets must be non-negative");}

  if (tiltOffsetIn->HasFiniteTilt() && BDS::IsFinite(component->GetAngle()))
    {throw BDSException(__METHOD_NAME__, "finite tilt with angled component unsupported.");}

  BDSExtent extent;
  if (dynamic_cast<BDSLine*>(component))
    {
      componentBeamline = new BDSBeamline();
      componentBeamline->AddComponent(component,
                                      new BDSTiltOffset(tiltOffsetIn->GetXOffset(),
                                                        tiltOffsetIn->GetYOffset(),
                                                        tiltOffsetIn->GetTilt()));
      if (componentBeamline->size() == 0)
        {throw BDSException(__METHOD_NAME__, "empty internal link beamline");}

      const BDSBeamlineElement* nominalFirst = componentBeamline->GetFirstItem();
      const BDSBeamlineElement* nominalLast  = componentBeamline->GetLastItem();
      arcLength = 0;
      chordLength = 0;
      angle = 0;
      for (const auto* nominal : *componentBeamline)
        {
          arcLength += nominal->GetArcLength();
          chordLength += nominal->GetChordLength();
          angle += nominal->GetAngle();
        }
      transformToStart = G4Transform3D(*nominalFirst->GetReferenceRotationStart(),
                                      nominalFirst->GetReferencePositionStart());
      transformToOutput = G4Transform3D(*nominalLast->GetReferenceRotationEnd(),
                                       nominalLast->GetReferencePositionEnd());

      const BDSExtentGlobal extentGlobal = componentBeamline->GetExtentGlobal();
      extent = BDSExtent(extentGlobal.XNegGlobal(), extentGlobal.XPosGlobal(),
                         extentGlobal.YNegGlobal(), extentGlobal.YPosGlobal(),
                         extentGlobal.ZNegGlobal(), extentGlobal.ZPosGlobal());
      for (const auto* element : *componentBeamline)
        {
          outputSamplerRadius = std::max(
            outputSamplerRadius,
            element->GetExtent().TransverseBoundingRadius());
        }
      offsetToStart = transformToStart.getTranslation();
    }
  else
    {
      extent = component->GetExtent().TiltOffset(tiltOffsetIn);
      G4RotationMatrix identity;
      offsetToStart = G4ThreeVector(0, 0, -0.5*chordLength);
      transformToStart = G4Transform3D(identity, offsetToStart);
      transformToOutput = G4Transform3D(identity,
                                        G4ThreeVector(0, 0, 0.5*chordLength));

      // Keep a native one-element reference beamline for the field-coordinate
      // parallel world.  Its native start is z=0, whereas the existing link
      // wrapper keeps a straight component centred at z=0.
      componentBeamline = new BDSBeamline();
      componentBeamline->AddComponent(component,
                                      new BDSTiltOffset(tiltOffsetIn->GetXOffset(),
                                                        tiltOffsetIn->GetYOffset(),
                                                        tiltOffsetIn->GetTilt()));
      nativeToOpaque = G4Transform3D(identity,
                                    G4ThreeVector(0, 0, -0.5*chordLength));
    }
  const G4double gap                = 10 * CLHEP::cm;
  const G4double opaqueBoxThickness = 10 * CLHEP::mm;
  G4String name = component->GetName();

  G4double mx = extent.MaximumX();
  G4double my = extent.MaximumY();
  G4double mz = std::max(extent.MaximumZ(), 0.5*component->GetChordLength());

  // The output sampler follows the outgoing reference frame.  Include its
  // transformed cylindrical envelope when sizing the link container; this is
  // important for bends, where its transverse radius also projects onto z.
  const G4double samplerHalfLength = 0.5 * BDSSamplerCustom::ChordLength();
  const G4double samplerCentreOffset = outputTrackingOffset - samplerHalfLength;
  const G4Transform3D outputSamplerTransform = transformToOutput *
    G4Transform3D(G4RotationMatrix(), G4ThreeVector(0, 0, samplerCentreOffset));
  const G4RotationMatrix& outputSamplerRotation = outputSamplerTransform.getRotation();
  const G4ThreeVector& outputSamplerCentre = outputSamplerTransform.getTranslation();
  const G4double samplerHalfX = outputSamplerRadius *
    std::hypot(outputSamplerRotation.xx(), outputSamplerRotation.xy()) +
    samplerHalfLength * std::abs(outputSamplerRotation.xz());
  const G4double samplerHalfY = outputSamplerRadius *
    std::hypot(outputSamplerRotation.yx(), outputSamplerRotation.yy()) +
    samplerHalfLength * std::abs(outputSamplerRotation.yz());
  const G4double samplerHalfZ = outputSamplerRadius *
    std::hypot(outputSamplerRotation.zx(), outputSamplerRotation.zy()) +
    samplerHalfLength * std::abs(outputSamplerRotation.zz());
  mx = std::max(mx, std::abs(outputSamplerCentre.x()) + samplerHalfX);
  my = std::max(my, std::abs(outputSamplerCentre.y()) + samplerHalfY);
  mz = std::max(mz, std::abs(outputSamplerCentre.z()) + samplerHalfZ);

  G4double mr = std::max({mx, my, outputSamplerRadius});
  G4Box* terminatorBoxOuter = new G4Box(name + "_terminator_box_outer_solid",
					mr + gap + opaqueBoxThickness,
					mr + gap + opaqueBoxThickness,
					mz + gap + opaqueBoxThickness);
  RegisterSolid(terminatorBoxOuter);
  G4Box* terminatorBoxInner = new G4Box(name + "_terminator_box_inner_solid",
					mr + gap,
					mr + gap,
					mz + gap);
  RegisterSolid(terminatorBoxInner);
  G4SubtractionSolid* opaqueBox = new G4SubtractionSolid(name + "_opaque_box_solid",
							 terminatorBoxOuter,
							 terminatorBoxInner);
  RegisterSolid(opaqueBox);
  G4LogicalVolume* opaqueBoxLV = new G4LogicalVolume(opaqueBox,
						     BDSMaterials::Instance()->GetMaterial("G4_Galactic"),
						     name + "_opaque_box_lv");
  RegisterLogicalVolume(opaqueBoxLV);

  G4UserLimits* termUL = new G4UserLimits();
  termUL->SetUserMinEkine(std::numeric_limits<double>::max());
  RegisterUserLimits(termUL);
  opaqueBoxLV->SetUserLimits(termUL);
  
  G4VisAttributes* obVis = new G4VisAttributes(*BDSColours::Instance()->GetColour("opaquebox"));
  obVis->SetVisibility(true);
  opaqueBoxLV->SetVisAttributes(obVis);
  RegisterVisAttributes(obVis);
  
  G4double ls = BDSGlobalConstants::Instance()->LengthSafetyLarge();
  G4double margin = gap + opaqueBoxThickness + ls;
  G4double xsize = mr + margin;
  G4double ysize = mr + margin;
  G4double zsize = mz + margin;
  containerSolid = new G4Box(name + "_opaque_box_vacuum_solid",
			     xsize,
			     ysize,
			     zsize);
  
  containerLogicalVolume = new G4LogicalVolume(containerSolid,
					       BDSMaterials::Instance()->GetMaterial("G4_Galactic"),
					       name + "_container_lv");
  containerLogicalVolume->SetVisAttributes(BDSGlobalConstants::Instance()->ContainerVisAttr());

  // auto boxPlacement = 
  new G4PVPlacement(nullptr,
		    G4ThreeVector(),
		    opaqueBoxLV,
		    name + "_opaque_box_pv",
		    containerLogicalVolume,
		    false,
		    1,
		    true);

  for (const auto* element : *componentBeamline)
    {
      auto* componentLV = element->GetAcceleratorComponent()->GetContainerLogicalVolume();
      if (componentLV)
        {
          new G4PVPlacement(nativeToOpaque * *element->GetPlacementTransform(),
                            componentLV,
                            element->GetPlacementName() + "_pv",
                            containerLogicalVolume,
                            false,
                            element->GetCopyNo(),
                            true);
        }
    }
  
  outerExtent = BDSExtent(xsize, ysize, zsize);

}

BDSLinkOpaqueBox::~BDSLinkOpaqueBox()
{
  delete sampler;
  delete componentBeamline;
}

void BDSLinkOpaqueBox::AppendFieldReferenceElements(
  BDSBeamline*         target,
  const G4Transform3D& opaqueToGlobal,
  G4double&            referenceS,
  G4int&               referenceIndex) const
{
  if (!target || !componentBeamline)
    {return;}

  for (const auto* native : *componentBeamline)
    {
      auto frame = [this, &opaqueToGlobal](const G4RotationMatrix* rotation,
                                           const G4ThreeVector& position)
      {
        return opaqueToGlobal * nativeToOpaque *
          G4Transform3D(*rotation, position);
      };
      const G4Transform3D placementStart =
        frame(native->GetRotationStart(), native->GetPositionStart());
      const G4Transform3D placementMiddle =
        frame(native->GetRotationMiddle(), native->GetPositionMiddle());
      const G4Transform3D placementEnd =
        frame(native->GetRotationEnd(), native->GetPositionEnd());
      const G4Transform3D referenceStart =
        frame(native->GetReferenceRotationStart(), native->GetReferencePositionStart());
      const G4Transform3D referenceMiddle =
        frame(native->GetReferenceRotationMiddle(), native->GetReferencePositionMiddle());
      const G4Transform3D referenceEnd =
        frame(native->GetReferenceRotationEnd(), native->GetReferencePositionEnd());
      const G4double nativeArcLength = native->GetArcLength();
      const BDSTiltOffset* nativeTilt = native->GetTiltOffset();
      BDSTiltOffset* tiltCopy = nativeTilt ? new BDSTiltOffset(*nativeTilt) : nullptr;

      target->AddBeamlineElement(new BDSBeamlineElement(
        native->GetAcceleratorComponent(),
        placementStart.getTranslation(),
        placementMiddle.getTranslation(),
        placementEnd.getTranslation(),
        new G4RotationMatrix(placementStart.getRotation()),
        new G4RotationMatrix(placementMiddle.getRotation()),
        new G4RotationMatrix(placementEnd.getRotation()),
        referenceStart.getTranslation(),
        referenceMiddle.getTranslation(),
        referenceEnd.getTranslation(),
        new G4RotationMatrix(referenceStart.getRotation()),
        new G4RotationMatrix(referenceMiddle.getRotation()),
        new G4RotationMatrix(referenceEnd.getRotation()),
        referenceS,
        referenceS + 0.5*nativeArcLength,
        referenceS + nativeArcLength,
        0,
        0,
        0,
        tiltCopy,
        nullptr,
        referenceIndex));
      referenceS += nativeArcLength;
      referenceIndex++;
    }
}

G4int BDSLinkOpaqueBox::PlaceOutputSampler()
{  
  G4String samplerName = component->GetName() + "_out";
  BDSApertureType apt = BDSApertureType::circular;
  BDSApertureInfo ap = BDSApertureInfo(apt, outputSamplerRadius, 0, 0, 0);
  sampler = new BDSSamplerCustom(samplerName, ap);
  sampler->GetContainerLogicalVolume()->SetSensitiveDetector(BDSSDManager::Instance()->SamplerLink());
  sampler->MakeMaterialValidForUseInMassWorld();
  // outputTrackingOffset measures to the downstream hit surface. Place the
  // sampler centre half a sampler chord before it in the BDSIM output frame.
  const G4double centreOffset = outputTrackingOffset -
                                0.5*BDSSamplerCustom::ChordLength();
  const G4Transform3D samplerTransform = transformToOutput *
    G4Transform3D(G4RotationMatrix(), G4ThreeVector(0, 0, centreOffset));
  BDSSamplerPlacementRecord info(samplerName, sampler, samplerTransform);
  
  G4int samplerID = BDSSamplerRegistry::Instance()->RegisterSampler(info);
  new G4PVPlacement(samplerTransform,
		    sampler->GetContainerLogicalVolume(),
		    samplerName + "_pv",
		    containerLogicalVolume,
		    false,
		    samplerID,
		    true);
  return samplerID;
}
