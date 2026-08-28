import gc
from types import SimpleNamespace

import beam       # Register GMAD::Beam before retrieving it from the parser.
import bdsimlink  # Register BDSIMLink before retrieving it from the tracker.
import fastlist   # Register the beam-line container.
import numpy as np
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

# TrackXSuite must pass particle momentum in the momentum slot, rather than
# accidentally interpreting it as kinetic energy.
particles = SimpleNamespace(
    x=np.array([0.0]),
    y=np.array([0.0]),
    px=np.array([0.0]),
    py=np.array([0.0]),
    zeta=np.array([0.0]),
    delta=np.array([0.0]),
    chi=np.array([1.0]),
    charge_ratio=np.array([1.0]),
    mass_ratio=np.array([1.0]),
    s=np.array([0.0]),
    pdg_id=np.array([2212], dtype=np.int64),
    particle_id=np.array([0], dtype=np.int64),
    parent_particle_id=np.array([0], dtype=np.int64),
    state=np.array([1], dtype=np.int64),
    at_element=np.array([0], dtype=np.int64),
    at_turn=np.array([0], dtype=np.int64),
)
tracker.TrackXSuite(0, "d1", particles, 100000.0)
assert particles.state[0] == 1
assert abs(particles.delta[0]) < 1e-12

# The link borrows a parser supplied by Python.  Resetting the singleton must
# not delete that parser; Python releases it together with the wrapper below.
tracker.Reset()
del link
del tracker
del bds_parser
gc.collect()
