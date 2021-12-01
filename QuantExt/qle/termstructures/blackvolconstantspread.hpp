/*
 Copyright (C) 2017 Quaternion Risk Management Ltd
 All rights reserved.

 This file is part of ORE, a free-software/open-source library
 for transparent pricing and risk analysis - http://opensourcerisk.org

 ORE is free software: you can redistribute it and/or modify it
 under the terms of the Modified BSD License.  You should have received a
 copy of the license along with this program.
 The license is also available online at <http://opensourcerisk.org>

 This program is distributed on the basis that it will form a useful
 contribution to risk analytics and model standardisation, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE. See the license for more details.
*/

/*! \file qle/termstructures/equityvolconstantspread.hpp
    \brief surface that combines an ATM curve and vol spreads from a surface
    \ingroup termstructures
*/

#ifndef quantext_blackvolatilityconstantspread_hpp
#define quantext_blackvolatilityconstantspread_hpp

#include <boost/shared_ptr.hpp>
#include <ql/termstructures/volatility/equityfx/blackvariancecurve.hpp>
#include <ql/termstructures/volatility/equityfx/blackvariancesurface.hpp>
#include <ql/termstructures/volatility/equityfx/blackvoltermstructure.hpp>

namespace QuantExt {
using namespace QuantLib;

//! Cube that combines an ATM matrix and vol spreads from a cube
/*! Notice that the TS has a floating reference date and accesses the source TS only via
 their time-based volatility methods.

 \warning the given atm vol structure should be strike independent, this is not checked
*/
class BlackVolatilityConstantSpread : public BlackVolTermStructure {
public:
    BlackVolatilityConstantSpread(const Handle<BlackVolTermStructure>& atm,
                                  const Handle<BlackVolTermStructure>& surface);

    //! \name TermStructure interface
    //@{
    DayCounter dayCounter() const;
    Date maxDate() const;
    Time maxTime() const;
    const Date& referenceDate() const;
    Calendar calendar() const;
    Natural settlementDays() const;
    //@}
    //! \name VolatilityTermStructure interface
    //@{
    Rate minStrike() const;
    Rate maxStrike() const;
    //@}

    // override Termstructure deepUpdate to ensure atm_ curve is updatesd
    void deepUpdate();

protected:
    Volatility blackVolImpl(Time t, Rate strike) const;
    Real blackVarianceImpl(Time t, Real strike) const;

private:
    Handle<BlackVolTermStructure> atm_, surface_;
};

} // namespace QuantExt

#endif
