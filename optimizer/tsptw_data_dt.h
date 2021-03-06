#ifndef OR_TOOLS_TUTORIALS_CPLUSPLUS_TSPTW_DATA_DT_H
#define OR_TOOLS_TUTORIALS_CPLUSPLUS_TSPTW_DATA_DT_H

#include <ostream>
#include <iomanip>
#include <vector>

#include "constraint_solver/routing.h"
#include "base/filelinereader.h"
#include "base/split.h"
#include "base/strtoint.h"


#include "routing_data_dt.h"

namespace operations_research {

class TSPTWDataDT : public RoutingDataDT {
public:
  explicit TSPTWDataDT(std::string filename) : RoutingDataDT(0), instantiated_(false) {
    LoadInstance(filename);
    SetRoutingDataInstanciated();
  }
  void LoadInstance(const std::string & filename);
  
  void SetStart(RoutingModel::NodeIndex s) {
    CHECK_LT(s, Size());
    start_ = s;
  }

  void SetStop(RoutingModel::NodeIndex s) {
    CHECK_LT(s, Size());
    stop_ = s;
  }

  RoutingModel::NodeIndex Start() const {
    return start_;
  }

  RoutingModel::NodeIndex Stop() const {
    return stop_;
  }

  int64 Horizon() const {
    return horizon_;
  }

    int64 ReadyTime(RoutingModel::NodeIndex i) const {
    return tsptw_clients_[i.value()].ready_time;
  }

  int64 DueTime(RoutingModel::NodeIndex i) const {
    return tsptw_clients_[i.value()].due_time;
  }

  int64 ServiceTime(RoutingModel::NodeIndex i)  const {
    return  tsptw_clients_[i.value()].service_time;
  }

  int64 Demand(RoutingModel::NodeIndex i) const {
    return tsptw_clients_[i.value()].demand;
  }

  //  Transit quantity at a node "from"
  //  This is the quantity added after visiting node "from"
  int64 DistancePlusServiceTime(RoutingModel::NodeIndex from,
                  RoutingModel::NodeIndex to) const {
    return Distance(from, to) + ServiceTime(from);
  }

  //  Transit quantity at a node "from"
  //  This is the quantity added after visiting node "from"
  int64 TimePlusServiceTime(RoutingModel::NodeIndex from,
                  RoutingModel::NodeIndex to) const {
    return Time(from, to) + ServiceTime(from);
  }

  int64 TimePlus(RoutingModel::NodeIndex from,
                  RoutingModel::NodeIndex to) const {
    return Time(from, to);
  }

  void PrintLIBInstance(std::ostream& out) const;
  void PrintDSUInstance(std::ostream& out) const;
  void WriteLIBInstance(const std::string & filename) const;
  void WriteDSUInstance(const std::string & filename) const;

  
private:
  void ProcessNewLine(char* const line);
  void InitLoadInstance() {
    line_number_ = 0;
    visualizable_ = false;
    two_dimension_ = false;
    symmetric_ = false;
    name_ = "";
    comment_ = "";
  }
  
  //  Helper function
  int64& SetMatrix(int i, int j) {
    return distances_.Cost(RoutingModel::NodeIndex(i), RoutingModel::NodeIndex(j));
  }

  int64& SetTimeMatrix(int i, int j) {
    return times_.Cost(RoutingModel::NodeIndex(i), RoutingModel::NodeIndex(j));
  }


  bool instantiated_;
  RoutingModel::NodeIndex start_, stop_;
  struct TSPTWClient {
    TSPTWClient(int cust_no, double d, double r_t, double d_t, double s_t) :
    customer_number(cust_no), demand(d), ready_time(r_t), due_time(d_t), service_time(s_t) {}
    TSPTWClient(int cust_no, double r_t, double d_t):
    customer_number(cust_no), demand(0.0), ready_time(r_t), due_time(d_t), service_time(0.0){
    }
    TSPTWClient(int cust_no, double r_t, double d_t, double s_t):
    customer_number(cust_no), demand(0.0), ready_time(r_t), due_time(d_t), service_time(s_t){
    }
    int customer_number;
    int64 demand;
    int64 ready_time;
    int64 due_time;
    int64 service_time;
  };

  std::vector<TSPTWClient> tsptw_clients_;
  std::string details_;
  std::string filename_;
  int64 horizon_;
  bool visualizable_;
  bool two_dimension_;
  bool symmetric_;
    
  int line_number_;
  std::string comment_;
};

// Parses a file in López-Ibáñez-Blum or
// da Silva-Urrutia formats and loads the coordinates.
// Note that the format is only partially checked:
// bad inputs might cause undefined behavior.
void TSPTWDataDT::LoadInstance(const std::string & filename) {
  InitLoadInstance();
  size_ = 0;
  FileLineReader reader(filename.c_str());
  reader.set_line_callback(NewPermanentCallback(
                           this,
                           &TSPTWDataDT::ProcessNewLine));
  reader.Reload();
  if (!reader.loaded_successfully()) {
    LOG(ERROR) << "Could not open TSPTW file " << filename;
  }

  //  Number of clients
  size_ = tsptw_clients_.size();

  // Compute horizon
  for (int32 i = 0; i < Size(); ++i  ) {
    horizon_ = std::max(horizon_, tsptw_clients_[i].due_time);
  }

  // Setting start: always first node
  start_ = RoutingModel::NodeIndex(tsptw_clients_[0].customer_number - 1);

  // Setting stop: always last node
  stop_ = RoutingModel::NodeIndex(tsptw_clients_[Size() - 1].customer_number);

  filename_ = filename;
  instantiated_ = true;
}

void TSPTWDataDT::ProcessNewLine(char* const line) {
  ++line_number_;

  static const char kWordDelimiters[] = " ";
  std::vector<std::string> words = strings::Split(line, kWordDelimiters, strings::SkipEmpty());
  
  
  static const int DSU_data_tokens = 7;
  static const int DSU_last_customer = 999;

  //  Empty lines
  if (words.size() == 0) {
    return;
  }

  if (size_ == 0) {
      size_ = atoi32(words[0]);
      CreateRoutingData(size_);
  }

          if (words.size() == size_ * 2) {
            CHECK_LE(line_number_ - 1, size_) << "Distance matrix in TSPTW instance file is ill formed.";
            for (int j = 0; j < Size(); ++j) {
              SetMatrix(line_number_ - 2, j) = static_cast<int64>(atof(words[j*2].c_str()));
              SetTimeMatrix(line_number_ - 2, j) = static_cast<int64>(atof(words[j*2+1].c_str()));
            }
          }
          else if (words.size() == 3) {
            CHECK_LE(line_number_, 2 * Size() + 1) << "Not the right number of clients in TSPTW instance file.";

            tsptw_clients_.push_back(TSPTWClient(line_number_ - Size() -1,
                                                  atof(words[0].c_str()),
                                                  atof(words[1].c_str()),
                                                  atof(words[2].c_str())
            ));
          }

    }  //  void ProcessNewLine(char* const line)

}  //  namespace operations_research

#endif //  OR_TOOLS_TUTORIALS_CPLUSPLUS_TSP_DATA_DT_H