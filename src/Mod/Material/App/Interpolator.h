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
    virtual ~Interpolator() = default;

    virtual QList<QVariant> interpolate(const QVariant& samplePoint) = 0;
    virtual void create(const Material2DArray& array) = 0;

protected:
    static double valueOf(const QVariant& value);
    static bool compare(const std::vector<double>& a, const std::vector<double>& b);
};

typedef Eigen::Spline<double, 1> Spline2d;

class MaterialsExport InterpolatorSpline: public Interpolator
{
public:
    InterpolatorSpline();
    explicit InterpolatorSpline(const InterpolatorSpline& other);
    explicit InterpolatorSpline(const Material2DArray& array);
    ~InterpolatorSpline() override = default;

    QList<QVariant> interpolate(const QVariant& samplePoint) override;
    void create(const Material2DArray& array) override;

private:
    double scale(double x) const;
    Eigen::RowVectorXd scaledValues(Eigen::VectorXd const& x_vec) const;
    Spline2d createInterpolator(std::vector<double>& abscissas, std::vector<double>& ordinates);

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

    QList<QVariant> interpolate(const QVariant& samplePoint) override;
    void create(const Material2DArray& array) override;

private:
    std::pchip<std::vector<double>> createInterpolator(std::vector<double>& abscissas,
                                                       std::vector<double>& ordinates);

    QList<std::pchip<std::vector<double>>> _interpolators;
};

// class MaterialsExport Material2DArray: public MaterialValue
// {
//     TYPESYSTEM_HEADER_WITH_OVERRIDE();

// public:
//     Material2DArray();
//     Material2DArray(const Material2DArray& other);
//     ~Material2DArray() override = default;

//     Material2DArray& operator=(const Material2DArray& other);

//     bool isNull() const override;

//     const QList<std::shared_ptr<QList<QVariant>>>& getArray() const
//     {
//         return _rows;
//     }
//     QList<QVariant> interpolate(const QVariant& samplePoint);

//     void validateRow(int row) const;
//     void validateColumn(int column) const;

//     std::shared_ptr<QList<QVariant>> getRow(int row) const;
//     std::shared_ptr<QList<QVariant>> getRow(int row);
//     int rows() const
//     {
//         return _rows.size();
//     }
//     int columns() const
//     {
//         return _columns;
//     }
//     void setColumns(int size)
//     {
//         _columns = size;
//     }
//     void addRow(const std::shared_ptr<QList<QVariant>>& row);
//     void insertRow(int index, const std::shared_ptr<QList<QVariant>>& row);
//     void deleteRow(int row);

//     void setValue(int row, int column, const QVariant& value);
//     QVariant getValue(int row, int column) const;

//     QString getYAMLString() const override;

// protected:
//     void deepCopy(const Material2DArray& other);
//     std::pchip<std::vector<double>> createInterpolator(std::vector<double>& abscissas,
//                                                        std::vector<double>& ordinates);
//     void createInterpolators();
//     static double valueOf(const QVariant& value);
//     QVariant interpLinear(const QVariant& samplePoint);
//     QVariant
//     interpLinearXY(const QVariant& samplePoint, double x1, double y1, double x2, double y2);
//     static bool compare(const std::vector<double>& a, const std::vector<double>& b);

//     QList<std::shared_ptr<QList<QVariant>>> _rows;
//     int _columns;

// private:
//     static void dumpRow(const std::shared_ptr<QList<QVariant>>& row);
//     void dump() const;

//     QList<std::pchip<std::vector<double>>> _interpolators;
// };

}  // namespace Materials

#endif  // MATERIAL_INTERPOLATOR_H
