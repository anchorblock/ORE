/*
 Copyright (C) 2019 Quaternion Risk Management Ltd
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

/*! \file ored/utilities/calendaradjustmentconfig.hpp
    \brief 
    \ingroup utilities
*/

#pragma once



#include <ored/utilities/xmlutils.hpp>
#include <map>
#include <vector>
#include <set>
#include <string>
#include <ql/time/date.hpp>
#include <ql/time/calendar.hpp>
#include <ql/utilities/dataparsers.hpp>
#include <ql/patterns/singleton.hpp>


namespace ore {
namespace data {
using std::string;
using std::vector;
using std::map;
using std::set;
using std::string;
using QuantLib::Date;
using QuantLib::Calendar; 



class CalendarAdjustmentConfig : public XMLSerializable {
public:
    
    //default constructor 
    CalendarAdjustmentConfig();
    //! This method adds d to the list of holidays for cal name.
    void addHolidays(const string& calname, const Date& d);

    //! This method adds d to the list of business days for cal name.
    void addBusinessDays(const string& calname, const Date& d);

    //! Returns all the holidays for a given calname
    const vector<Date>& getHolidays(const string& calname) const;

    //! Returns all the business days for a given calname
    const vector<Date>& getBusinessDays(const string& calname) const;

    set<string> getCalendars() const; 

    void fromXML(XMLNode* node) override;
    XMLNode* toXML(XMLDocument& doc) override;



private:
    map<string, vector<Date>> additionalHolidays_;
    map<string, vector<Date>> additionalBusinessDays_;

    string normalisedName(const string&) const;

};

class CalendarAdjustments : public QuantLib::Singleton<CalendarAdjustments> {
    friend class QuantLib::Singleton<CalendarAdjustments>;
public:
    //get the global config
    const CalendarAdjustmentConfig& config() const {
        return config_;
    }

    // set the global config
    CalendarAdjustmentConfig& setConfig(const CalendarAdjustmentConfig& c) {
        config_ = c;
    }

private:
    CalendarAdjustmentConfig config_;

    CalendarAdjustments() {}
};

}
}
