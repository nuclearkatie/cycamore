// storage.cc
// Implements the Storage class
#include "storage.h"

namespace cycamore {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Storage::Storage(cyclus::Context* ctx)
    : cyclus::Facility(ctx),
      latitude(0.0),
      longitude(0.0),
      coordinates(latitude, longitude) {
  cyclus::Warn<cyclus::EXPERIMENTAL_WARNING>(
      "The Storage Facility is experimental.");};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// pragmas

#pragma cyclus def schema cycamore::Storage

#pragma cyclus def annotations cycamore::Storage

#pragma cyclus def initinv cycamore::Storage

#pragma cyclus def snapshotinv cycamore::Storage

#pragma cyclus def infiletodb cycamore::Storage

#pragma cyclus def snapshot cycamore::Storage

#pragma cyclus def clone cycamore::Storage

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Storage::InitFrom(Storage* m) {
#pragma cyclus impl initfromcopy cycamore::Storage
  cyclus::toolkit::CommodityProducer::Copy(m);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Storage::InitFrom(cyclus::QueryableBackend* b) {
#pragma cyclus impl initfromdb cycamore::Storage

  using cyclus::toolkit::Commodity;
  Commodity commod = Commodity(out_commods.front());
  cyclus::toolkit::CommodityProducer::Add(commod);
  cyclus::toolkit::CommodityProducer::SetCapacity(commod, throughput);
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Storage::EnterNotify() {
  cyclus::Facility::EnterNotify();
  buy_policy.Init(this, &inventory, std::string("inventory"), throughput, active_buying, dormant_buying);

  // dummy comp, use in_recipe if provided
  cyclus::CompMap v;
  cyclus::Composition::Ptr comp = cyclus::Composition::CreateFromAtom(v);
  if (in_recipe != "") {
    comp = context()->GetRecipe(in_recipe);
  }

  if (in_commod_prefs.size() == 0) {
    for (int i = 0; i < in_commods.size(); ++i) {
      in_commod_prefs.push_back(cyclus::kDefaultPref);
    }
  } else if (in_commod_prefs.size() != in_commods.size()) {
    std::stringstream ss;
    ss << "in_commod_prefs has " << in_commod_prefs.size()
       << " values, expected " << in_commods.size();
    throw cyclus::ValueError(ss.str());
  }

  for (int i = 0; i != in_commods.size(); ++i) {
    buy_policy.Set(in_commods[i], comp, in_commod_prefs[i]);
  }
  buy_policy.Start();

  if (out_commods.size() == 1) {
    sell_policy.Init(this, &stocks, std::string("stocks"), 1e+299, false, sell_quantity)
      .Set(out_commods.front())
      .Start();

  } else {
    std::stringstream ss;
    ss << "out_commods has " << out_commods.size() << " values, expected 1.";
    throw cyclus::ValueError(ss.str());
  }
  RecordPosition();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::string Storage::str() {
  std::stringstream ss;
  std::string ans, out_str;
  if (out_commods.size() == 1) {
    out_str = out_commods.front();
  } else {
    out_str = "";
  }
  if (cyclus::toolkit::CommodityProducer::Produces(
          cyclus::toolkit::Commodity(out_str))) {
    ans = "yes";
  } else {
    ans = "no";
  }
  ss << cyclus::Facility::str();
  ss << " has facility parameters {"
     << "\n"
     << "     Output Commodity = " << out_str << ",\n"
     << "     Residence Time = " << residence_time << ",\n"
     << "     Throughput = " << throughput << ",\n"
     << " commod producer members: "
     << " produces " << out_str << "?:" << ans << "'}";
  return ss.str();
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Storage::Tick() {
  

  LOG(cyclus::LEV_INFO3, "ComCnv") << prototype() << " is ticking {";

  LOG(cyclus::LEV_INFO5, "ComCnv") << "Processing = " << processing.quantity() << ", ready = " << ready.quantity() << ", stocks = " << stocks.quantity() << " and max inventory = " << max_inv_size;

  LOG(cyclus::LEV_INFO4, "ComCnv") << "current capacity " << max_inv_size << " - " << processing.quantity() << " - " << ready.quantity() << " - " << stocks.quantity() << " = " << current_capacity();

  // Set available capacity for Buy Policy
  inventory.capacity(current_capacity());


  if (current_capacity() > cyclus::eps_rsrc()) {
    LOG(cyclus::LEV_INFO4, "ComCnv")
        << " has capacity for " << current_capacity() << ".";
  }
  LOG(cyclus::LEV_INFO3, "ComCnv") << "}";
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Storage::Tock() {
  using cyclus::toolkit::RecordTimeSeries;
  LOG(cyclus::LEV_INFO3, "ComCnv") << prototype() << " is tocking {";

  BeginProcessing_();  // place unprocessed inventory into processing

  LOG(cyclus::LEV_INFO4, "ComCnv") << "processing currently holds " << processing.quantity() << ". ready currently holds " << ready.quantity() << ".";

  if (ready_time() >= 0 || residence_time == 0 && !inventory.empty()) {
    ReadyMatl_(ready_time());  // place processing into ready
  }

  LOG(cyclus::LEV_INFO5, "ComCnv") << "Ready now holds " << ready.quantity() << " kg.";

  if (ready.quantity() > throughput) {
    LOG(cyclus::LEV_INFO5, "ComCnv") << "Up to " << throughput << " kg will be placed in stocks based on throughput limits. ";
    }

  ProcessMat_(throughput);  // place ready into stocks

  std::vector<double>::iterator result;
  result = std::max_element(in_commod_prefs.begin(), in_commod_prefs.end());
  int maxindx = std::distance(in_commod_prefs.begin(), result);
  
  if (manager()->context()->time() % (active_buying + dormant_buying) < active_buying) {
    cyclus::toolkit::RecordTimeSeries<double>("demand"+in_commods[maxindx], this,
                                            current_capacity());
  }
  else {
    cyclus::toolkit::RecordTimeSeries<double>("demand"+in_commods[maxindx], this, 0);
    }
  
  // Multiple commodity tracking is not supported, user can only
  // provide one value for out_commods, despite it being a vector of strings.
  cyclus::toolkit::RecordTimeSeries<double>("supply"+out_commods[0], this,
                                            stocks.quantity());

  LOG(cyclus::LEV_INFO4, "ComCnv") << "process has "
                                   << processing.quantity() << ". Ready has " << ready.quantity() << ". Stocks has " << stocks.quantity() << ".";
  LOG(cyclus::LEV_INFO3, "ComCnv") << "}";
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Storage::AddMat_(cyclus::Material::Ptr mat) {
  LOG(cyclus::LEV_INFO5, "ComCnv") << prototype() << " is initially holding "
                                   << inventory.quantity() << " total.";

  try {
    inventory.Push(mat);
  } catch (cyclus::Error& e) {
    e.msg(Agent::InformErrorMsg(e.msg()));
    throw e;
  }

  LOG(cyclus::LEV_INFO5, "ComCnv")
      << prototype() << " added " << mat->quantity()
      << " of material to its inventory, which is holding "
      << inventory.quantity() << " total.";
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Storage::BeginProcessing_() {
  while (inventory.count() > 0) {
    try {
      processing.Push(inventory.Pop());
      entry_times.push_back(context()->time());

      LOG(cyclus::LEV_DEBUG2, "ComCnv")
          << "Storage " << prototype()
          << " added resources to processing at t= " << context()->time();
    } catch (cyclus::Error& e) {
      e.msg(Agent::InformErrorMsg(e.msg()));
      throw e;
    }
  }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Storage::ProcessMat_(double cap) {
  using cyclus::Material;
  using cyclus::ResCast;
  using cyclus::toolkit::ResBuf;
  using cyclus::toolkit::Manifest;

  if (!ready.empty()) {
    try {
      double max_pop = std::min(cap, ready.quantity());

      if (discrete_handling) {
        if (max_pop == ready.quantity()) {
          stocks.Push(ready.PopN(ready.count()));
        } else {
          double cap_pop = ready.Peek()->quantity();
          while (cap_pop <= max_pop && !ready.empty()) {
            stocks.Push(ready.Pop());
            cap_pop += ready.empty() ? 0 : ready.Peek()->quantity();
          }
        }
      } else {
        stocks.Push(ready.Pop(max_pop, cyclus::eps_rsrc()));
      }

      LOG(cyclus::LEV_INFO1, "ComCnv") << "Storage " << prototype()
                                       << " moved resources"
                                       << " from ready to stocks"
                                       << " at t= " << context()->time();
    } catch (cyclus::Error& e) {
      e.msg(Agent::InformErrorMsg(e.msg()));
      throw e;
    }
  }
}

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Storage::ReadyMatl_(int time) {
  using cyclus::toolkit::ResBuf;
  LOG(cyclus::LEV_INFO5, "ComCnv") << "Placing material into ready";

  int to_ready = 0;

  while (!entry_times.empty() && entry_times.front() <= time) {
    entry_times.pop_front();
    ++to_ready;
  }

  ready.Push(processing.PopN(to_ready));
}

void Storage::RecordPosition() {
  std::string specification = this->spec();
  context()
      ->NewDatum("AgentPosition")
      ->AddVal("Spec", specification)
      ->AddVal("Prototype", this->prototype())
      ->AddVal("AgentId", id())
      ->AddVal("Latitude", latitude)
      ->AddVal("Longitude", longitude)
      ->Record();
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
extern "C" cyclus::Agent* ConstructStorage(cyclus::Context* ctx) {
  return new Storage(ctx);
}

}  // namespace cycamore
