import gc
import sys

import beam       # Register GMAD::Beam before retrieving it from the parser.
import bdsbunch   # Register the base class before BDSLinkBunch.
import bdsimlink  # Register BDSIMLink before retrieving it from the tracker.
import bdslinkbunch
import fastlist   # Register the beam-line container.
import parser     # Register the GMAD::Parser base class.
from bdsimlink import BDSIMLink
from bdslinkbunch import BDSLinkBunch
from bdslinktrackerinterface import BDSLinkTrackerInterface
from bdsparser import BDSParser
from element import Element
from elementtype import ElementType


def make_parser():
    bds_parser = BDSParser()
    beam_definition = bds_parser.GetBeam()
    beam_definition["particleName"] = "proton"
    beam_definition["kineticEnergy"] = 100.0

    collimator = Element()
    collimator.name = "probe"
    collimator.type = ElementType.RCOL
    collimator["l"] = 0.2
    collimator["material"] = "G4_Fe"
    collimator["xsize"] = 0.01
    collimator["ysize"] = 0.01
    collimator["aper1"] = 0.05
    bds_parser.GetBeamline().push_back(collimator, False, "element")
    return bds_parser


def assert_parser_available(bds_parser):
    beamline = bds_parser.GetBeamline()
    assert beamline.size() == 1
    assert next(iter(beamline)).name == "probe"


def test_tracker_interface():
    bds_parser = make_parser()
    tracker = BDSLinkTrackerInterface.GetInstance(
        bds_parser,
        referenceParticlePDG=2212,
        referenceKineticEnergy=100.0,
        batchMode=True,
    )
    link = tracker.GetBDSIMLink()
    assert link.GetLinkIndex("probe") == 0

    # Reset ends the BDSIMLink session but must not delete the parser supplied
    # by Python.
    tracker.Reset()
    assert_parser_available(bds_parser)
    del link
    del tracker
    del bds_parser
    gc.collect()


def test_direct_interface():
    bds_parser = make_parser()
    bunch = BDSLinkBunch()
    link = BDSIMLink(bunch, bds_parser)
    link.Initialise(["bdsim", "--output=None", "--batch", "--seed=1234"])
    assert link.GetLinkIndex("probe") == 0

    del link
    gc.collect()
    assert_parser_available(bds_parser)
    del bunch
    del bds_parser
    gc.collect()


if sys.argv[1] == "tracker":
    test_tracker_interface()
elif sys.argv[1] == "direct":
    test_direct_interface()
else:
    raise ValueError(f"Unknown test mode: {sys.argv[1]}")
