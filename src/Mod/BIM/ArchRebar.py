# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *                                                                         *
# *   Copyright (c) 2013 Yorik van Havre <yorik@uncreated.net>              *
# *                                                                         *
# *   This file is part of FreeCAD.                                         *
# *                                                                         *
# *   FreeCAD is free software: you can redistribute it and/or modify it    *
# *   under the terms of the GNU Lesser General Public License as           *
# *   published by the Free Software Foundation, either version 2.1 of the  *
# *   License, or (at your option) any later version.                       *
# *                                                                         *
# *   FreeCAD is distributed in the hope that it will be useful, but        *
# *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
# *   Lesser General Public License for more details.                       *
# *                                                                         *
# *   You should have received a copy of the GNU Lesser General Public      *
# *   License along with FreeCAD. If not, see                               *
# *   <https://www.gnu.org/licenses/>.                                      *
# *                                                                         *
# ***************************************************************************

# Modified Amritpal Singh <amrit3701@gmail.com> on 07-07-2017

__title__  = "FreeCAD Rebar"
__author__ = "Yorik van Havre"
__url__    = "https://www.freecad.org"

## @package ArchRebar
#  \ingroup ARCH
#  \brief The Rebar object and tools
#
#  This module provides tools to build Rebar objects.
#  Rebars (or Reinforcing Bars) are metallic bars placed
#  inside concrete structures to reinforce them.

import FreeCAD
import ArchCommands
import ArchIFC
import ArchComponent
import Draft
import DraftVecUtils

from draftutils import params

if FreeCAD.GuiUp:
    from PySide.QtCore import QT_TRANSLATE_NOOP
    import FreeCADGui
    from draftutils.translate import translate
    # TODO: check if this import is still needed, and if so, whether
    # it can be moved made conditional on the GUI being loaded
    # for Rebar addon compatibility
    from bimcommands import BimRebar
    _CommandRebar = BimRebar.Arch_Rebar
else:
    # \cond
    def translate(ctxt,txt):
        return txt
    def QT_TRANSLATE_NOOP(ctxt,txt):
        return txt
    # \endcond


class _Rebar(ArchComponent.Component):

    "A parametric reinforcement bar (rebar) object"

    def __init__(self,obj):

        ArchComponent.Component.__init__(self,obj)
        self.Type = "Rebar"
        self.setProperties(obj)
        obj.IfcType = "Reinforcing Bar"

    def setProperties(self,obj):

        pl = obj.PropertiesList
        if not "Diameter" in pl:
            obj.addProperty("App::PropertyLength","Diameter","Rebar",QT_TRANSLATE_NOOP("App::Property","The diameter of the bar"), locked=True)
        if not "OffsetStart" in pl:
            obj.addProperty("App::PropertyDistance","OffsetStart","Rebar",QT_TRANSLATE_NOOP("App::Property","The distance between the border of the beam and the first bar (concrete cover)."), locked=True)
        if not "OffsetEnd" in pl:
            obj.addProperty("App::PropertyDistance","OffsetEnd","Rebar",QT_TRANSLATE_NOOP("App::Property","The distance between the border of the beam and the last bar (concrete cover)."), locked=True)
        if not "Amount" in pl:
            obj.addProperty("App::PropertyInteger","Amount","Rebar",QT_TRANSLATE_NOOP("App::Property","The amount of bars"), locked=True)
        if not "Spacing" in pl:
            obj.addProperty("App::PropertyLength","Spacing","Rebar",QT_TRANSLATE_NOOP("App::Property","The spacing between the bars"), locked=True)
            obj.setEditorMode("Spacing", 1)
        if not "Distance" in pl:
            obj.addProperty("App::PropertyLength","Distance","Rebar",QT_TRANSLATE_NOOP("App::Property","The total distance to span the rebars over. Keep 0 to automatically use the host shape size."), locked=True)
        if not "Direction" in pl:
            obj.addProperty("App::PropertyVector","Direction","Rebar",QT_TRANSLATE_NOOP("App::Property","The direction to use to spread the bars. Keep (0,0,0) for automatic direction."), locked=True)
        if not "Rounding" in pl:
            obj.addProperty("App::PropertyFloat","Rounding","Rebar",QT_TRANSLATE_NOOP("App::Property","The fillet to apply to the angle of the base profile. This value is multiplied by the bar diameter."), locked=True)
        if not "PlacementList" in pl:
            obj.addProperty("App::PropertyPlacementList","PlacementList","Rebar",QT_TRANSLATE_NOOP("App::Property","List of placement of all the bars"), locked=True)
        if not "Host" in pl:
            obj.addProperty("App::PropertyLink","Host","Rebar",QT_TRANSLATE_NOOP("App::Property","The structure object that hosts this rebar"), locked=True)
        if not "CustomSpacing" in pl:
            obj.addProperty("App::PropertyString", "CustomSpacing", "Rebar", QT_TRANSLATE_NOOP("App::Property","The custom spacing of rebar"), locked=True)
        if not "Length" in pl:
            obj.addProperty("App::PropertyDistance", "Length", "Rebar", QT_TRANSLATE_NOOP("App::Property","Length of a single rebar"), locked=True)
            obj.setEditorMode("Length", 1)
        if not "TotalLength" in pl:
            obj.addProperty("App::PropertyDistance", "TotalLength", "Rebar", QT_TRANSLATE_NOOP("App::Property","Total length of all rebars"), locked=True)
            obj.setEditorMode("TotalLength", 1)
        if not "Mark" in pl:
            obj.addProperty(
                "App::PropertyString",
                "Mark",
                "Rebar",
                QT_TRANSLATE_NOOP("App::Property", "The rebar mark"),
                locked=True,
            )

    def onDocumentRestored(self,obj):

        ArchComponent.Component.onDocumentRestored(self,obj)
        self.setProperties(obj)

    def loads(self,state):

        self.Type = "Rebar"

    def getBaseAndAxis(self,wire):

        "returns a base point and orientation axis from the base wire"
        import DraftGeomUtils
        if wire:
            e = wire.Edges[0]
            #v = DraftGeomUtils.vec(e).normalize()
            v = e.tangentAt(e.FirstParameter)
            return e.Vertexes[0].Point,v
        if obj.Base:
            if obj.Base.Shape:
                if obj.Base.Shape.Wires:
                    e = obj.Base.Shape.Wires[0].Edges[0]
                    #v = DraftGeomUtils.vec(e).normalize()
                    v = e.tangentAt(e.FirstParameter)
                    return e.Vertexes[0].Point,v
        return None,None

    def getRebarData(self,obj):

        #if not obj.Host:
        #    return
        #if Draft.getType(obj.Host) != "Structure":
        #    return
        #if not obj.Host.Shape:
        #    return
        if not obj.Base:
            return
        if not obj.Base.Shape:
            return
        if not obj.Base.Shape.Wires:
            return
        if not obj.Diameter.Value:
            return
        if not obj.Amount:
            return
        father = None
        if obj.Host:
            father = obj.Host
        wire = obj.Base.Shape.Wires[0]
        axis = None
        if Draft.getType(obj.Base) == "Wire": # Draft Wires can have "wrong" placement
            import DraftGeomUtils
            axis = DraftGeomUtils.getNormal(obj.Base.Shape)
        if not axis:
            axis = obj.Base.Placement.Rotation.multVec(FreeCAD.Vector(0,0,-1))
        if hasattr(obj,"Direction"):
            if not DraftVecUtils.isNull(obj.Direction):
                axis = FreeCAD.Vector(obj.Direction)
                axis.normalize()
        size = 0
        if father:
            size = (ArchCommands.projectToVector(father.Shape.copy(),axis)).Length
        if hasattr(obj,"Distance"):
            if obj.Distance.Value:
                size = obj.Distance.Value
        if hasattr(obj,"Rounding"):
            if obj.Rounding:
                radius = obj.Rounding * obj.Diameter.Value
                import DraftGeomUtils
                wire = DraftGeomUtils.filletWire(wire,radius)
        wires = []
        if obj.Amount == 1:
            if size and father:
                offset = DraftVecUtils.scaleTo(axis,size/2)
            else:
                offset = FreeCAD.Vector()
            wire.translate(offset)
            wires.append(wire)
        else:
            if obj.OffsetStart.Value:
                baseoffset = DraftVecUtils.scaleTo(axis,obj.OffsetStart.Value)
            else:
                baseoffset = None
            if hasattr(obj, "RebarShape") and obj.RebarShape == "Stirrup":
                interval = size - (obj.OffsetStart.Value + obj.OffsetEnd.Value + obj.Diameter.Value)
            else:
                interval = size - (obj.OffsetStart.Value + obj.OffsetEnd.Value)
            interval = interval / (obj.Amount - 1)
            vinterval = DraftVecUtils.scaleTo(axis,interval)
            for i in range(obj.Amount):
                if i == 0:
                    if baseoffset:
                        wire.translate(baseoffset)
                    wires.append(wire)
                else:
                    wire = wire.copy()
                    wire.translate(vinterval)
                    wires.append(wire)
        return [wires,obj.Diameter.Value/2]

    def onChanged(self,obj,prop):
        if prop == "IfcType":
            root = ArchIFC.IfcProduct()
            root.setupIfcAttributes(obj)
            root.setupIfcComplexAttributes(obj)
        elif prop == "Host":
            if hasattr(obj,"Host"):
                if obj.Host:
                    # mark host to recompute so it can detect this object
                    obj.Host.touch()

    def execute(self,obj):

        if self.clone(obj):
            return
        if not self.ensureBase(obj):
            return
        if not obj.Base:
            # let pass without error so that object can receive a shape directly
            #FreeCAD.Console.PrintError(
            #    "No Base, return without a rebar shape for {}.\n"
            #    .format(obj.Name)
            #)
            return
        if not hasattr(obj.Base,"Shape") or (not obj.Base.Shape) or obj.Base.Shape.isNull():
            FreeCAD.Console.PrintError(
                "No Shape in Base, return without a rebar shape for {}.\n"
                .format(obj.Name)
            )
            return
        if obj.Base.Shape.Faces:
            FreeCAD.Console.PrintError(
                "Faces in Shape of Base, return without a rebar shape for {}.\n"
                .format(obj.Name)
            )
            return
        if not obj.Base.Shape.Edges:
            FreeCAD.Console.PrintError(
                "No Edges in Shape of Base, return without a rebar shape for {}.\n"
                .format(obj.Name)
            )
            return
        if not obj.Diameter.Value:
            FreeCAD.Console.PrintError(
                "No Diameter Value, return without a rebar shape for {}.\n"
                .format(obj.Name)
            )
            return
        if not obj.Amount:
            FreeCAD.Console.PrintError(
                "No Amount, return without a rebar shape for {}.\n"
                .format(obj.Name)
            )
            return
        father = obj.Host
        fathershape = None
        if not father:
            # support for old-style rebars
            if obj.InList:
                if hasattr(obj.InList[0],"Armatures"):
                    if obj in obj.InList[0].Armatures:
                        father = obj.InList[0]
        if father:
            if hasattr(father,'Shape'):
                fathershape = father.Shape

        import Part
        # corner cases:
        #    compound from more Wires
        #    compound without Wires but with multiple Edges
        # Does they make sense? If yes handle them.
        # Does it makes sense to handle Shapes with Faces or even Solids?
        if not obj.Base.Shape.Wires and len(obj.Base.Shape.Edges) == 1:
            wire = Part.Wire(obj.Base.Shape.Edges[0])
        else:
            wire = obj.Base.Shape.Wires[0]
        if hasattr(obj,"Rounding"):
            #print(obj.Rounding)
            if obj.Rounding:
                radius = obj.Rounding * obj.Diameter.Value
                from DraftGeomUtils import filletWire
                wire = filletWire(wire,radius)
        bpoint, bvec = self.getBaseAndAxis(wire)
        if not bpoint:
            return
        axis = obj.Base.Placement.Rotation.multVec(FreeCAD.Vector(0,0,-1))
        if fathershape:
            size = (ArchCommands.projectToVector(fathershape.copy(),axis)).Length
        else:
            size = 1
        if hasattr(obj,"Direction"):
            if not DraftVecUtils.isNull(obj.Direction):
                axis = FreeCAD.Vector(obj.Direction)
                axis.normalize()
                if fathershape:
                    size = (ArchCommands.projectToVector(fathershape.copy(),axis)).Length
                else:
                    size = 1
        if hasattr(obj,"Distance"):
            if obj.Distance.Value:
                size = obj.Distance.Value
        spacinglist = None
        if hasattr(obj, "CustomSpacing"):
            if obj.CustomSpacing:
                spacinglist = strprocessOfCustomSpacing(obj.CustomSpacing)
                influenceArea = sum(spacinglist) - spacinglist[0] / 2 - spacinglist[-1] / 2
        # Drop this check to solve issue as discussed here: https://github.com/FreeCAD/FreeCAD/pull/2550
        # if (obj.OffsetStart.Value + obj.OffsetEnd.Value) > size:
        #        return
        # all tests ok!
        if hasattr(obj, "Length"):
            length = getLengthOfRebar(obj)
            if length:
                obj.Length = length
        pl = obj.Placement
        circle = Part.makeCircle(obj.Diameter.Value/2,bpoint,bvec)
        circle = Part.Wire(circle)
        try:
            bar = wire.makePipeShell([circle],True,False,2)
            basewire = wire.copy()
        except Part.OCCError:
            print("Arch: error sweeping rebar profile along the base sketch")
            return
        # building final shape
        shapes = []
        placementlist = []
        self.wires = []
        rot = FreeCAD.Rotation()
        if obj.Amount == 1:
            if hasattr(obj, "RebarShape"):
                barplacement = CalculatePlacement(obj.Amount, 1, obj.Diameter.Value, size, axis, rot, obj.OffsetStart.Value, obj.OffsetEnd.Value, obj.RebarShape)
            else:
                barplacement = CalculatePlacement(obj.Amount, 1, obj.Diameter.Value, size, axis, rot, obj.OffsetStart.Value, obj.OffsetEnd.Value)
            placementlist.append(barplacement)
            if hasattr(obj,"Spacing"):
                obj.Spacing = 0
        else:
            if obj.OffsetStart.Value:
                baseoffset = DraftVecUtils.scaleTo(axis,obj.OffsetStart.Value)
            else:
                baseoffset = None
            if hasattr(obj, "RebarShape") and obj.RebarShape == "Stirrup":
                interval = size - (obj.OffsetStart.Value + obj.OffsetEnd.Value + obj.Diameter.Value)
            else:
                interval = size - (obj.OffsetStart.Value + obj.OffsetEnd.Value)
            interval = interval / (obj.Amount - 1)
            for i in range(obj.Amount):
                if hasattr(obj, "RebarShape"):
                    barplacement = CalculatePlacement(obj.Amount, i+1, obj.Diameter.Value, size, axis, rot, obj.OffsetStart.Value, obj.OffsetEnd.Value, obj.RebarShape)
                else:
                    barplacement = CalculatePlacement(obj.Amount, i+1, obj.Diameter.Value, size, axis, rot, obj.OffsetStart.Value, obj.OffsetEnd.Value)
                placementlist.append(barplacement)
            if hasattr(obj,"Spacing"):
                obj.Spacing = interval
        # Calculate placement of bars from custom spacing.
        if spacinglist:
            placementlist[:] = []
            if hasattr(obj, "RebarShape") and obj.RebarShape == "Stirrup":
                reqInfluenceArea = size - (obj.OffsetStart.Value + obj.OffsetEnd.Value + obj.Diameter.Value)
            else:
                reqInfluenceArea = size - (obj.OffsetStart.Value + obj.OffsetEnd.Value)
            # Avoid unnecessary checks to pass like. For eg.: when we have values
            # like influenceArea is 100.00001 and reqInflueneArea is 100
            if round(influenceArea) > round(reqInfluenceArea):
                FreeCAD.Console.PrintWarning("Influence area of rebars is greater than "+ str(reqInfluenceArea) + ".\n")
            elif round(influenceArea) < round(reqInfluenceArea):
                FreeCAD.Console.PrintWarning("Last span is greater that end offset.\n")
            for i in range(len(spacinglist)):
                barplacement = CustomSpacingPlacement(spacinglist, i+1, axis, father.Placement.Rotation, obj.OffsetStart.Value, obj.OffsetEnd.Value)
                placementlist.append(barplacement)
            obj.Amount = len(spacinglist)
            obj.Spacing = 0
        obj.PlacementList = placementlist
        for i in range(len(obj.PlacementList)):
            bar = bar.copy()
            bar.Placement = obj.PlacementList[i]
            shapes.append(bar)
            w = basewire.copy()
            w.Placement = obj.PlacementList[i]
            self.wires.append(w)
        if shapes:
            obj.Shape = Part.makeCompound(shapes)
            obj.Placement = pl
        obj.TotalLength = obj.Length * len(obj.PlacementList)

class _ViewProviderRebar(ArchComponent.ViewProviderComponent):

    "A View Provider for the Rebar object"

    def __init__(self,vobj):

        ArchComponent.ViewProviderComponent.__init__(self,vobj)
        self.setProperties(vobj)
        vobj.ShapeColor = ArchCommands.getDefaultColor("Rebar")

    def setProperties(self,vobj):

        pl = vobj.PropertiesList
        if not "RebarShape" in pl:
            vobj.addProperty("App::PropertyString","RebarShape","Rebar",QT_TRANSLATE_NOOP("App::Property","Shape of rebar"), locked=True).RebarShape
            vobj.setEditorMode("RebarShape",2)

    def onDocumentRestored(self,vobj):

        self.setProperties(vobj)

    def getIcon(self):

        import Arch_rc
        return ":/icons/Arch_Rebar_Tree.svg"

    def setEdit(self, vobj, mode):
        # The Reinforcement Workbench does not implement resetEdit.
        # Therefore unsetEdit is never called and objects would stay in
        # edit mode if this function were to return `True`.

        if mode != 0:
            return None

        if hasattr(vobj.Object, "RebarShape"):
            try:
                # Import module of RebarShape
                module = __import__(vobj.Object.RebarShape)
            except ImportError:
                FreeCAD.Console.PrintError("Unable to import RebarShape module\n")
                return False
        elif vobj.RebarShape:
            try:
                # Import module of RebarShape
                module = __import__(vobj.RebarShape)
            except ImportError:
                FreeCAD.Console.PrintError("Unable to import RebarShape module\n")
                return False
        else:
            return False

        module.editDialog(vobj)
        return False

    def updateData(self,obj,prop):

        if prop == "Shape":
            if hasattr(self,"centerline"):
                if self.centerline:
                    self.centerlinegroup.removeChild(self.centerline)
            if hasattr(obj.Proxy,"wires"):
                if obj.Proxy.wires:
                    import re
                    from pivy import coin
                    import Part
                    self.centerline = coin.SoSeparator()
                    comp = Part.makeCompound(obj.Proxy.wires)
                    buf = re.findall(r"point \[(.*?)\]",comp.writeInventor().replace("\n",""))
                    pts = [zip(*[iter( c.split() )]*3) for c in buf]
                    for pt in pts:
                        vlist = [ [float(v[0]),float(v[1]),float(v[2])] for v in pt ]
                        ps = coin.SoSeparator()
                        coords = coin.SoCoordinate3()
                        coords.point.setValues(vlist)
                        ps.addChild(coords)
                        ls = coin.SoLineSet()
                        ls.numVertices = -1
                        ps.addChild(ls)
                        self.centerline.addChild(ps)
                    self.centerlinegroup.addChild(self.centerline)
        ArchComponent.ViewProviderComponent.updateData(self,obj,prop)

    def attach(self,vobj):

        from pivy import coin
        self.centerlinegroup = coin.SoSeparator()
        self.centerlinegroup.setName("Centerline")
        self.centerlinecolor = coin.SoBaseColor()
        self.centerlinestyle = coin.SoDrawStyle()
        self.centerlinegroup.addChild(self.centerlinecolor)
        self.centerlinegroup.addChild(self.centerlinestyle)
        vobj.addDisplayMode(self.centerlinegroup,"Centerline")
        ArchComponent.ViewProviderComponent.attach(self,vobj)

    def onChanged(self,vobj,prop):

        if (prop == "LineColor") and hasattr(vobj,"LineColor"):
            if hasattr(self,"centerlinecolor"):
                c = vobj.LineColor
                self.centerlinecolor.rgb.setValue(c[0],c[1],c[2])
        elif (prop == "LineWidth") and hasattr(vobj,"LineWidth"):
            if hasattr(self,"centerlinestyle"):
                self.centerlinestyle.lineWidth = vobj.LineWidth
        ArchComponent.ViewProviderComponent.onChanged(self,vobj,prop)

    def getDisplayModes(self,vobj):

        modes=["Centerline"]
        return modes+ArchComponent.ViewProviderComponent.getDisplayModes(self,vobj)

def CalculatePlacement(baramount, barnumber, bardiameter, size, axis, rotation, offsetstart, offsetend, RebarShape = ""):

    """ CalculatePlacement([baramount, barnumber, bardiameter, size, axis, rotation, offsetstart, offsetend, RebarShape]):
    Calculate the placement of the bar from given values."""
    if baramount == 1:
        interval = offsetstart
    else:
        if RebarShape == "Stirrup":
            interval = size - (offsetstart + offsetend + bardiameter)
        else:
            interval = size - (offsetstart + offsetend)
        interval = interval / (baramount - 1)
    bardistance = (interval * (barnumber - 1)) + offsetstart
    barplacement = DraftVecUtils.scaleTo(axis, bardistance)
    placement = FreeCAD.Placement(barplacement, rotation)
    return placement

def CustomSpacingPlacement(spacinglist, barnumber, axis, rotation, offsetstart, offsetend):

    """ CustomSpacingPlacement(spacinglist, barnumber, axis, rotation, offsetstart, offsetend):
    Calculate placement of the bar from custom spacing list."""
    if barnumber == 1:
        bardistance = offsetstart
    else:
        bardistance = sum(spacinglist[0:barnumber])
        bardistance = bardistance - spacinglist[0] / 2
        bardistance = bardistance - spacinglist[barnumber - 1] / 2
        bardistance = bardistance + offsetstart
    barplacement = DraftVecUtils.scaleTo(axis, bardistance)
    placement = FreeCAD.Placement(barplacement, rotation)
    return placement

def strprocessOfCustomSpacing(span_string):

    """ strprocessOfCustomSpacing(span_string): This function take input
    in specific syntax and return output in the form of list. For eg.
    Input: "3@100+2@200+3@100"
    Output: [100, 100, 100, 200, 200, 100, 100, 100]"""
    # import string
    span_st = span_string.strip()
    span_sp = span_st.split('+')
    index = 0
    spacinglist = []
    while index < len(span_sp):
        # Find "@" recursively in span_sp array.
        # If not found, append the index value to "spacinglist" array.
        if span_sp[index].find('@') == -1:
            spacinglist.append(float(span_sp[index]))
        else:
            in_sp = span_sp[index].split('@')
            count = 0
            while count < int(in_sp[0]):
                spacinglist.append(float(in_sp[1]))
                count += 1
        index += 1
    return spacinglist

def getLengthOfRebar(rebar):

    """ getLengthOfRebar(RebarObject): Calculates the length of the rebar."""
    base = rebar.Base
    # When rebar is derived from DWire
    if hasattr(base, "Length"):
        return base.Length
    # When rebar is derived from Sketch
    elif base.isDerivedFrom("Sketcher::SketchObject"):
        length = 0
        for geo in base.Geometry:
            length += geo.length()
        return length
    elif base.isDerivedFrom("Part::Helix"):
        return base.Shape.Wires[0].Length
    else:
        FreeCAD.Console.PrintError("Cannot calculate rebar length from its base object\n")
        return None
