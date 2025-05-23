/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#include "kdl_cp/chain.hpp"
#endif

#include <Base/Reader.h>
#include <Base/Writer.h>

#include "Waypoint.h"


using namespace Robot;
using namespace Base;
using namespace KDL;


TYPESYSTEM_SOURCE(Robot::Waypoint, Base::Persistence)

Waypoint::Waypoint(const char* name,
                   const Base::Placement& endPos,
                   WaypointType type,
                   float velocity,
                   float acceleration,
                   bool cont,
                   unsigned int tool,
                   unsigned int base)

    : Name(name)
    , Type(type)
    , Velocity(velocity)
    , Acceleration(acceleration)
    , Cont(cont)
    , Tool(tool)
    , Base(base)
    , EndPos(endPos)
{}

Waypoint::Waypoint()
    : Type(UNDEF)
    , Velocity(1000.0)
    , Acceleration(100.0)
    , Cont(false)
    , Tool(0)
    , Base(0)
{}

Waypoint::~Waypoint() = default;

unsigned int Waypoint::getMemSize() const
{
    return 0;
}

void Waypoint::Save(Writer& writer) const
{
    writer.Stream() << writer.ind() << "<Waypoint "
                    << "name=\"" << Name << "\" "
                    << "Px=\"" << EndPos.getPosition().x << "\" "
                    << "Py=\"" << EndPos.getPosition().y << "\" "
                    << "Pz=\"" << EndPos.getPosition().z << "\" "
                    << "Q0=\"" << EndPos.getRotation()[0] << "\" "
                    << "Q1=\"" << EndPos.getRotation()[1] << "\" "
                    << "Q2=\"" << EndPos.getRotation()[2] << "\" "
                    << "Q3=\"" << EndPos.getRotation()[3] << "\" "
                    << "vel=\"" << Velocity << "\" "
                    << "acc=\"" << Acceleration << "\" "
                    << "cont=\"" << int((Cont) ? 1 : 0) << "\" "
                    << "tool=\"" << Tool << "\" "
                    << "base=\"" << Base << "\" ";
    if (Type == Waypoint::PTP) {
        writer.Stream() << " type=\"PTP\"/> ";
    }
    else if (Type == Waypoint::LINE) {
        writer.Stream() << " type=\"LIN\"/> ";
    }
    else if (Type == Waypoint::CIRC) {
        writer.Stream() << " type=\"CIRC\"/> ";
    }
    else if (Type == Waypoint::WAIT) {
        writer.Stream() << " type=\"WAIT\"/> ";
    }
    else if (Type == Waypoint::UNDEF) {
        writer.Stream() << " type=\"UNDEF\"/> ";
    }
    writer.Stream() << std::endl;
}

void Waypoint::Restore(XMLReader& reader)
{
    // read my Element
    reader.readElement("Waypoint");
    Name = reader.getAttribute<const char*>("name");
    // get the value of the placement
    EndPos = Base::Placement(Base::Vector3d(reader.getAttribute<double>("Px"),
                                            reader.getAttribute<double>("Py"),
                                            reader.getAttribute<double>("Pz")),
                             Base::Rotation(reader.getAttribute<double>("Q0"),
                                            reader.getAttribute<double>("Q1"),
                                            reader.getAttribute<double>("Q2"),
                                            reader.getAttribute<double>("Q3")));

    Velocity = (float)reader.getAttribute<double>("vel");
    Acceleration = (float)reader.getAttribute<double>("acc");
    Cont = reader.getAttribute<bool>("cont");
    Tool = reader.getAttribute<long>("tool");
    Base = reader.getAttribute<long>("base");

    std::string type = reader.getAttribute<const char*>("type");
    if (type == "PTP") {
        Type = Waypoint::PTP;
    }
    else if (type == "LIN") {
        Type = Waypoint::LINE;
    }
    else if (type == "CIRC") {
        Type = Waypoint::CIRC;
    }
    else if (type == "WAIT") {
        Type = Waypoint::WAIT;
    }
    else {
        Type = Waypoint::UNDEF;
    }
}
