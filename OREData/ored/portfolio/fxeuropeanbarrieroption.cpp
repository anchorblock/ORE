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

#include <boost/make_shared.hpp>
#include <ored/portfolio/builders/fxoption.hpp>
#include <ored/portfolio/builders/fxdigitaloption.hpp>
#include <ored/portfolio/enginefactory.hpp>
#include <ored/portfolio/fxeuropeanbarrieroption.hpp>
#include <ored/portfolio/fxoption.hpp>
#include <ored/utilities/log.hpp>
#include <ored/utilities/parsers.hpp>
#include <ql/errors.hpp>
#include <ql/exercise.hpp>
#include <ql/instruments/barriertype.hpp>
#include <ql/instruments/compositeinstrument.hpp>
#include <ql/instruments/vanillaoption.hpp>

using namespace QuantLib;

namespace ore {
namespace data {

void FxEuropeanBarrierOption::build(const boost::shared_ptr<EngineFactory>& engineFactory) {

    const boost::shared_ptr<Market> market = engineFactory->market();

    // Only European Single Barrier supported for now
    QL_REQUIRE(option_.style() == "European", "Option Style unknown: " << option_.style());
    QL_REQUIRE(option_.exerciseDates().size() == 1, "Invalid number of excercise dates");
    QL_REQUIRE(barrier_.levels().size() == 1, "Invalid number of barrier levels");
    QL_REQUIRE(barrier_.style().empty() || barrier_.style() == "European", "Only european barrier style suppported");
    QL_REQUIRE(tradeActions().empty(), "TradeActions not supported for FxEuropeanBarrierOption");

    Currency boughtCcy = parseCurrency(boughtCurrency_);
    Currency soldCcy = parseCurrency(soldCurrency_);
    Real level = barrier_.levels()[0].value();
    Real rebate = barrier_.rebate();
    QL_REQUIRE(rebate >= 0, "Rebate must be non-negative");

    // Replicate the payoff of European Barrier Option (with strike K and barrier B) using combinations of options

    // Call
    //   Up
    //     In
    //       Long Up&Out Digital Option with barrier B payoff rebate
    //       B > K
    //         Long European Call Option with strike B
    //         Long Up&In Digital Option with barrier B payoff B - K
    //       B <= K
    //         Long European Call Option with strike K
    //     Out
    //       Long Up&In Digital Option with barrier B payoff rebate
    //       B > K
    //         Long European Call Option with strike K
    //         Short European Call Option with strike B
    //         Short Up&In Digital Option with barrier B payoff B - K
    //       B <= K
    //         0
    //   Down
    //     In
    //       Long Down&Out Digital Option with barrier B payoff rebate
    //       B > K
    //         Long European Call Option with strike K
    //       B <= K
    //         0
    //     Out
    //       Long Down&In Digital Option with barrier B payoff rebate
    //       B > K
    //         Long European Call Option with strike B
    //         Long Down&Out Digital Option with barrier B payoff B - K
    //       B <= K
    //         Long European Call Option with strike K

    // Put
    //   Up
    //     In
    //       Long Up&Out Digital Option with barrier B payoff rebate
    //       B > K
    //         0
    //       B <= K
    //         Long European Put Option with strike K
    //         Short European Put Option with strike B
    //         Short Up&Out Digital Option with barrier B payoff K - B
    //     Out
    //       Long Up&In Digital Option with barrier B payoff rebate
    //       B > K
    //         Long European Put Option with strike K
    //       B <= K
    //         Long European Put Option with strike B
    //         Long Up&Out Digital Option with barrier B payoff K - B
    //   Down
    //     In
    //       Long Down&Out Digital Option with barrier B payoff rebate
    //       B > K
    //         Long European Put Option with strike K
    //       B <= K
    //         Long European Put Option with strike B
    //         Long Down&In Digital Option with barrier B payoff K - B
    //     Out
    //       Long Down&In Digital Option with barrier B payoff rebate
    //       B > K
    //         0
    //       B <= K
    //         Long European Put Option with strike K

    Real strike = soldAmount_ / boughtAmount_;
    Option::Type type = parseOptionType(option_.callPut());

    // Exercise
    Date expiryDate = parseDate(option_.exerciseDates().front());
    boost::shared_ptr<Exercise> exercise = boost::make_shared<EuropeanExercise>(expiryDate);

    Barrier::Type barrierType = parseBarrierType(barrier_.type());

    // Payoff - European Option with strike K
    boost::shared_ptr<StrikedTypePayoff> payoffVanillaK(new PlainVanillaPayoff(type, strike));
    // Payoff - European Option with strike B
    boost::shared_ptr<StrikedTypePayoff> payoffVanillaB(new PlainVanillaPayoff(type, level));
    // Payoff - Digital Option with barrier B payoff abs(B - K)
    boost::shared_ptr<StrikedTypePayoff> payoffDigital(new CashOrNothingPayoff(type, level, fabs(level - strike)));

    boost::shared_ptr<Instrument> digital = boost::make_shared<VanillaOption>(payoffDigital, exercise);
    boost::shared_ptr<Instrument> vanillaK = boost::make_shared<VanillaOption>(payoffVanillaK, exercise);
    boost::shared_ptr<Instrument> vanillaB = boost::make_shared<VanillaOption>(payoffVanillaB, exercise);

    boost::shared_ptr<StrikedTypePayoff> rebatePayoff;
    if (barrierType == Barrier::Type::UpIn || barrierType == Barrier::Type::DownOut) {
        // Payoff - Up&Out / Down&In Digital Option with barrier B payoff rebate
        rebatePayoff = boost::make_shared<CashOrNothingPayoff>(Option::Put, level, rebate);
    } else if (barrierType == Barrier::Type::UpOut || barrierType == Barrier::Type::DownIn) {
        // Payoff - Up&In / Down&Out Digital Option with barrier B payoff rebate
        rebatePayoff = boost::make_shared<CashOrNothingPayoff>(Option::Call, level, rebate);
    }
    boost::shared_ptr<Instrument> rebateInstrument = boost::make_shared<VanillaOption>(rebatePayoff, exercise);

    // set pricing engines
    boost::shared_ptr<EngineBuilder> builder = engineFactory->builder("FxOption");
    QL_REQUIRE(builder, "No builder found for FxOption");
    boost::shared_ptr<FxEuropeanOptionEngineBuilder> fxOptBuilder =
        boost::dynamic_pointer_cast<FxEuropeanOptionEngineBuilder>(builder);

    builder = engineFactory->builder("FxDigitalOption");
    QL_REQUIRE(builder, "No builder found for FxDigitalOption");
    boost::shared_ptr<FxDigitalOptionEngineBuilder> fxDigitalOptBuilder =
        boost::dynamic_pointer_cast<FxDigitalOptionEngineBuilder>(builder);

    digital->setPricingEngine(fxDigitalOptBuilder->engine(boughtCcy, soldCcy));
    vanillaK->setPricingEngine(fxOptBuilder->engine(boughtCcy, soldCcy, expiryDate));
    vanillaB->setPricingEngine(fxOptBuilder->engine(boughtCcy, soldCcy, expiryDate));
    rebateInstrument->setPricingEngine(fxDigitalOptBuilder->engine(boughtCcy, soldCcy));

    boost::shared_ptr<CompositeInstrument> qlInstrument = boost::make_shared<CompositeInstrument>();
    qlInstrument->add(rebateInstrument);
    if (type == Option::Call) {
        if (barrierType == Barrier::Type::UpIn || barrierType == Barrier::Type::DownOut) {
            if (level > strike) {
                qlInstrument->add(vanillaB);
                qlInstrument->add(digital);
            } else {
                qlInstrument->add(vanillaK);
            }
        } else if (barrierType == Barrier::Type::UpOut || barrierType == Barrier::Type::DownIn) {
            if (level > strike) {
                qlInstrument->add(vanillaK);
                qlInstrument->add(vanillaB, -1);
                qlInstrument->add(digital, -1);
            } else {
                // empty
            }
        } else {
            QL_FAIL("Unknown Barrier Type: " << barrierType);
        }
    } else if (type == Option::Put) {
        if (barrierType == Barrier::Type::UpIn || barrierType == Barrier::Type::DownOut) {
            if (level > strike) {
                // empty
            } else {
                qlInstrument->add(vanillaK);
                qlInstrument->add(vanillaB, -1);
                qlInstrument->add(digital, -1);
            }
        } else if (barrierType == Barrier::Type::UpOut || barrierType == Barrier::Type::DownIn) {
            if (level > strike) {
                qlInstrument->add(vanillaK);
            } else {
                qlInstrument->add(vanillaB);
                qlInstrument->add(digital);
            }
        } else {
            QL_FAIL("Unknown Barrier Type: " << barrierType);
        }
    }

    // Add additional premium payments
    Position::Type positionType = parsePositionType(option_.longShort());
    Real bsInd = (positionType == QuantLib::Position::Long ? 1.0 : -1.0);
    Real mult = boughtAmount_ * bsInd;

    std::vector<boost::shared_ptr<Instrument>> additionalInstruments;
    std::vector<Real> additionalMultipliers;
    Date lastPremiumDate =
        addPremiums(additionalInstruments, additionalMultipliers, mult, option_.premiumData(), -bsInd, soldCcy,
                    engineFactory, fxOptBuilder->configuration(MarketContext::pricing));

    instrument_ = boost::shared_ptr<InstrumentWrapper>(
        new VanillaInstrument(qlInstrument, mult, additionalInstruments, additionalMultipliers));

    npvCurrency_ = soldCurrency_; // sold is the domestic
    notional_ = soldAmount_;
    notionalCurrency_ = soldCurrency_;
    maturity_ = std::max(lastPremiumDate, expiryDate);

    additionalData_["boughtCurrency"] = boughtCurrency_; 
    additionalData_["boughtAmount"] = boughtAmount_;
    additionalData_["soldCurrency"] = soldCurrency_;
    additionalData_["soldAmount"] = soldAmount_;
}

void FxEuropeanBarrierOption::fromXML(XMLNode* node) {
    Trade::fromXML(node);
    XMLNode* fxNode = XMLUtils::getChildNode(node, "FxEuropeanBarrierOptionData");
    QL_REQUIRE(fxNode, "No FxEuropeanBarrierOptionData Node");
    option_.fromXML(XMLUtils::getChildNode(fxNode, "OptionData"));
    barrier_.fromXML(XMLUtils::getChildNode(fxNode, "BarrierData"));
    boughtCurrency_ = XMLUtils::getChildValue(fxNode, "BoughtCurrency", true);
    soldCurrency_ = XMLUtils::getChildValue(fxNode, "SoldCurrency", true);
    boughtAmount_ = XMLUtils::getChildValueAsDouble(fxNode, "BoughtAmount", true);
    soldAmount_ = XMLUtils::getChildValueAsDouble(fxNode, "SoldAmount", true);
}

XMLNode* FxEuropeanBarrierOption::toXML(XMLDocument& doc) {
    XMLNode* node = Trade::toXML(doc);
    XMLNode* fxNode = doc.allocNode("FxEuropeanBarrierOptionData");
    XMLUtils::appendNode(node, fxNode);

    XMLUtils::appendNode(fxNode, option_.toXML(doc));
    XMLUtils::appendNode(fxNode, barrier_.toXML(doc));
    XMLUtils::addChild(doc, fxNode, "BoughtCurrency", boughtCurrency_);
    XMLUtils::addChild(doc, fxNode, "BoughtAmount", boughtAmount_);
    XMLUtils::addChild(doc, fxNode, "SoldCurrency", soldCurrency_);
    XMLUtils::addChild(doc, fxNode, "SoldAmount", soldAmount_);

    return node;
}
} // namespace data
} // namespace oreplus