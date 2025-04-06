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

#include <Eigen/Core>
#include <unsupported/Eigen/Splines>

#include <Mod/Material/MaterialGlobal.h>
#include <Mod/Material/App/MaterialValue.h>

namespace Materials
{

class MaterialsExport Interpolator
{
public:
    Interpolator();
    explicit Interpolator(const Interpolator& other);
    explicit Interpolator(const Array2D& array);
    explicit Interpolator(const Array3D& array, const QVariant& samplePoint);
    virtual ~Interpolator() = default;

    virtual QList<QVariant> interpolate(const QVariant& samplePoint, bool extrapolate) = 0;
    virtual QList<QVariant>
    interpolate(const QVariant& samplePoint1, const QVariant& samplePoint2, bool extrapolate) = 0;

protected:
    static double valueOf(const QVariant& value);
    static bool compare(const std::vector<double>& a, const std::vector<double>& b);
};

typedef Eigen::Spline<double, 1> Spline2d;
typedef Eigen::Spline<double, 2> Spline3d;

class MaterialsExport InterpolatorSpline: public Interpolator
{
public:
    InterpolatorSpline();
    explicit InterpolatorSpline(const InterpolatorSpline& other);
    explicit InterpolatorSpline(const Array2D& array);
    explicit InterpolatorSpline(const Array3D& array, int depth);
    ~InterpolatorSpline() override = default;

    QList<QVariant> interpolate(const QVariant& samplePoint, bool extrapolate) override;
    QList<QVariant> interpolate(const QVariant& samplePoint1,
                                const QVariant& samplePoint2,
                                bool extrapolate) override;

private:
    double scale(double x) const;
    Eigen::RowVectorXd scaledValues(Eigen::VectorXd const& x_vec) const;
    Spline2d createInterpolator(std::vector<double>& abscissas, std::vector<double>& ordinates);

    void create(const Array2D& array);
    void create(const Array3D& array, int depth);
    QList<Spline2d>
    createSplines(const Array3D& array, int depth, const QVariant& samplePoint);

    QList<Spline2d> _interpolators;
    double _xmin;
    double _xmax;
};

class MaterialsExport InterpolatorSpline3D: public Interpolator
{
public:
    InterpolatorSpline3D();
    explicit InterpolatorSpline3D(const InterpolatorSpline3D& other);
    explicit InterpolatorSpline3D(const Array3D& array);
    ~InterpolatorSpline3D() override = default;

    QList<QVariant> interpolate(const QVariant& samplePoint, bool extrapolate) override;
    QList<QVariant> interpolate(const QVariant& samplePoint1,
                                const QVariant& samplePoint2,
                                bool extrapolate) override;

private:
    double scale(double z) const;
    Eigen::RowVectorXd scaledValues(Eigen::VectorXd const& x_vec) const;
    Spline2d createInterpolator(std::vector<double>& abscissas, std::vector<double>& ordinates);

    void create(const Array3D& array);
    QList<Spline2d> createSplines(const Array3D& array, int depth, const QVariant& samplePoint);

    QList<InterpolatorSpline> _interpolators;
    Spline2d _3dInterpolator;
    double _zmin;
    double _zmax;
};

}  // namespace Materials

#endif  // MATERIAL_INTERPOLATOR_H
