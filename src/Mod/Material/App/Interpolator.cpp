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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QMetaType>
#include <QRegularExpression>
#endif

// #include <App/Application.h>
// #include <Base/QtTools.h>
#include <Base/Console.h>
#include <Base/Quantity.h>
#include <Gui/MetaTypes.h>

#include "Exceptions.h"
#include "Interpolator.h"


using namespace Materials;

/* TRANSLATOR Material::Interpolator */

Interpolator::Interpolator()
{}

Interpolator::Interpolator(const Interpolator& other)
{}

Interpolator::Interpolator(const Array2D& array)
{}

Interpolator::Interpolator(const Array3D& array, const QVariant& samplePoint)
{}

double Interpolator::valueOf(const QVariant& value)
{
    if (value.isNull()) {
        throw InterpolationError(QLatin1String("Array has undefined entries"));
    }

    if (value.canConvert<Base::Quantity>()) {
        auto quantity = value.value<Base::Quantity>();
        return quantity.getValue();
    }

    return value.toFloat();
}

bool Interpolator::compare(const std::vector<double>& a, const std::vector<double>& b)
{
    return a[0] < b[0];
}

/* TRANSLATOR Material::InterpolatorSpline */

InterpolatorSpline::InterpolatorSpline()
{}

InterpolatorSpline::InterpolatorSpline(const InterpolatorSpline& other)
    : _xmin(other._xmin)
    , _xmax(other._xmax)
{
    for (auto interp : other._interpolators) {
        _interpolators.append(Spline2d(interp));
    }
}

InterpolatorSpline::InterpolatorSpline(const Array2D& array)
{
    create(array);
}

InterpolatorSpline::InterpolatorSpline(const Array3D& array, int depth)
{
    create(array, depth);
}

double InterpolatorSpline::scale(double x) const
{
    // Helpers to scale X values down to [0, 1]
    return (x - _xmin) / (_xmax - _xmin);
}

Eigen::RowVectorXd InterpolatorSpline::scaledValues(Eigen::VectorXd const& x_vec) const
{
    return x_vec
        .unaryExpr([this](double x) {
            return scale(x);
        })
        .transpose();
}

QList<QVariant> InterpolatorSpline::interpolate(const QVariant& samplePoint, bool extrapolate)
{
    // May be a quantity, int, float, etc
    auto pointValue = scale(valueOf(samplePoint));
    // Base::Console().Log("sample point(%f)->%f\n", valueOf(samplePoint), pointValue);

    if (!extrapolate) {
        if ((pointValue < 0.0) || (pointValue > 1.0)) {
            throw InterpolationOutOfRangeError();
        }
    }

    QList<QVariant> ret;

    for (auto interp : _interpolators) {
        auto values = interp(pointValue);
        ret.append(values(0));
    }

    return ret;
}

QList<QVariant> InterpolatorSpline::interpolate(const QVariant& samplePoint1,
                                                const QVariant& samplePoint2,
                                                bool extrapolate)
{
    // May be a quantity, int, float, etc
    auto pointValue = scale(valueOf(samplePoint1));

    // if (!extrapolate) {
    //     if ((pointValue < 0.0) || (pointValue > 1.0)) {
    //         throw InterpolationOutOfRangeError();
    //     }
    // }

    QList<QVariant> ret;

    for (auto interp : _interpolators) {
        auto values = interp(pointValue);
        ret.append(values(0));
    }

    return ret;
}

Spline2d InterpolatorSpline::createInterpolator(std::vector<double>& abscissas,
                                                std::vector<double>& ordinates)
{
    Eigen::VectorXd x_vec(abscissas.size());
    Eigen::VectorXd y_vec(abscissas.size());
    for (int index = 0; index < abscissas.size(); index++) {
        x_vec(index) = abscissas[index];
        y_vec(index) = ordinates[index];
    }

    // No more than cubic spline, but accept short vectors.
    return Eigen::SplineFitting<Spline2d>::Interpolate(y_vec.transpose(),
                                                       std::min<int>(x_vec.rows() - 1, 3),
                                                       scaledValues(x_vec));
}

void InterpolatorSpline::create(const Array2D& array)
{
    if (array.columns() < 2 || array.rows() < 2) {
        throw InterpolationError(QLatin1String("No data to interpolate"));
    }

    // Sort the array so that the abscissas are in order
    std::vector<std::vector<double>> sortArray;
    for (auto row : array.getArray()) {
        std::vector<double> rowCopy;
        for (auto column : *row) {
            rowCopy.push_back(valueOf(column));
        }
        sortArray.push_back(rowCopy);
    }
    std::sort(sortArray.begin(), sortArray.end(), compare);


    std::vector<double> abscissas;
    std::vector<std::vector<double>> ordinates;
    for (int i = 1; i < array.columns(); i++) {
        ordinates.push_back(std::vector<double>());
    }
    for (auto row : sortArray) {
        abscissas.push_back(valueOf(row[0]));
        for (int i = 1; i < array.columns(); i++) {
            ordinates[i - 1].push_back(valueOf(row[i]));
        }
    }

    _xmin = abscissas[0];
    _xmax = abscissas[array.rows() - 1];

    _interpolators.clear();
    for (auto ordinate : ordinates) {
        _interpolators.append(createInterpolator(abscissas, ordinate));
    }
}

void InterpolatorSpline::create(const Array3D& array, int depth)
{
    if (array.columns() < 2 || array.rows(depth) < 2) {
        throw InterpolationError(QLatin1String("No data to interpolate"));
    }

    // Sort the array so that the abscissas are in order
    std::vector<std::vector<double>> sortArray;
    for (auto row : *array.getTable(depth)) {
        std::vector<double> rowCopy;
        for (auto column : *row) {
            rowCopy.push_back(column.getValue());
        }
        sortArray.push_back(rowCopy);
    }
    std::sort(sortArray.begin(), sortArray.end(), compare);


    std::vector<double> abscissas;
    std::vector<std::vector<double>> ordinates;
    for (int i = 1; i < array.columns(); i++) {
        ordinates.push_back(std::vector<double>());
    }
    for (auto row : sortArray) {
        abscissas.push_back(valueOf(row[0]));
        for (int i = 1; i < array.columns(); i++) {
            ordinates[i - 1].push_back(valueOf(row[i]));
        }
    }

    _xmin = abscissas[0];
    _xmax = abscissas[array.rows() - 1];

    _interpolators.clear();
    for (auto ordinate : ordinates) {
        _interpolators.append(createInterpolator(abscissas, ordinate));
    }
}

QList<Spline2d> InterpolatorSpline::createSplines(const Array3D& array,
                                                  int depth,
                                                  const QVariant& samplePoint)
{
    auto& table = array.getTable(depth);

    // Sort the array so that the abscissas are in order
    std::vector<std::vector<double>> sortArray;
    for (auto row : *table) {
        std::vector<double> rowCopy;
        for (auto column : *row) {
            rowCopy.push_back(column.getValue());
        }
        sortArray.push_back(rowCopy);
    }
    std::sort(sortArray.begin(), sortArray.end(), compare);


    std::vector<double> abscissas;
    std::vector<std::vector<double>> ordinates;
    for (int i = 1; i < array.columns(); i++) {
        ordinates.push_back(std::vector<double>());
    }
    for (auto row : sortArray) {
        abscissas.push_back(row[0]);
        for (int i = 1; i < array.columns(); i++) {
            ordinates[i - 1].push_back(row[i]);
        }
    }

    // _xmin = abscissas[0];
    // _xmax = abscissas[array.rows() - 1];

    QList<Spline2d> interpolators;
    for (auto ordinate : ordinates) {
        interpolators.append(createInterpolator(abscissas, ordinate));
    }
    return interpolators;
}

/* TRANSLATOR Material::InterpolatorSpline3D */

InterpolatorSpline3D::InterpolatorSpline3D()
{}

InterpolatorSpline3D::InterpolatorSpline3D(const InterpolatorSpline3D& other)
    : _3dInterpolator(other._3dInterpolator)
    , _zmin(other._zmin)
    , _zmax(other._zmax)
{
    for (auto& interp : other._interpolators) {
        _interpolators.append(InterpolatorSpline(interp));
    }
}

InterpolatorSpline3D::InterpolatorSpline3D(const Array3D& array)
{
    create(array);
}

double InterpolatorSpline3D::scale(double z) const
{
    // Helpers to scale X values down to [0, 1]
    return (z - _zmin) / (_zmax - _zmin);
}

Eigen::RowVectorXd InterpolatorSpline3D::scaledValues(Eigen::VectorXd const& x_vec) const
{
    return x_vec
        .unaryExpr([this](double x) {
            return scale(x);
        })
        .transpose();
}

QList<QVariant> InterpolatorSpline3D::interpolate(const QVariant& samplePoint, bool extrapolate)
{
    // // May be a quantity, int, float, etc
    // auto pointValue = scale(valueOf(samplePoint));
    // // Base::Console().Log("sample point(%f)->%f\n", valueOf(samplePoint), pointValue);

    // if (!extrapolate) {
    //     if ((pointValue < 0.0) || (pointValue > 1.0)) {
    //         throw InterpolationOutOfRangeError();
    //     }
    // }

    QList<QVariant> ret;

    // for (auto interp : _interpolators) {
    //     auto values = interp(pointValue);
    //     ret.append(values(0));
    // }

    return ret;
}

QList<QVariant> InterpolatorSpline3D::interpolate(const QVariant& samplePoint1,
                                                  const QVariant& samplePoint2,
                                                  bool extrapolate)
{
    // May be a quantity, int, float, etc
    auto pointValue1 = scale(valueOf(samplePoint1));
    auto pointValue2 = scale(valueOf(samplePoint2));

    if (!extrapolate) {
        if ((pointValue1 < 0.0) || (pointValue1 > 1.0)) {
            throw InterpolationOutOfRangeError();
        }
    }

    QList<QVariant> ret;

    // for (auto interp : _interpolators) {
    //     // auto values = interp(pointValue1, pointValue2);
    //     auto values = interp(pointValue1);
    //     ret.append(values(0));
    // }

    return ret;
}

Spline2d InterpolatorSpline3D::createInterpolator(std::vector<double>& abscissas,
                                                std::vector<double>& ordinates)
{
    Eigen::VectorXd x_vec(abscissas.size());
    Eigen::VectorXd y_vec(abscissas.size());
    for (int index = 0; index < abscissas.size(); index++) {
        x_vec(index) = abscissas[index];
        y_vec(index) = ordinates[index];
    }

    // No more than cubic spline, but accept short vectors.
    return Eigen::SplineFitting<Spline2d>::Interpolate(y_vec.transpose(),
                                                       std::min<int>(x_vec.rows() - 1, 3),
                                                       scaledValues(x_vec));
}

void InterpolatorSpline3D::create(const Array3D& array)
{
    auto depth = array.depth();
    if (depth < 2 || array.columns() < 2) {
        throw InterpolationError(QLatin1String("No data to interpolate"));
    }
    _zmin = array.getDepthValue(0).getValue();
    _zmax = _zmin;

    _interpolators.clear();
    for (int depthIndex = 0; depthIndex < depth; depthIndex++) {
        double value = array.getDepthValue(depthIndex).getValue();
        if (_zmin > value) {
            _zmin = value;
        }
        if (_zmax < value) {
            _zmax = value;
        }

        _interpolators.append(InterpolatorSpline(array, depthIndex));
    }
}

QList<Spline2d> InterpolatorSpline3D::createSplines(const Array3D& array,
                                                  int depth,
                                                  const QVariant& samplePoint)
{
    auto& table = array.getTable(depth);

    // Sort the array so that the abscissas are in order
    std::vector<std::vector<double>> sortArray;
    for (auto row : *table) {
        std::vector<double> rowCopy;
        for (auto column : *row) {
            rowCopy.push_back(column.getValue());
        }
        sortArray.push_back(rowCopy);
    }
    std::sort(sortArray.begin(), sortArray.end(), compare);


    std::vector<double> abscissas;
    std::vector<std::vector<double>> ordinates;
    for (int i = 1; i < array.columns(); i++) {
        ordinates.push_back(std::vector<double>());
    }
    for (auto row : sortArray) {
        abscissas.push_back(row[0]);
        for (int i = 1; i < array.columns(); i++) {
            ordinates[i - 1].push_back(row[i]);
        }
    }

    // _xmin = abscissas[0];
    // _xmax = abscissas[array.rows() - 1];

    QList<Spline2d> interpolators;
    // for (auto ordinate : ordinates) {
    //     interpolators.append(createInterpolator(abscissas, ordinate));
    // }
    return interpolators;
}
