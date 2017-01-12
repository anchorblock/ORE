/* 
Copyright (C) 2016 Quaternion Risk Management Ltd
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

#pragma once

#include <ored/portfolio/trade.hpp>
#include <ored/portfolio/legdata.hpp>
#include <ored/portfolio/swap.hpp>

namespace ore{
namespace data{

class Bond : public Trade {
public:
    //!Default constructor
    Bond() : Trade("Bond") {}

    //! Constructor 
    Bond(Envelope env, string settlementDays, string calendar, string issueDate, LegData& coupons) 
        : Trade("Bond", env), settlementDays_(settlementDays), calendar_(calendar), issueDate_(issueDate), 
          coupons_({coupons}) {}

    //Build QuantLib/QuantExt instrument, link pricing engine
    virtual void build(const boost::shared_ptr<EngineFactory>&);

    virtual void fromXML(XMLNode* node);
    virtual XMLNode* toXML(XMLDocument& doc);

    const string& settlementDays() const { return settlementDays_; }  
    const string& calendar() const { return calendar_; }
    const string& issueDate() const { return issueDate_; }
    const LegData& coupons() const { return coupons_; } 

private:
    string settlementDays_;
    string calendar_;
    string issueDate_;
    LegData coupons_;
};
}
}

