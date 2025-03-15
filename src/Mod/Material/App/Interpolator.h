/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#ifndef MATERIAL_INTERPOLATOR_H
#define MATERIAL_INTERPOLATOR_H

#include <memory>

// #include <QList>
// #include <QMetaType>
// #include <QVariant>

#include <boost/math/interpolators/pchip.hpp>

// #include <Gui/MetaTypes.h>

#include <Eigen/Core>
#include <unsupported/Eigen/Splines>

#include <Mod/Material/MaterialGlobal.h>
#include <Mod/Material/App/MaterialValue.h>

namespace std
{
using boost::math::interpolators::pchip;
}

namespace Materials
{

class MaterialsExport Interpolator
{
public:
    Interpolator();
    explicit Interpolator(const Interpolator& other);
    explicit Interpolator(const Material2DArray& array);
    explicit Interpolator(const Material3DArray& array, const QVariant& samplePoint);
    virtual ~Interpolator() = default;

    virtual QList<QVariant> interpolate(const QVariant& samplePoint, bool extrapolate) = 0;
    virtual QList<QVariant> interpolate3D(const QVariant& samplePoint) = 0;

protected:
    static double valueOf(const QVariant& value);
    static bool compare(const std::vector<double>& a, const std::vector<double>& b);

    virtual void create(const Material2DArray& array) = 0;
    virtual void create(const Material3DArray& array, const QVariant& samplePoint) = 0;
};

typedef Eigen::Spline<double, 1> Spline2d;

class MaterialsExport InterpolatorSpline: public Interpolator
{
public:
    InterpolatorSpline();
    explicit InterpolatorSpline(const InterpolatorSpline& other);
    explicit InterpolatorSpline(const Material2DArray& array);
    explicit InterpolatorSpline(const Material3DArray& array, const QVariant& samplePoint);
    ~InterpolatorSpline() override = default;

    QList<QVariant> interpolate(const QVariant& samplePoint, bool extrapolate) override;
    QList<QVariant> interpolate3D(const QVariant& samplePoint) override;

private:
    double scale(double x) const;
    Eigen::RowVectorXd scaledValues(Eigen::VectorXd const& x_vec) const;
    Spline2d createInterpolator(std::vector<double>& abscissas, std::vector<double>& ordinates);

    void create(const Material2DArray& array) override;
    void create(const Material3DArray& array, const QVariant& samplePoint) override;
    QList<Spline2d>
    createSplines(const Material3DArray& array, int depth, const QVariant& samplePoint);

    QList<Spline2d> _interpolators;
    double _xmin;
    double _xmax;
};

class MaterialsExport InterpolatorPchip: public Interpolator
{
public:
    InterpolatorPchip();
    explicit InterpolatorPchip(const InterpolatorPchip& other);
    explicit InterpolatorPchip(const Material2DArray& array);
    ~InterpolatorPchip() override = default;

    QList<QVariant> interpolate(const QVariant& samplePoint, bool extrapolate) override;
    void create(const Material2DArray& array) override;

private:
    std::pchip<std::vector<double>> createInterpolator(std::vector<double>& abscissas,
                                                       std::vector<double>& ordinates);

    QList<std::pchip<std::vector<double>>> _interpolators;
};

}  // namespace Materials

#endif  // MATERIAL_INTERPOLATOR_H
