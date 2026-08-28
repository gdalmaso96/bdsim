import fastlist  # Register FastList specialisations before returning one.
import parser    # Register the GMAD::Parser base class before BDSParser.
from bdsparser import BDSParser
from coolingchannel import CoolingChannel  # Register the returned definition type.
from element import Element
from elementtype import ElementType


bds_parser = BDSParser()

cooling = bds_parser.GetGlobal_CoolingChannel()
cooling.clear()
cooling["name"] = "cooling"
cooling["nCells"] = 1
cooling["cellLengthZ"] = 1.0
cooling["magneticFieldModel"] = "solenoidblock"
bds_parser.Add_CoolingChannel()

element = Element()
element.name = "cooler"
element.type = ElementType.MUONCOOLER
element["l"] = 1.0
element["horizontalWidth"] = 0.8
element["coolingDefinition"] = "cooling"
element["aper1"] = 0.05
bds_parser.GetBeamline().push_back(element, False, "element")

registered = bds_parser.GetCoolingChannel("cooling")
assert registered.nCells == 1
assert registered.cellLengthZ == 1.0

beamline = list(bds_parser.GetBeamline())
assert len(beamline) == 1
assert beamline[0].type == ElementType.MUONCOOLER
assert beamline[0].get_value("coolingDefinition") == "cooling"
