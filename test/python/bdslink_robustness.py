import gc

import beam       # Register GMAD::Beam before retrieving it from the parser.
import bdsimlink  # Register BDSIMLink before retrieving it from the tracker.
import fastlist   # Register the beam-line container.
import parser     # Register the GMAD::Parser base class.
from bdslinktrackerinterface import BDSLinkTrackerInterface
from bdsparser import BDSParser
from element import Element
from elementtype import ElementType


bds_parser = BDSParser()
beam_definition = bds_parser.GetGlobal_Beam()
beam_definition["particle"] = "proton"
beam_definition["kineticEnergy"] = 100.0

marker = Element()
marker.name = "m0"
marker.type = ElementType.MARKER
bds_parser.GetBeamline().push_back(marker, False, "element")

drift = Element()
drift.name = "d1"
drift.type = ElementType.DRIFT
drift["l"] = 1.0
drift["aper1"] = 0.05
bds_parser.GetBeamline().push_back(drift, False, "element")

tracker = BDSLinkTrackerInterface.GetInstance(
    bds_parser,
    referenceParticlePDG=2212,
    referenceKineticEnergy=100.0,
    batchMode=True,
)
link = tracker.GetBDSIMLink()

assert link.GetLinkIndex("m0") == -1
assert link.GetLinkIndex("d1") == 0
assert link.GetChordLengthOfLinkElement(1) == -1.0
assert link.GetChordLengthOfLinkElement("missing") == -1.0
assert link.GetArcLengthOfLinkElement("missing") == -1.0

# The link borrows a parser supplied by Python.  Resetting the singleton must
# not delete that parser; Python releases it together with the wrapper below.
tracker.Reset()
del link
del tracker
del bds_parser
gc.collect()
